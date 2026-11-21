# From /sr, 2026-11-21: six UEVR plugin traps from a shipped plugin, worth checking against ours

**For:** the native UEVR C++ plugin (the hands, the warp's hand pass, the smoothing). Library page:
`flat-to-vr-cross-engine-research/docs/engines/unreal-4-5-uevr.md`, "Plugin traps from a shipped UEVR plugin".

**Finding** `[reported 2026-11-21, read online through phunkaeg's VR Modding Playbook, chapter 18, refreshed
2026-11-16, commit b228217; the System Shock remake's own plugin page was not re-read]`. What that plugin learned in
production, in our words:

1. An object lookup that MISSES scans the whole object array (about 105 ms there); a periodic check made of misses
   gave a stutter every 6.8 s. Any lookup of ours that can miss during play (a class not loaded in some areas) is a
   suspect for a regular hitch.
2. UEVR places the hands by the view's yaw only; the head renders with the full turn. Where the pawn pitches or rolls,
   the hands drift.
3. Functions called only from Blueprint never reach UEVR's function hook.
4. VR controller input reaches only the lowest polled XInput slot.
5. Additive UI writes no alpha and vanishes in the headset; a reticle drawn without depth testing shimmers under DLSS.
6. A newer MSVC runtime in the plugin than the game's can make the first `std::mutex` lock throw.

**Why it may matter here** `[hypothesis]`: 1 is a cheap thing to rule out for any regular hitch the wearer reports;
5 bears on any reticle or HUD piece drawn with DLSS on. Nothing here is known to be wrong in our plugin.

**Source:** <https://github.com/phunkaeg/vr-modding-playbook/blob/main/docs/18-beyond-the-native-injector.md>
