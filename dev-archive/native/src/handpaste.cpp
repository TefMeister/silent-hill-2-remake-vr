// handpaste.cpp -- see handpaste.h.
#include "handpaste.h"

#include <windows.h>

#include <d3d12.h>
#include <d3dcompiler.h>

#include <atomic>
#include <cmath>
#include <cstring>

#include "common.h"
#include "settings.h"

namespace sh2vr::handpaste {

namespace {
// ---- the pass (HLSL, compiled once at run time) -------------------------------------------------------------------
const char* SHADER = R"(
Texture2D tPrev  : register(t0);   // the made-up eye's last real picture
Texture2D tDepth : register(t1);   // its depth
Texture2D tVel   : register(t2);   // the game's velocity picture for it (red > 0: a moving object was drawn here)
Texture2D tWarp  : register(t3);   // the warp's made-up picture
SamplerState sLin : register(s0);
SamplerState sPt  : register(s1);
cbuffer C : register(b0) { float4 h0; float4 h1; float4 h2; float4 misc; }   // rows of the turn's map + (depth line, depth sign, tint); shift

struct V { float4 p : SV_Position; float2 uv : TEXCOORD0; };
V vs(uint id : SV_VertexID) {
    V o; o.uv = float2((id << 1) & 2, id & 2); o.p = float4(o.uv * float2(2, -2) + float2(-1, 1), 0, 1); return o;
}
bool hand(float2 uv) {
    if (any(uv < 0) || any(uv > 1)) return false;
    float d = tDepth.SampleLevel(sPt, uv, 0).r;
    float v = tVel.SampleLevel(sPt, uv, 0).r;
    return (d - h0.w) * h1.w > 0 && v > 0;
}
// where, in the last real picture, the warp took the pixel it put at uv (near pixels: the head's turn alone)
float2 source_of(float2 uv, out bool ok) {
    float3 n = float3(uv.x * 2 - 1, 1 - uv.y * 2, 1);
    float3 q = float3(dot(h0.xyz, n), dot(h1.xyz, n), dot(h2.xyz, n));
    ok = q.z > 1e-6;
    float2 p = q.xy / q.z;
    return float2(p.x * 0.5 + 0.5, 0.5 - p.y * 0.5);
}
bool ghost(float2 uv) { bool ok; float2 s = source_of(uv, ok); return ok && hand(s); }

float4 ps(V i) : SV_Target {
    float2 uv = i.uv, shift = misc.xy;
    if (hand(uv - shift)) {
        float4 c = tPrev.SampleLevel(sLin, uv - shift, 0);
        if (h2.w > 0.5) c.rgb = lerp(c.rgb, float3(0, 1, 0), 0.4);
        return c;
    }
    bool ok; float2 s = source_of(uv, ok);
    float4 c = tWarp.SampleLevel(sLin, uv, 0);
    if (ok && hand(s)) {
        float2 dir = (uv - s) / 6.0;   // the way the room went: the background beyond the copy lies that way
        for (int k = 1; k <= 12; k++) {
            float2 t = uv + dir * k;
            if (!ghost(t) && !hand(t - shift)) { c = tWarp.SampleLevel(sLin, t, 0); break; }
        }
        if (h2.w > 0.5) c.rgb = lerp(c.rgb, float3(1, 0, 0), 0.4);
    }
    return c;
}
)";

// ---- state (the render thread's own, except the atomics) ----------------------------------------------------------
std::atomic<int> g_mode{settings::HANDPASTE_MODE};
std::atomic<float> g_near{settings::HANDPASTE_NEAR_M};
std::atomic<int> g_draws{0}, g_skips{0};
std::atomic<bool> g_failed{false};
std::atomic<const char*> g_why{nullptr};

ID3D12Device* g_dev = nullptr;
ID3D12RootSignature* g_root = nullptr;
ID3D12PipelineState* g_pso = nullptr;
DXGI_FORMAT g_pso_format = DXGI_FORMAT_UNKNOWN;
ID3DBlob* g_vs = nullptr; ID3DBlob* g_ps = nullptr;
ID3D12DescriptorHeap* g_srv_heap = nullptr; ID3D12DescriptorHeap* g_rtv_heap = nullptr;
UINT g_srv_size = 0, g_rtv_size = 0;
ID3D12Resource* g_target = nullptr;            // the pass draws here, then it is copied over the warp's picture
ID3D12Resource* g_keep[2] = {nullptr, nullptr};   // each eye's last real picture
D3D12_RESOURCE_DESC g_desc{};                  // what the three were made for

struct Eye {
    bool have = false;
    uint64_t frame = 0;
    ID3D12Resource* color = nullptr; uint32_t color_state = 0;
    ID3D12Resource* depth = nullptr; ID3D12Resource* velocity = nullptr;
    float w2v[16]{}, v2c[16]{};   // its real view that frame
};
Eye g_eye[2];
uint64_t g_frame = 0;

void hold(ID3D12Resource*& slot, ID3D12Resource* r) {
    if (slot == r) return;
    if (r != nullptr) r->AddRef();
    if (slot != nullptr) slot->Release();
    slot = r;
}

constexpr int SRVS_PER_DRAW = 4;
constexpr D3D12_RESOURCE_STATES SHADER_READ = (D3D12_RESOURCE_STATES)(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

bool fail(const char* why) { g_why.store(why); g_failed.store(true); return false; }

DXGI_FORMAT view_format(DXGI_FORMAT f) {   // a format a picture can be read through
    switch (f) {
    case DXGI_FORMAT_R32_TYPELESS: case DXGI_FORMAT_D32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_R24G8_TYPELESS: case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case DXGI_FORMAT_R32G8X24_TYPELESS: case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case DXGI_FORMAT_R16_TYPELESS: case DXGI_FORMAT_D16_UNORM: return DXGI_FORMAT_R16_UNORM;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS: return DXGI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS: return DXGI_FORMAT_R10G10B10A2_UNORM;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    default: return f;
    }
}

void transition(ID3D12GraphicsCommandList* l, ID3D12Resource* r, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to) {
    if (from == to) return;
    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = r; b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = from; b.Transition.StateAfter = to;
    l->ResourceBarrier(1, &b);
}

bool make_texture(const D3D12_RESOURCE_DESC& like, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, ID3D12Resource** out) {
    D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC d = like;
    d.Format = view_format(like.Format); d.Flags = flags; d.MipLevels = 1; d.Alignment = 0;
    return SUCCEEDED(g_dev->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &d, state, nullptr, IID_PPV_ARGS(out)));
}

bool make_basics(ID3D12GraphicsCommandList* list) {
    if (g_dev != nullptr) return true;
    if (FAILED(list->GetDevice(IID_PPV_ARGS(&g_dev)))) return fail("no device from the command list");
    ID3DBlob* err = nullptr;
    if (FAILED(D3DCompile(SHADER, std::strlen(SHADER), "handpaste", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &g_vs, &err))) return fail("the vertex program did not compile");
    if (FAILED(D3DCompile(SHADER, std::strlen(SHADER), "handpaste", nullptr, nullptr, "ps", "ps_5_0", 0, 0, &g_ps, &err))) return fail("the pixel program did not compile");

    D3D12_DESCRIPTOR_RANGE range{}; range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; range.NumDescriptors = SRVS_PER_DRAW;
    D3D12_ROOT_PARAMETER rp[2]{};
    rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; rp[0].DescriptorTable.NumDescriptorRanges = 1; rp[0].DescriptorTable.pDescriptorRanges = &range;
    rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS; rp[1].Constants.Num32BitValues = 16; rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_STATIC_SAMPLER_DESC samp[2]{};
    for (int i = 0; i < 2; ++i) {
        samp[i].Filter = i == 0 ? D3D12_FILTER_MIN_MAG_MIP_LINEAR : D3D12_FILTER_MIN_MAG_MIP_POINT;
        samp[i].AddressU = samp[i].AddressV = samp[i].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        samp[i].MaxLOD = D3D12_FLOAT32_MAX; samp[i].ShaderRegister = (UINT)i; samp[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }
    D3D12_ROOT_SIGNATURE_DESC rs{}; rs.NumParameters = 2; rs.pParameters = rp; rs.NumStaticSamplers = 2; rs.pStaticSamplers = samp;
    ID3DBlob* blob = nullptr;
    if (FAILED(D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &err))) return fail("the root signature did not serialise");
    if (FAILED(g_dev->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&g_root)))) return fail("the root signature was refused");
    blob->Release();

    D3D12_DESCRIPTOR_HEAP_DESC hd{}; hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; hd.NumDescriptors = SRVS_PER_DRAW * 2; hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(g_dev->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&g_srv_heap)))) return fail("no descriptor heap");
    D3D12_DESCRIPTOR_HEAP_DESC rd{}; rd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; rd.NumDescriptors = 1;
    if (FAILED(g_dev->CreateDescriptorHeap(&rd, IID_PPV_ARGS(&g_rtv_heap)))) return fail("no target heap");
    g_srv_size = g_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    g_rtv_size = g_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    return true;
}

bool make_pso(DXGI_FORMAT format) {
    if (g_pso != nullptr && g_pso_format == format) return true;
    if (g_pso != nullptr) { g_pso->Release(); g_pso = nullptr; }
    D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};
    d.pRootSignature = g_root;
    d.VS = {g_vs->GetBufferPointer(), g_vs->GetBufferSize()}; d.PS = {g_ps->GetBufferPointer(), g_ps->GetBufferSize()};
    d.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    d.SampleMask = 0xFFFFFFFF;
    d.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; d.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    d.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    d.NumRenderTargets = 1; d.RTVFormats[0] = format; d.SampleDesc.Count = 1;
    if (FAILED(g_dev->CreateGraphicsPipelineState(&d, IID_PPV_ARGS(&g_pso)))) return fail("the pipeline was refused");
    g_pso_format = format;
    return true;
}

bool make_pictures(ID3D12Resource* color) {
    const D3D12_RESOURCE_DESC d = color->GetDesc();
    if (g_target != nullptr && d.Width == g_desc.Width && d.Height == g_desc.Height && d.Format == g_desc.Format) return true;
    for (ID3D12Resource** r : {&g_target, &g_keep[0], &g_keep[1]}) if (*r != nullptr) { (*r)->Release(); *r = nullptr; }
    g_eye[0].have = g_eye[1].have = false;
    if (d.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || d.SampleDesc.Count != 1) return fail("the eye picture is not a plain 2D picture");
    if (!make_texture(d, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET, &g_target)) return fail("no picture to draw into");
    for (int i = 0; i < 2; ++i) if (!make_texture(d, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &g_keep[i])) return fail("no picture to keep the last real one in");
    g_desc = d;
    LOGI("%s handpaste: pictures made, %llu x %u, format %d", TAG, (unsigned long long)d.Width, d.Height, (int)d.Format);
    return true;
}

void srv(ID3D12Resource* r, int slot) {
    D3D12_SHADER_RESOURCE_VIEW_DESC v{};
    v.Format = view_format(r->GetDesc().Format); v.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    v.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; v.Texture2D.MipLevels = 1;
    D3D12_CPU_DESCRIPTOR_HANDLE h = g_srv_heap->GetCPUDescriptorHandleForHeapStart(); h.ptr += (SIZE_T)slot * g_srv_size;
    g_dev->CreateShaderResourceView(r, &v, h);
}

// ---- small matrix helpers (glm layout: m[column * 4 + row]) ------------------------------------------------------
struct M3 { float m[9]; };   // row-major 3x3
M3 mul3(const M3& a, const M3& b) { M3 o{}; for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c) for (int k = 0; k < 3; ++k) o.m[r * 3 + c] += a.m[r * 3 + k] * b.m[k * 3 + c]; return o; }
float at(const float* m, int row, int col) { return m[col * 4 + row]; }
M3 rot_of(const float* m) { M3 o{}; for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c) o.m[r * 3 + c] = at(m, r, c); return o; }

// the depth-picture value of a point `metres` straight ahead, and which way "nearer" goes
bool depth_line(const float* v2c, float metres, float& line, float& sign) {
    auto value = [&](float z, float& out) {
        const float cz = at(v2c, 2, 2) * z + at(v2c, 2, 3), cw = at(v2c, 3, 2) * z + at(v2c, 3, 3);
        if (std::fabs(cw) < 1e-9f) return false;
        out = cz / cw; return true;
    };
    const float ahead = (at(v2c, 3, 2) < 0.0f) ? -1.0f : 1.0f;   // which way the view looks down its z
    float far_value = 0.0f;
    if (!value(ahead * metres, line) || !value(ahead * metres * 50.0f, far_value) || line == far_value) return false;
    sign = line > far_value ? 1.0f : -1.0f;
    return true;
}

void draw(const Call& c, const Eye& last, ID3D12Resource* warped, D3D12_RESOURCE_STATES warped_state) {
    // the turn's map: a pixel of the made-up view now -> where that direction was in the eye's last real view
    const float* dest_c2v = c.camera + 3 * 16; const float* dest_v2w = c.camera + 1 * 16;
    float line = 0.0f, sign = 1.0f;
    if (!depth_line(last.v2c, g_near.load(), line, sign)) { g_skips.fetch_add(1); return; }
    // view direction of (x, y, 1): columns x, y of clip-to-view, and its (depth line) z column + w column
    M3 unproject{};
    float wsum = at(dest_c2v, 3, 2) * line + at(dest_c2v, 3, 3);
    for (int r = 0; r < 3; ++r) {
        unproject.m[r * 3 + 0] = at(dest_c2v, r, 0); unproject.m[r * 3 + 1] = at(dest_c2v, r, 1);
        unproject.m[r * 3 + 2] = at(dest_c2v, r, 2) * line + at(dest_c2v, r, 3);
    }
    const float flip = wsum < 0.0f ? -1.0f : 1.0f;   // the point's w: keeps "in front" in front
    M3 project{};   // rows x, y, w of the last view's projection, for a direction (w = 0)
    for (int col = 0; col < 3; ++col) { project.m[0 * 3 + col] = at(last.v2c, 0, col); project.m[1 * 3 + col] = at(last.v2c, 1, col); project.m[2 * 3 + col] = at(last.v2c, 3, col); }
    M3 h = mul3(project, mul3(rot_of(last.w2v), mul3(rot_of(dest_v2w), unproject)));
    for (float& v : h.m) v *= flip;

    const int mode = g_mode.load();
    const float constants[16] = {h.m[0], h.m[1], h.m[2], line, h.m[3], h.m[4], h.m[5], sign, h.m[6], h.m[7], h.m[8], mode >= 2 ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

    const int base = (1 - c.eye) * SRVS_PER_DRAW;
    srv(g_keep[1 - c.eye], base + 0); srv(last.depth, base + 1); srv(last.velocity != nullptr ? last.velocity : last.depth, base + 2); srv(warped, base + 3);
    D3D12_RENDER_TARGET_VIEW_DESC rv{}; rv.Format = view_format(g_desc.Format); rv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    const D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_rtv_heap->GetCPUDescriptorHandleForHeapStart();
    g_dev->CreateRenderTargetView(g_target, &rv, rtv);

    auto* l = c.list;
    const D3D12_VIEWPORT vp{0.0f, 0.0f, (float)g_desc.Width, (float)g_desc.Height, 0.0f, 1.0f};
    const D3D12_RECT sc{0, 0, (LONG)g_desc.Width, (LONG)g_desc.Height};
    transition(l, warped, warped_state, SHADER_READ);
    l->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    l->RSSetViewports(1, &vp); l->RSSetScissorRects(1, &sc);
    l->SetPipelineState(g_pso); l->SetGraphicsRootSignature(g_root);
    l->SetDescriptorHeaps(1, &g_srv_heap);
    D3D12_GPU_DESCRIPTOR_HANDLE table = g_srv_heap->GetGPUDescriptorHandleForHeapStart(); table.ptr += (UINT64)base * g_srv_size;
    l->SetGraphicsRootDescriptorTable(0, table);
    l->SetGraphicsRoot32BitConstants(1, 16, constants, 0);
    l->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    l->DrawInstanced(3, 1, 0, 0);
    // over the warp's picture
    transition(l, g_target, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    transition(l, warped, SHADER_READ, D3D12_RESOURCE_STATE_COPY_DEST);
    l->CopyResource(warped, g_target);
    transition(l, warped, D3D12_RESOURCE_STATE_COPY_DEST, warped_state);
    transition(l, g_target, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    g_draws.fetch_add(1);
}
} // namespace

void set_mode(int mode) { g_mode.store(mode); }
void set_near(float metres) { g_near.store(metres); }

void after_warp(const Call& c) {
    ++g_frame;
    if (g_failed.load() || c.list == nullptr || c.color == nullptr || c.depth == nullptr || c.camera == nullptr || c.eye < 0 || c.eye > 1) return;
    const int mode = g_mode.load();
    if (mode <= 0) { g_eye[0].have = g_eye[1].have = false; return; }
    if (!make_basics(c.list) || !make_pictures(c.color) || !make_pso(view_format(g_desc.Format))) return;

    // 1. the made-up eye: the warp has just written its picture where that eye's last real one was
    const Eye& last = g_eye[1 - c.eye];
    if (c.previous_frame_mode && last.have && last.frame + 1 == g_frame && last.color != nullptr && last.depth != nullptr) {
        const D3D12_RESOURCE_DESC wd = last.color->GetDesc();
        if (wd.Width == g_desc.Width && wd.Height == g_desc.Height && wd.Format == g_desc.Format) draw(c, last, last.color, (D3D12_RESOURCE_STATES)last.color_state);
        else g_skips.fetch_add(1);
    } else g_skips.fetch_add(1);

    // 2. the drawn eye: keep its real picture and its view for the next frame
    Eye& now = g_eye[c.eye];
    auto* l = c.list;
    const auto rest = (D3D12_RESOURCE_STATES)c.color_state;
    transition(l, c.color, rest, D3D12_RESOURCE_STATE_COPY_SOURCE);
    transition(l, g_keep[c.eye], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    l->CopyResource(g_keep[c.eye], c.color);
    transition(l, g_keep[c.eye], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    transition(l, c.color, D3D12_RESOURCE_STATE_COPY_SOURCE, rest);
    // the injector replaces its pictures at a level load or a size change: hold each one for the frame it is still needed
    hold(now.color, c.color); hold(now.depth, c.depth); hold(now.velocity, c.velocity);
    now.have = true; now.frame = g_frame; now.color_state = c.color_state;
    std::memcpy(now.w2v, c.camera + 4 * 16, sizeof(now.w2v)); std::memcpy(now.v2c, c.camera + 6 * 16, sizeof(now.v2c));
}

void report() {
    static bool said_fail = false;
    if (g_failed.load() && !said_fail) { said_fail = true; LOGW("%s handpaste: switched itself off: %s", TAG, g_why.load() != nullptr ? g_why.load() : "?"); }
    const int d = g_draws.exchange(0), s = g_skips.exchange(0);
    if (d > 0 || s > 0) LOGI("%s handpaste: %d made-up pictures redrawn, %d left as the warp made them", TAG, d, s);
}

} // namespace sh2vr::handpaste
