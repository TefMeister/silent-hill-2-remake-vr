// deband.cpp -- see deband.h.
#include "deband.h"

#include <windows.h>

#include <d3d12.h>
#include <d3dcompiler.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>

#include "common.h"
#include "settings.h"

namespace sh2vr::deband {
namespace {
const char* SHADER = R"(
Texture2D tPic : register(t0);
SamplerState sPt : register(s0);
cbuffer C : register(b0) { float4 p; }   // x: radius in pixels, y: threshold (of 1), z: grain (of 1), w: a number that changes every frame
struct V { float4 pos : SV_Position; float2 uv : TEXCOORD0; };
V vs(uint id : SV_VertexID) {
    V o; o.uv = float2((id << 1) & 2, id & 2); o.pos = float4(o.uv * float2(2, -2) + float2(-1, 1), 0, 1); return o;
}
float hash(float2 q, float s) { float3 a = frac(float3(q.xyx) * 0.1031 + s * 0.071); a += dot(a, a.yzx + 33.33); return frac((a.x + a.y) * a.z); }
float4 ps(V i) : SV_Target {
    float w, h; tPic.GetDimensions(w, h);
    float2 px = float2(1.0 / w, 1.0 / h);
    float4 c = tPic.SampleLevel(sPt, i.uv, 0);
    float ang = hash(i.pos.xy, p.w) * 6.2831853;
    float2 d = float2(cos(ang), sin(ang)) * p.x * (0.5 + 0.5 * hash(i.pos.yx, p.w + 7.0));
    float3 a = tPic.SampleLevel(sPt, i.uv + d * px, 0).rgb;
    float3 b = tPic.SampleLevel(sPt, i.uv - d * px, 0).rgb;
    float3 e = tPic.SampleLevel(sPt, i.uv + float2(-d.y, d.x) * px, 0).rgb;
    float3 f = tPic.SampleLevel(sPt, i.uv - float2(-d.y, d.x) * px, 0).rgb;
    // a slow gradient: the four around average out to the pixel itself (within the threshold) and none is far from it.
    // (b042 first try asked every one of the four to be within the threshold: the fog's steps are about 10 pixels
    // apart, so one of them was nearly always a step further and the fog was left alone: measured, no change.)
    float3 avg = (a + b + e + f) * 0.25;
    float3 off = abs(avg - c.rgb);
    float3 dev = max(max(abs(a - c.rgb), abs(b - c.rgb)), max(abs(e - c.rgb), abs(f - c.rgb)));
    if (max(off.r, max(off.g, off.b)) > p.y || max(dev.r, max(dev.g, dev.b)) > p.y * 3.0) return c;
    float n = (hash(i.pos.xy, p.w + 13.0) + hash(i.pos.xy, p.w + 29.0) - 1.0) * p.z;   // triangular, one level wide either way
    return float4(saturate(avg + n), c.a);
}
)";

std::atomic<bool> g_on{settings::DEBAND_ON};
std::atomic<float> g_radius{settings::DEBAND_RADIUS_PX}, g_threshold{settings::DEBAND_THRESHOLD_LEVELS}, g_grain{settings::DEBAND_GRAIN_LEVELS};
std::atomic<int> g_draws{0};
std::atomic<const char*> g_why{nullptr};
std::atomic<bool> g_failed{false};
bool fail(const char* why) { g_why.store(why); g_failed.store(true); return false; }

// the render thread's own
ID3D12Device* g_dev = nullptr;
ID3D12RootSignature* g_root = nullptr;
ID3D12PipelineState* g_pso = nullptr;
ID3DBlob* g_vs = nullptr; ID3DBlob* g_ps = nullptr;
ID3D12DescriptorHeap* g_srv_heap = nullptr; ID3D12DescriptorHeap* g_rtv_heap = nullptr;
UINT g_srv_size = 0;
constexpr UINT CB_SLOT = 256, CB_SLOTS = 16, SRV_SLOTS = 8;
ID3D12Resource* g_cb = nullptr; uint8_t* g_cb_map = nullptr; UINT g_cb_next = 0, g_srv_next = 0;
ID3D12Resource* g_target = nullptr;
D3D12_RESOURCE_DESC g_desc{};
ID3D12Resource* g_last[2] = {nullptr, nullptr};   // each eye's picture as last seen (the made-up one is written into it the frame after)
uint32_t g_last_state[2] = {0, 0};
uint64_t g_last_frame[2] = {0, 0}, g_frame = 0;
uint32_t g_seed = 0;

void transition(ID3D12GraphicsCommandList* l, ID3D12Resource* r, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to) {
    if (from == to) return;
    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = r; b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = from; b.Transition.StateAfter = to;
    l->ResourceBarrier(1, &b);
}
// only plain 8-bit pictures are stepped like this, and only those are handled
bool eight_bit(DXGI_FORMAT f) { return f == DXGI_FORMAT_B8G8R8A8_UNORM || f == DXGI_FORMAT_R8G8B8A8_UNORM; }

bool make_basics(ID3D12GraphicsCommandList* list) {
    if (g_dev != nullptr) return true;
    if (FAILED(list->GetDevice(IID_PPV_ARGS(&g_dev)))) return fail("no device from the command list");
    ID3DBlob* err = nullptr;
    if (FAILED(D3DCompile(SHADER, std::strlen(SHADER), "deband", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &g_vs, &err))) return fail("the vertex program did not compile");
    if (FAILED(D3DCompile(SHADER, std::strlen(SHADER), "deband", nullptr, nullptr, "ps", "ps_5_0", 0, 0, &g_ps, &err))) return fail("the pixel program did not compile");
    D3D12_DESCRIPTOR_RANGE range{}; range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; range.NumDescriptors = 1;
    D3D12_ROOT_PARAMETER rp[2]{};
    rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; rp[0].DescriptorTable.NumDescriptorRanges = 1; rp[0].DescriptorTable.pDescriptorRanges = &range;
    rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_STATIC_SAMPLER_DESC samp{};
    samp.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    samp.AddressU = samp.AddressV = samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samp.MaxLOD = D3D12_FLOAT32_MAX; samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC rs{}; rs.NumParameters = 2; rs.pParameters = rp; rs.NumStaticSamplers = 1; rs.pStaticSamplers = &samp;
    ID3DBlob* blob = nullptr;
    if (FAILED(D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &err))) return fail("the root signature did not serialise");
    if (FAILED(g_dev->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&g_root)))) return fail("the root signature was refused");
    blob->Release();
    D3D12_DESCRIPTOR_HEAP_DESC hd{}; hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; hd.NumDescriptors = SRV_SLOTS; hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(g_dev->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&g_srv_heap)))) return fail("no descriptor heap");
    g_srv_size = g_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_DESCRIPTOR_HEAP_DESC rd{}; rd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; rd.NumDescriptors = 1;
    if (FAILED(g_dev->CreateDescriptorHeap(&rd, IID_PPV_ARGS(&g_rtv_heap)))) return fail("no target heap");
    D3D12_HEAP_PROPERTIES up{}; up.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bd{}; bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; bd.Width = (UINT64)CB_SLOT * CB_SLOTS; bd.Height = 1; bd.DepthOrArraySize = 1; bd.MipLevels = 1;
    bd.SampleDesc.Count = 1; bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (FAILED(g_dev->CreateCommittedResource(&up, D3D12_HEAP_FLAG_NONE, &bd, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&g_cb)))) return fail("no buffer for the pass's numbers");
    if (FAILED(g_cb->Map(0, nullptr, (void**)&g_cb_map))) return fail("the numbers' buffer did not map");
    return true;
}

bool make_target(ID3D12Resource* color) {
    const D3D12_RESOURCE_DESC d = color->GetDesc();
    if (g_target != nullptr && d.Width == g_desc.Width && d.Height == g_desc.Height && d.Format == g_desc.Format) return true;
    if (g_target != nullptr) { g_target->Release(); g_target = nullptr; }
    if (g_pso != nullptr) { g_pso->Release(); g_pso = nullptr; }
    g_desc = D3D12_RESOURCE_DESC{};
    if (d.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || d.SampleDesc.Count != 1 || !eight_bit(d.Format)) return false;   // not a picture this pass is for: left alone, quietly
    D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC t = d; t.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; t.MipLevels = 1; t.Alignment = 0;
    if (FAILED(g_dev->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &t, D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&g_target)))) return fail("no picture to draw into");
    D3D12_GRAPHICS_PIPELINE_STATE_DESC p{};
    p.pRootSignature = g_root;
    p.VS = {g_vs->GetBufferPointer(), g_vs->GetBufferSize()}; p.PS = {g_ps->GetBufferPointer(), g_ps->GetBufferSize()};
    p.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    p.SampleMask = 0xFFFFFFFF;
    p.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; p.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    p.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    p.NumRenderTargets = 1; p.RTVFormats[0] = d.Format; p.SampleDesc.Count = 1;
    if (FAILED(g_dev->CreateGraphicsPipelineState(&p, IID_PPV_ARGS(&g_pso)))) return fail("the pipeline was refused");
    g_desc = d;
    LOGI("%s deband: ready for pictures of %llu x %u, format %d", TAG, (unsigned long long)d.Width, d.Height, (int)d.Format);
    return true;
}

void one(ID3D12GraphicsCommandList* l, ID3D12Resource* pic, D3D12_RESOURCE_STATES rest) {
    const D3D12_RESOURCE_DESC d = pic->GetDesc();
    if (d.Width != g_desc.Width || d.Height != g_desc.Height || d.Format != g_desc.Format) return;
    const float numbers[4] = {g_radius.load(), g_threshold.load() / 255.0f, g_grain.load() / 255.0f, (float)(g_seed++ % 1024)};
    uint8_t* slot = g_cb_map + (size_t)g_cb_next * CB_SLOT;
    std::memcpy(slot, numbers, sizeof(numbers));
    const D3D12_GPU_VIRTUAL_ADDRESS cb_at = g_cb->GetGPUVirtualAddress() + (UINT64)g_cb_next * CB_SLOT;
    g_cb_next = (g_cb_next + 1) % CB_SLOTS;
    D3D12_SHADER_RESOURCE_VIEW_DESC v{};
    v.Format = d.Format; v.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; v.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; v.Texture2D.MipLevels = 1;
    D3D12_CPU_DESCRIPTOR_HANDLE sh = g_srv_heap->GetCPUDescriptorHandleForHeapStart(); sh.ptr += (SIZE_T)g_srv_next * g_srv_size;
    g_dev->CreateShaderResourceView(pic, &v, sh);
    D3D12_GPU_DESCRIPTOR_HANDLE table = g_srv_heap->GetGPUDescriptorHandleForHeapStart(); table.ptr += (UINT64)g_srv_next * g_srv_size;
    g_srv_next = (g_srv_next + 1) % SRV_SLOTS;
    D3D12_RENDER_TARGET_VIEW_DESC rv{}; rv.Format = d.Format; rv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    const D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_rtv_heap->GetCPUDescriptorHandleForHeapStart();
    g_dev->CreateRenderTargetView(g_target, &rv, rtv);
    const D3D12_VIEWPORT vp{0.0f, 0.0f, (float)d.Width, (float)d.Height, 0.0f, 1.0f};
    const D3D12_RECT sc{0, 0, (LONG)d.Width, (LONG)d.Height};
    transition(l, pic, rest, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    l->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    l->RSSetViewports(1, &vp); l->RSSetScissorRects(1, &sc);
    l->SetPipelineState(g_pso); l->SetGraphicsRootSignature(g_root);
    l->SetDescriptorHeaps(1, &g_srv_heap);
    l->SetGraphicsRootDescriptorTable(0, table);
    l->SetGraphicsRootConstantBufferView(1, cb_at);
    l->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    l->DrawInstanced(3, 1, 0, 0);
    transition(l, g_target, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    transition(l, pic, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    l->CopyResource(pic, g_target);
    transition(l, pic, D3D12_RESOURCE_STATE_COPY_DEST, rest);
    transition(l, g_target, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    g_draws.fetch_add(1);
}

void hold(ID3D12Resource*& slot, ID3D12Resource* r) {
    if (slot == r) return;
    if (r != nullptr) r->AddRef();
    if (slot != nullptr) slot->Release();
    slot = r;
}
} // namespace

void after_warp(ID3D12GraphicsCommandList* list, int eye, ID3D12Resource* color, uint32_t color_state) {
    ++g_frame;
    if (list == nullptr || color == nullptr || eye < 0 || eye > 1) return;
    const int other = 1 - eye;
    const bool made_up_ready = g_last[other] != nullptr && g_last_frame[other] + 1 == g_frame;
    if (g_on.load() && !g_failed.load() && make_basics(list) && make_target(color)) {
        if (made_up_ready) one(list, g_last[other], (D3D12_RESOURCE_STATES)g_last_state[other]);   // the made-up eye: the warp (and the hands' pass) wrote it this call
        one(list, color, (D3D12_RESOURCE_STATES)color_state);                                      // the drawn eye (the warp and the hands' pass took their copies of it before this)
    }
    hold(g_last[eye], color); g_last_state[eye] = color_state; g_last_frame[eye] = g_frame;
}

void tick(float delta) {
    static float file_timer = 0.0f, report_timer = 0.0f;
    file_timer += delta; report_timer += delta;
    if (file_timer >= 1.0f) {
        file_timer = 0.0f;
        bool on = settings::DEBAND_ON; float r = settings::DEBAND_RADIUS_PX, t = settings::DEBAND_THRESHOLD_LEVELS, g = settings::DEBAND_GRAIN_LEVELS;
        const std::wstring p = appdata_dir() + L"\\" + settings::PROFILE_DIR + L"\\sh2vr_deband.txt";
        FILE* f = nullptr;
        if (_wfopen_s(&f, p.c_str(), L"r") == 0 && f != nullptr) {
            float a = -1.0f, b = -1.0f, c = -1.0f, d = -1.0f;
            const int n = fscanf_s(f, "%f %f %f %f", &a, &b, &c, &d);
            if (n >= 1 && (a == 0.0f || a == 1.0f)) on = a == 1.0f;
            if (n >= 2 && b >= 1.0f && b <= 64.0f) r = b;
            if (n >= 3 && c >= 0.0f && c <= 8.0f) t = c;
            if (n >= 4 && d >= 0.0f && d <= 4.0f) g = d;
            std::fclose(f);
        }
        static bool said = false;
        if (!said || on != g_on.load() || r != g_radius.load() || t != g_threshold.load() || g != g_grain.load()) {
            said = true;
            g_on.store(on); g_radius.store(r); g_threshold.store(t); g_grain.store(g);
            LOGI("%s deband: the fog's contour lines pass %s (radius %.0f pixels, threshold %.2f levels, grain %.2f levels)", TAG, on ? "on" : "off", r, t, g);
        }
    }
    if (report_timer >= settings::WARP_REPORT_S) {
        report_timer = 0.0f;
        static bool said_fail = false, said_first = false;
        if (g_failed.load() && !said_fail) { said_fail = true; LOGW("%s deband: switched itself off: %s", TAG, g_why.load() != nullptr ? g_why.load() : "?"); }
        const int d = g_draws.exchange(0);
        if (d > 0 && !said_first) { said_first = true; LOGI("%s deband: %d pictures smoothed in the last %.0f s", TAG, d, settings::WARP_REPORT_S); }
    }
}

} // namespace sh2vr::deband
