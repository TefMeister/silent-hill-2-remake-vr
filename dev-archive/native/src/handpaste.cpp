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
#include "uevr/API.hpp"

namespace sh2vr::handpaste {

namespace {
// ---- the pass (HLSL, compiled once at run time) -------------------------------------------------------------------
const char* SHADER = R"(
Texture2D tPrev  : register(t0);   // the made-up eye's last real picture
Texture2D tDepth : register(t1);   // its depth
Texture2D tVel   : register(t2);   // the game's velocity picture for it (red > 0: a moving object was drawn here)
Texture2D tWarp  : register(t3);   // the warp's made-up picture
Texture2D tOther : register(t4);   // the OTHER eye's picture, really drawn this very frame
Texture2D tOtherDepth : register(t5);
SamplerState sLin : register(s0);
SamplerState sPt  : register(s1);
cbuffer C : register(b0) {
    float4 h0; float4 h1; float4 h2;        // rows of the turn's map; w: depth line, depth sign, tint
    float4 tl0; float4 tl1; float4 tl2;     // the left hand's move since the last real picture (view space, rows of a 3x4)
    float4 tr0; float4 tr1; float4 tr2;     // the right hand's
    float4 pj0; float4 pj1;                 // the eye's projection: (m00, m02, m11, m12), (m22, m23, m32, m33)
    float4 shifts;                          // a first guess of each hand's shift in the picture (left xy, right xy)
    float4 cl; float4 cr;                   // where each controller was then (view space); cl.w: the blend band in metres
    float4 cam0; float4 cam1; float4 cam2;  // a point in front of the eye -> where the game's "previous" view saw it (rows x, y, w)
    float4 misc;                            // x: how far (picture units) a pixel must move against the room to count; y: 1 = that test is on; z: 1 = the reach test is on
    float4 oe0; float4 oe1; float4 oe2;     // a point in front of this eye now -> the other eye's picture now (rows x, y, w)
    float4 fl; float4 ul; float4 sl;        // left controller: where it points, its up, that shoulder (view space, then)
    float4 fr; float4 ur; float4 sr;        // right controller; fl.w: weapon reach radius, ul.w: arm radius
}

struct V { float4 p : SV_Position; float2 uv : TEXCOORD0; };
V vs(uint id : SV_VertexID) {
    V o; o.uv = float2((id << 1) & 2, id & 2); o.p = float4(o.uv * float2(2, -2) + float2(-1, 1), 0, 1); return o;
}
bool near_depth(float d) { return (d - h0.w) * h1.w > 0; }
float3 unproject(float2 uv, float d);
float seg_dist(float3 p, float3 a, float3 b) { float3 ab = b - a; float t = saturate(dot(p - a, ab) / max(dot(ab, ab), 1e-6)); return distance(p, a + ab * t); }
// within reach of a controller: along where it points and along its up (a gun, a pipe, a plank), or along the arm behind it
bool in_reach(float3 p) {
    float w = fl.w, a = ul.w;
    if (seg_dist(p, cl.xyz - fl.xyz * 0.35, cl.xyz + fl.xyz * 1.2) < w) return true;
    if (seg_dist(p, cl.xyz - ul.xyz * 1.2, cl.xyz + ul.xyz * 1.2) < w) return true;
    if (seg_dist(p, cl.xyz, sl.xyz) < a) return true;
    if (seg_dist(p, cr.xyz - fr.xyz * 0.35, cr.xyz + fr.xyz * 1.2) < w) return true;
    if (seg_dist(p, cr.xyz - ur.xyz * 1.2, cr.xyz + ur.xyz * 1.2) < w) return true;
    if (seg_dist(p, cr.xyz, sr.xyz) < a) return true;
    return false;
}
// hands and weapon: close to the eyes AND moving differently from the room. The game's velocity picture holds every
// pixel's motion in the picture (the camera's included); what a still thing would show is worked out from its depth.
bool hand(float2 uv) {
    if (any(uv < 0) || any(uv > 1)) return false;
    float d = tDepth.SampleLevel(sPt, uv, 0).r;
    if (!near_depth(d)) return false;
    if (misc.y < 0.5) return true;
    float2 e = tVel.SampleLevel(sPt, uv, 0).rg;
    if (e.r < 0.01) return false;                     // nothing written (the cleared value is not exactly 0 here): a still thing
    float2 v = e * 4.008 - 2.004; v = v * abs(v) * 0.5;
    float4 p = float4(unproject(uv, d), 1);
    float3 c = float3(dot(cam0, p), dot(cam1, p), dot(cam2, p));
    float2 still = float2(uv.x * 2 - 1, 1 - uv.y * 2) - c.xy / c.z;
    float2 r = v - still;
    if (dot(r, r) <= misc.x * misc.x) return false;
    return misc.z < 0.5 || in_reach(p.xyz);
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

// a pixel of the last real picture, as a point in front of the eye, and back
float3 unproject(float2 uv, float d) {
    float2 n = float2(uv.x * 2 - 1, 1 - uv.y * 2);
    float z = (pj1.y - d * pj1.w) / (d * pj1.z - pj1.x);
    float w = pj1.z * z + pj1.w;
    return float3((n.x * w - pj0.y * z) / pj0.x, (n.y * w - pj0.w * z) / pj0.z, z);
}
float2 project(float3 p) {
    float w = pj1.z * p.z + pj1.w;
    float2 n = float2(pj0.x * p.x + pj0.y * p.z, pj0.z * p.y + pj0.w * p.z) / w;
    return float2(n.x * 0.5 + 0.5, 0.5 - n.y * 0.5);
}
// the point carried the way its hand moved and turned; between the hands the two moves are blended
float3 moved(float3 p) {
    float dl = distance(p, cl.xyz), dr = distance(p, cr.xyz);
    float w = saturate((dl - dr) / cl.w * 0.5 + 0.5);
    float3 a = float3(dot(tl0.xyz, p) + tl0.w, dot(tl1.xyz, p) + tl1.w, dot(tl2.xyz, p) + tl2.w);
    float3 b = float3(dot(tr0.xyz, p) + tr0.w, dot(tr1.xyz, p) + tr1.w, dot(tr2.xyz, p) + tr2.w);
    return lerp(a, b, w);
}
// which pixel of the last real picture lands on uv now: a few steps of "try, see where it lands, correct"
bool pasted(float2 uv, out float2 src) {
    for (int s = 0; s < 2; s++) {
        src = uv - (s == 0 ? shifts.xy : shifts.zw);
        bool ok = true; float2 miss = float2(1, 1);
        for (int k = 0; k < 4; k++) {
            if (any(src < 0) || any(src > 1)) { ok = false; break; }
            float d = tDepth.SampleLevel(sPt, src, 0).r;
            if (!near_depth(d)) { ok = false; break; }
            miss = uv - project(moved(unproject(src, d)));
            if (k < 3) src += miss;
        }
        if (ok && dot(miss, miss) < 4e-6 && hand(src)) return true;
    }
    return false;
}

float4 ps(V i) : SV_Target {
    float2 uv = i.uv, src;
    if (h2.w > 1.5) {   // the diagnostic view: near pixels show red = the game's motion, green = a still thing's, blue = nothing written
        float d = tDepth.SampleLevel(sPt, uv, 0).r;
        float4 w = tWarp.SampleLevel(sLin, uv, 0);
        if (!near_depth(d)) return w;
        float2 e = tVel.SampleLevel(sPt, uv, 0).rg;
        float2 v = e * 4.008 - 2.004; v = v * abs(v) * 0.5;
        float4 p = float4(unproject(uv, d), 1);
        float3 c = float3(dot(cam0, p), dot(cam1, p), dot(cam2, p));
        float2 still = float2(uv.x * 2 - 1, 1 - uv.y * 2) - c.xy / c.z;
        if (uv.y > 0.5) return float4(saturate(0.5 + v.x * 25), saturate(0.5 + still.x * 25), e.r < 0.01 ? 1 : 0, 1);
        return float4(saturate(length(v) * 50), saturate(length(still) * 50), e.r < 0.01 ? 1 : 0, 1);
    }
    if (pasted(uv, src)) {
        float4 c = tPrev.SampleLevel(sLin, src, 0);
        if (h2.w > 0.5) c.rgb = lerp(c.rgb, float3(0, 1, 0), 0.4);
        return c;
    }
    bool ok; float2 s = source_of(uv, ok);
    float4 c = tWarp.SampleLevel(sLin, uv, 0);
    if (ok && hand(s)) {
        float2 dir = (uv - s) / 6.0;   // the way the room went: the background beyond the copy lies that way
        for (int k = 1; k <= 12; k++) {
            float2 t = uv + dir * k;
            float2 other;
            if (!ghost(t) && !pasted(t, other)) {
                c = tWarp.SampleLevel(sLin, t, 0);
                // better than the picture from beside: what the OTHER eye really sees there this frame. The background's
                // depth is taken from just beyond the copy; the spot is carried into the other eye's picture.
                bool ok2; float2 ts = source_of(t, ok2);
                if (ok2 && all(ts >= 0) && all(ts <= 1)) {
                    float db = tDepth.SampleLevel(sPt, ts, 0).r;
                    float4 p = float4(unproject(uv, db), 1);
                    float3 q = float3(dot(oe0, p), dot(oe1, p), dot(oe2, p));
                    if (q.z > 1e-5) {
                        float2 n = q.xy / q.z; float2 ou = float2(n.x * 0.5 + 0.5, 0.5 - n.y * 0.5);
                        if (all(ou >= 0) && all(ou <= 1)) {
                            float od = tOtherDepth.SampleLevel(sPt, ou, 0).r;
                            if (near_depth(od) == near_depth(db)) c = tOther.SampleLevel(sLin, ou, 0);   // not behind that eye's own gun
                        }
                    }
                }
                break;
            }
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
constexpr UINT CB_SLOT = 512, CB_SLOTS = 64;
ID3D12Resource* g_cb = nullptr; uint8_t* g_cb_map = nullptr; UINT g_cb_next = 0;
std::atomic<float> g_against{settings::HANDPASTE_AGAINST_ROOM};
ID3D12Resource* g_target = nullptr;            // the pass draws here, then it is copied over the warp's picture
ID3D12Resource* g_keep[2] = {nullptr, nullptr};   // each eye's last real picture
D3D12_RESOURCE_DESC g_desc{};                  // what the three were made for

struct Q { float w, x, y, z; };
struct Eye {
    bool have = false;
    uint64_t frame = 0;
    ID3D12Resource* color = nullptr; uint32_t color_state = 0;
    ID3D12Resource* depth = nullptr; ID3D12Resource* velocity = nullptr;
    float w2v[16]{}, v2c[16]{};   // its real view that frame
    bool have_before = false;
    float before_w2v[16]{}, before_v2c[16]{};   // its real view the time before (what the game's velocity picture measures from)
    bool hands_ok = false;
    float hand_pos[2][3]{};       // each controller relative to the head then (metres, the head's own axes: x right, y up, -z ahead)
    Q hand_rot[2]{{1, 0, 0, 0}, {1, 0, 0, 0}};   // and its turn relative to the head
};
Eye g_eye[2];
uint64_t g_frame = 0;

void hold(ID3D12Resource*& slot, ID3D12Resource* r) {
    if (slot == r) return;
    if (r != nullptr) r->AddRef();
    if (slot != nullptr) slot->Release();
    slot = r;
}

constexpr int SRVS_PER_DRAW = 6;
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
    rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
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
    // the pass's numbers: a ring of slots in one always-mapped buffer (a slot is read by the card frames after it is written)
    D3D12_HEAP_PROPERTIES up{}; up.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bd{}; bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; bd.Width = (UINT64)CB_SLOT * CB_SLOTS; bd.Height = 1; bd.DepthOrArraySize = 1;
    bd.MipLevels = 1; bd.SampleDesc.Count = 1; bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (FAILED(g_dev->CreateCommittedResource(&up, D3D12_HEAP_FLAG_NONE, &bd, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&g_cb)))) return fail("no buffer for the pass's numbers");
    if (FAILED(g_cb->Map(0, nullptr, (void**)&g_cb_map))) return fail("the numbers' buffer did not map");
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

// ---- the hands' own motion ---------------------------------------------------------------------------------------
Q qmul(Q a, Q b) { return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z, a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x, a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w}; }
Q qconj(Q q) { return {q.w, -q.x, -q.y, -q.z}; }
void qrot(Q q, const float v[3], float out[3]) {
    const float tx = 2.0f * (q.y * v[2] - q.z * v[1]), ty = 2.0f * (q.z * v[0] - q.x * v[2]), tz = 2.0f * (q.x * v[1] - q.y * v[0]);
    out[0] = v[0] + q.w * tx + (q.y * tz - q.z * ty); out[1] = v[1] + q.w * ty + (q.z * tx - q.x * tz); out[2] = v[2] + q.w * tz + (q.x * ty - q.y * tx);
}
// each controller's place and turn relative to the head (metres, the head's own axes: x right, y up, -z ahead)
bool hands_now(float pos[2][3], Q rot[2]) {
    const auto head = API::VR::get_pose(API::VR::get_hmd_index());
    const Q qh{head.rotation.w, head.rotation.x, head.rotation.y, head.rotation.z};
    const UEVR_TrackedDeviceIndex idx[2] = {API::VR::get_left_controller_index(), API::VR::get_right_controller_index()};
    for (int i = 0; i < 2; ++i) {
        const auto p = API::VR::get_pose(idx[i]);
        if (p.position.x == 0.0f && p.position.y == 0.0f && p.position.z == 0.0f) return false;
        const float d[3] = {p.position.x - head.position.x, p.position.y - head.position.y, p.position.z - head.position.z};
        qrot(qconj(qh), d, pos[i]);
        rot[i] = qmul(qconj(qh), Q{p.rotation.w, p.rotation.x, p.rotation.y, p.rotation.z});
    }
    return true;
}
// the head's axes -> the matrices' view axes (the same, or z the other way round)
float view_z_sign(const float* v2c) { return (v2c[2 * 4 + 3] < 0.0f) ? 1.0f : -1.0f; }
// a view-space point -> its place in the picture (uv); false behind the eye
bool to_uv(const float* v2c, const float v3[3], float uv[2]) {
    const float v[4] = {v3[0], v3[1], v3[2], 1.0f};
    float clip[4]{};
    for (int r = 0; r < 4; ++r) for (int k = 0; k < 4; ++k) clip[r] += v2c[k * 4 + r] * v[k];
    if (clip[3] <= 1e-4f) return false;
    uv[0] = clip[0] / clip[3] * 0.5f + 0.5f; uv[1] = 0.5f - clip[1] / clip[3] * 0.5f;
    return true;
}
// one hand's move from then to now as a 3x4 in view space (rows; the 4th of each row is the shift); false = not to be followed
bool hand_move(const float p0[3], Q q0, const float p1[3], Q q1, float zs, float out[12]) {
    const Q d = qmul(q1, qconj(q0));
    const float angle = 2.0f * std::acos(std::fmin(1.0f, std::fabs(d.w))) * 57.29578f;
    const float ex[3] = {1, 0, 0}, ey[3] = {0, 1, 0}, ez[3] = {0, 0, 1};
    float c0[3], c1[3], c2[3], rp0[3];
    qrot(d, ex, c0); qrot(d, ey, c1); qrot(d, ez, c2); qrot(d, p0, rp0);
    const float t[3] = {p1[0] - rp0[0], p1[1] - rp0[1], p1[2] - rp0[2]};
    const float step = std::sqrt((p1[0] - p0[0]) * (p1[0] - p0[0]) + (p1[1] - p0[1]) * (p1[1] - p0[1]) + (p1[2] - p0[2]) * (p1[2] - p0[2]));
    if (angle > settings::HANDPASTE_MAX_TURN_DEG || step > settings::HANDPASTE_MAX_STEP_M) return false;
    const float s[3] = {1.0f, 1.0f, zs};
    for (int r = 0; r < 3; ++r) {
        out[r * 4 + 0] = c0[r] * s[r] * s[0]; out[r * 4 + 1] = c1[r] * s[r] * s[1]; out[r * 4 + 2] = c2[r] * s[r] * s[2];
        out[r * 4 + 3] = t[r] * s[r];
    }
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
    // each hand: how it moved and turned against the head since the eye's last real picture (the head's own turn is
    // the warp's business and already in the map above)
    const float zs = view_z_sign(last.v2c);
    float move[2][12] = {{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0}, {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0}};
    float shift[2][2]{}, then_view[2][3] = {{-0.2f, -0.2f, -0.4f * zs}, {0.2f, -0.2f, -0.4f * zs}};
    float pos_now[2][3]{}; Q rot_now[2]{};
    if (last.hands_ok) for (int i = 0; i < 2; ++i) { then_view[i][0] = last.hand_pos[i][0]; then_view[i][1] = last.hand_pos[i][1]; then_view[i][2] = last.hand_pos[i][2] * zs; }
    if (mode != 3 && last.hands_ok && hands_now(pos_now, rot_now)) {
        for (int i = 0; i < 2; ++i) {
            float m[12];
            if (!hand_move(last.hand_pos[i], last.hand_rot[i], pos_now[i], rot_now[i], zs, m)) continue;
            std::memcpy(move[i], m, sizeof(m));
            const float now_view[3] = {pos_now[i][0], pos_now[i][1], pos_now[i][2] * zs};
            float a[2], b[2];
            if (to_uv(last.v2c, then_view[i], a) && to_uv(last.v2c, now_view, b)) { shift[i][0] = b[0] - a[0]; shift[i][1] = b[1] - a[1]; }
        }
    }
    // a still thing's motion in the picture: the eye's view then -> the view the game measured from (that eye, the time before)
    float cam[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    const bool against = last.have_before && last.velocity != nullptr && g_against.load() > 0.0f;
    if (against) {
        float v2w[16]{};   // the inverse of an affine view matrix: the rotation turned over, the shift carried back
        for (int r = 0; r < 3; ++r) for (int col = 0; col < 3; ++col) v2w[col * 4 + r] = at(last.w2v, col, r);
        for (int r = 0; r < 3; ++r) v2w[3 * 4 + r] = -(at(last.w2v, 0, r) * at(last.w2v, 0, 3) + at(last.w2v, 1, r) * at(last.w2v, 1, 3) + at(last.w2v, 2, r) * at(last.w2v, 2, 3));
        v2w[15] = 1.0f;
        float a[16]{}, m[16]{};
        for (int col = 0; col < 4; ++col) for (int r = 0; r < 4; ++r) for (int k = 0; k < 4; ++k) a[col * 4 + r] += at(last.before_w2v, r, k) * v2w[col * 4 + k];
        for (int col = 0; col < 4; ++col) for (int r = 0; r < 4; ++r) for (int k = 0; k < 4; ++k) m[col * 4 + r] += at(last.before_v2c, r, k) * a[col * 4 + k];
        const int rows[3] = {0, 1, 3};
        for (int i = 0; i < 3; ++i) for (int col = 0; col < 4; ++col) cam[i * 4 + col] = at(m, rows[i], col);
    }
    const float* p = last.v2c;
    // this eye now -> the other (drawn) eye's picture now
    float oe[12]{};
    {
        const float* dv2w = c.camera + 1 * 16; const float* sw2v = c.camera + 4 * 16; const float* sv2c = c.camera + 6 * 16;
        float a[16]{}, m[16]{};
        for (int col = 0; col < 4; ++col) for (int r = 0; r < 4; ++r) for (int k = 0; k < 4; ++k) a[col * 4 + r] += at(sw2v, r, k) * dv2w[col * 4 + k];
        for (int col = 0; col < 4; ++col) for (int r = 0; r < 4; ++r) for (int k = 0; k < 4; ++k) m[col * 4 + r] += at(sv2c, r, k) * a[col * 4 + k];
        const int rows[3] = {0, 1, 3};
        for (int i = 0; i < 3; ++i) for (int col = 0; col < 4; ++col) oe[i * 4 + col] = at(m, rows[i], col);
    }
    // each controller's axes then, and that side's shoulder (the head's axes -> view axes)
    float fwd[2][3]{}, up[2][3]{}, shoulder[2][3]{};
    for (int i = 0; i < 2; ++i) {
        const float f[3] = {0, 0, -1}, u[3] = {0, 1, 0};
        qrot(last.hand_rot[i], f, fwd[i]); qrot(last.hand_rot[i], u, up[i]);
        fwd[i][2] *= zs; up[i][2] *= zs;
        shoulder[i][0] = (i == 0 ? -1.0f : 1.0f) * settings::HANDPASTE_SHOULDER[0]; shoulder[i][1] = settings::HANDPASTE_SHOULDER[1]; shoulder[i][2] = settings::HANDPASTE_SHOULDER[2] * zs;
    }
    const float reach = last.hands_ok ? 1.0f : 0.0f;
    const float constants[112] = {
        h.m[0], h.m[1], h.m[2], line, h.m[3], h.m[4], h.m[5], sign, h.m[6], h.m[7], h.m[8], mode == 2 ? 1.0f : mode == 4 ? 2.0f : 0.0f,
        move[0][0], move[0][1], move[0][2], move[0][3], move[0][4], move[0][5], move[0][6], move[0][7], move[0][8], move[0][9], move[0][10], move[0][11],
        move[1][0], move[1][1], move[1][2], move[1][3], move[1][4], move[1][5], move[1][6], move[1][7], move[1][8], move[1][9], move[1][10], move[1][11],
        at(p, 0, 0), at(p, 0, 2), at(p, 1, 1), at(p, 1, 2), at(p, 2, 2), at(p, 2, 3), at(p, 3, 2), at(p, 3, 3),
        shift[0][0], shift[0][1], shift[1][0], shift[1][1],
        then_view[0][0], then_view[0][1], then_view[0][2], settings::HANDPASTE_BLEND_M,
        then_view[1][0], then_view[1][1], then_view[1][2], 0.0f,
        cam[0], cam[1], cam[2], cam[3], cam[4], cam[5], cam[6], cam[7], cam[8], cam[9], cam[10], cam[11],
        g_against.load(), against ? 1.0f : 0.0f, reach, 0.0f,
        oe[0], oe[1], oe[2], oe[3], oe[4], oe[5], oe[6], oe[7], oe[8], oe[9], oe[10], oe[11],
        fwd[0][0], fwd[0][1], fwd[0][2], settings::HANDPASTE_WEAPON_RADIUS, up[0][0], up[0][1], up[0][2], settings::HANDPASTE_ARM_RADIUS, shoulder[0][0], shoulder[0][1], shoulder[0][2], 0.0f,
        fwd[1][0], fwd[1][1], fwd[1][2], 0.0f, up[1][0], up[1][1], up[1][2], 0.0f, shoulder[1][0], shoulder[1][1], shoulder[1][2], 0.0f};
    uint8_t* slot = g_cb_map + (size_t)g_cb_next * CB_SLOT;
    std::memcpy(slot, constants, sizeof(constants));
    const D3D12_GPU_VIRTUAL_ADDRESS cb_at = g_cb->GetGPUVirtualAddress() + (UINT64)g_cb_next * CB_SLOT;
    g_cb_next = (g_cb_next + 1) % CB_SLOTS;

    const int base = (1 - c.eye) * SRVS_PER_DRAW;
    srv(g_keep[1 - c.eye], base + 0); srv(last.depth, base + 1); srv(last.velocity != nullptr ? last.velocity : last.depth, base + 2); srv(warped, base + 3);
    srv(c.color, base + 4); srv(c.depth, base + 5);
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
    l->SetGraphicsRootConstantBufferView(1, cb_at);
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
void set_against_room(float amount) { g_against.store(amount); }

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
    now.have_before = now.have && now.frame + 2 == g_frame;
    if (now.have_before) { std::memcpy(now.before_w2v, now.w2v, sizeof(now.w2v)); std::memcpy(now.before_v2c, now.v2c, sizeof(now.v2c)); }
    now.have = true; now.frame = g_frame; now.color_state = c.color_state;
    std::memcpy(now.w2v, c.camera + 4 * 16, sizeof(now.w2v)); std::memcpy(now.v2c, c.camera + 6 * 16, sizeof(now.v2c));
    now.hands_ok = hands_now(now.hand_pos, now.hand_rot);
}

void report() {
    static bool said_fail = false;
    if (g_failed.load() && !said_fail) { said_fail = true; LOGW("%s handpaste: switched itself off: %s", TAG, g_why.load() != nullptr ? g_why.load() : "?"); }
    const int d = g_draws.exchange(0), s = g_skips.exchange(0);
    if (d > 0 || s > 0) LOGI("%s handpaste: %d made-up pictures redrawn, %d left as the warp made them", TAG, d, s);
}

} // namespace sh2vr::handpaste
