
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
    float4 rm0; float4 rm1; float4 rm2;     // a STILL point in front of this eye at its last real picture -> in front of it now (the camera's own move, rows of a 3x4)
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

// where a STILL thing seen at uv now was in this eye's last real picture: the camera's own move, by depth (the reader, 2026-11-22)
bool still_src(float2 uv, out float2 src) {
    src = uv; float2 miss = float2(1, 1);
    for (int k = 0; k < 4; k++) {
        if (any(src < 0) || any(src > 1)) return false;
        float d = tDepth.SampleLevel(sPt, src, 0).r;
        if (abs(d * pj1.z - pj1.x) < 1e-7) d += h1.w * 1e-6;   // the sky: as far as makes no difference, but a number
        float3 p = unproject(src, d);
        float3 q = float3(dot(rm0.xyz, p) + rm0.w, dot(rm1.xyz, p) + rm1.w, dot(rm2.xyz, p) + rm2.w);
        miss = uv - project(q);
        if (k < 3) src += miss;
    }
    return dot(miss, miss) < 4e-6;
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
    if (cr.w < 0.5 && pasted(uv, src)) {
        float4 c = tPrev.SampleLevel(sLin, src, 0);
        if (h2.w > 0.5) c.rgb = lerp(c.rgb, float3(0, 1, 0), 0.4);
        return c;
    }
    bool ok; float2 s = source_of(uv, ok);
    float4 c = tWarp.SampleLevel(sLin, uv, 0);
    if (cr.w > 0.5) {
        // MODE 3 ("both"), b045. Tefa, worn, 2026-11-22: "like a ghost hand is on both sides of my real James hand,
        // barely seeable", right of the hand in the right eye, left of it in the left, flickering, whenever the hand,
        // the head or James moves; the same dotted line stands beside a sign post. A near thing hides a patch of the
        // world from the OTHER eye; for that patch the warp has no fresh picture on a made-up frame and takes this
        // eye's old picture UNMOVED (the reader, from its programs). Here: a pixel whose spot the other eye cannot see
        // (something nearer is in its way there) takes this eye's last real picture MOVED by the camera, by depth.
        // The hands themselves are left as the warp made them: they are the smooth ones (ours, pasted, were "framey").
        if (!ok || any(s < 0) || any(s > 1)) return c;
        float d0 = tDepth.SampleLevel(sPt, s, 0).r;
        if (abs(d0 * pj1.z - pj1.x) < 1e-7) d0 += h1.w * 1e-6;
        float3 p0 = unproject(s, d0);
        float4 pn = float4(dot(rm0.xyz, p0) + rm0.w, dot(rm1.xyz, p0) + rm1.w, dot(rm2.xyz, p0) + rm2.w, 1);
        float3 qo = float3(dot(oe0, pn), dot(oe1, pn), dot(oe2, pn));
        if (qo.z <= 1e-5) return c;
        float2 on = qo.xy / qo.z; float2 ou = float2(on.x * 0.5 + 0.5, 0.5 - on.y * 0.5);
        if (any(ou < 0) || any(ou > 1)) return c;
        // the patch a little wider than it measures (its rim stayed as a dotted outline beside the hand: Tefa's
        // screenshots of b045): the nearest thing the other eye sees at the spot or a few pixels around it
        float tw, th; tWarp.GetDimensions(tw, th);
        float2 rr = float2(max(3.0, tw * 0.0015) / tw, max(3.0, tw * 0.0015) / th);   // 4 pixels at the headset's 2688
        float wo = 1e30;
        const float2 o5[5] = { float2(0, 0), float2(1, 0), float2(-1, 0), float2(0, 1), float2(0, -1) };
        for (int k5 = 0; k5 < 5; k5++) {
            float od = tOtherDepth.SampleLevel(sPt, ou + o5[k5] * rr, 0).r;
            float zo = (pj1.y - od * pj1.w) / (od * pj1.z - pj1.x);
            wo = min(wo, pj1.z * zo + pj1.w);
        }
        if (wo > qo.z * misc.w) {
            // not hidden from the other eye. What is left here are single dark dots along the edges of near things
            // (holes of the warp's throw): a pixel much darker than both its left and right neighbours takes their mean.
            float3 l1 = tWarp.SampleLevel(sPt, uv - float2(1.0 / tw, 0), 0).rgb, r1 = tWarp.SampleLevel(sPt, uv + float2(1.0 / tw, 0), 0).rgb;
            float lc = dot(c.rgb, float3(0.3, 0.6, 0.1)), ll = dot(l1, float3(0.3, 0.6, 0.1)), lr = dot(r1, float3(0.3, 0.6, 0.1));
            if (lc < min(ll, lr) * 0.6) { c.rgb = (l1 + r1) * 0.5; if (h2.w > 0.5) c.rgb = float3(1, 1, 0); return c; }
            // ... or than both the one above and the one below (a dotted line lying flat: Tefa's ring across the palm)
            float3 u1 = tWarp.SampleLevel(sPt, uv - float2(0, 1.0 / th), 0).rgb, d1 = tWarp.SampleLevel(sPt, uv + float2(0, 1.0 / th), 0).rgb;
            float lu = dot(u1, float3(0.3, 0.6, 0.1)), ld = dot(d1, float3(0.3, 0.6, 0.1));
            if (lc < min(lu, ld) * 0.6) { c.rgb = (u1 + d1) * 0.5; if (h2.w > 0.5) c.rgb = float3(1, 1, 0); }
            return c;
        }
        float2 ss;
        if (!still_src(uv, ss)) { if (h2.w > 0.5) c.rgb = lerp(c.rgb, float3(1, 0, 1), 0.5); return c; }
        if (hand(ss)) { if (h2.w > 0.5) c.rgb = lerp(c.rgb, float3(0, 0, 1), 0.5); return c; }   // the old picture had the moving hand there: no better picture known
        c = tPrev.SampleLevel(sLin, ss, 0);
        if (h2.w > 0.5) c.rgb = lerp(c.rgb, float3(1, 0, 0), 0.5);
        return c;
    }
    if (ok && hand(s)) {
        // where to look for the background beyond the warp's leftover copy. The way the room went (uv - s) works
        // while the head turns; with the head still it is zero and the search never left the copy, so the old gun
        // stayed (Tefa 2026-11-19, headset: "a trail of guns", tinted red / orange). Then: away from where each
        // hand went, then the sides; each in steps of at least misc.w of the picture.
        float2 dirs[6] = { uv - s, -shifts.xy, -shifts.zw, float2(1, 0), float2(-1, 0), float2(0, -1) };
        bool done = false;
        [loop] for (int di = 0; di < 6 && !done; di++) {
          float len = length(dirs[di]);
          if (len < 1e-5) continue;
          float2 dir = dirs[di] / len * max(len / 6.0, misc.w);
          [loop] for (int k = 1; k <= 16; k++) {
            float2 t = uv + dir * k;
            if (any(t < 0) || any(t > 1)) break;
            float2 other;
            if (!ghost(t) && !pasted(t, other)) {
                done = true;
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
        }
        if (h2.w > 0.5) c.rgb = lerp(c.rgb, done ? float3(1, 0, 0) : float3(1, 0, 1), 0.4);   // magenta: no background found
    }
    return c;
}
