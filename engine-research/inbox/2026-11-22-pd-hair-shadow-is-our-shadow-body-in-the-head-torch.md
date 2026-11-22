# The hair shadow on the wall is our own shadow body, lit by the torch that sits at the eyes (2026-11-22, the reader, files only)

From: the background reader of the 2026-11-22 live session. Nothing was launched; source, notes and the b038 wear log
(`2026-11-22-1638-tefa-wear-b038/uevr-log.txt` in the SH2 logs folder) only.
Tefa, b038, worn: "James hair shadow visible" (video: thin strands fanning from the top of the view on a prison
wall in the torch light, plus a small head-shaped blob).

## What shadows.cpp really does (read in `dev-archive/native/src/shadows.cpp`)

| piece | what is done to it | by what |
| --- | --- | --- |
| the body `CharacterMesh0`: 14 head material slots (0 1 6 8 9 11-18 22: lacrimal, teeth, eye shadow, face, facial hair, EIGHT hair slots, eyebrows) | set to the game's `MI_Invisible` | `SetMaterial` |
| the body's eyeballs (slot 10 keeps its material) | hidden by bone (`eye_l`, `eye_r`, `eye_Lt`, `eye_Rt`) | `HideBoneByName` |
| the body's own shadow | OFF in play | `SetCastShadow(false)` |
| **the shadow body** (a second skeletal mesh we add, following the body's pose) | hidden from the camera, **casts a shadow anyway** (`bCastHiddenShadow`), **holds the head's 14 real materials, hair included**, on lighting channels **0 and 1** | `make_copy()` |

So the head pieces are not "shadow off". The design is the opposite: the visible body casts nothing, and a hidden
full copy WITH the head and the hair casts the whole shadow. `[inferred-static, the source]`

The hair is eight material slots of the body mesh itself (`MI_JamesSunderland_Hair_LookDev_test` x8), not a separate
component `[measured: the log's slot list, lines 5249-5272]`. No groom or separate hair mesh is named anywhere in our
notes; nobody has listed the pawn's components for one `[not established]`.

## Why the strands show `[inferred-static; every link below is read from a file, the chain itself was not seen live]`

1. The torch's main light is on lighting channels 0 and 2; the game keeps James on channel 1 only, so his own torch
   never lights or shadows him (dossier section 8, `[verified-live 2026-11-06, n=1]`).
2. Our shadow body is put on channels **0 and 1** (`shadows.cpp`, `make_copy()`, the `SetLightingChannels(true, true,
   false)` line). The log confirms it both times a pawn was made: "casts hidden shadow 1, channels 110", and "the shadow
   body holds 14 of 14 head materials" `[measured: lines 5273-5274 and 6220-6221]`.
3. The pack's light location is "Head" (`light_type` 1 in the profile's `data/sh2_config.json`): the torch's light is
   attached to the headset with no offset (`main.lua` `attachLightToController`, hand 2). Note 2026-11-13c section 8 says
   the same: "the pack's Head light sits at the eyes".
4. So a light on channel 0 sits inside a hidden head whose hair is on channel 0 and casts shadows. The fringe hangs
   in front of the light: long strand shadows from the top of the view, and a piece of the head as a blob. With
   roomscale off the head model does not follow a lean, so leaning back or to the side puts more hair in front of
   the light.

Ruled out by the log `[measured]`: the game undoing our work in play. Body fixes are logged for the first three and
then every 50th; the first pawn (16:39:37 to 16:46:04) logged exactly two, both at its start, so nothing was put back
for six and a half minutes. The second pawn logged two at its start and a third only at the 16:54:59 cutscene (20
things: the planned switch to "head shown"). The body's own hair slots are invisible and its shadow is off.
Not ruled out: a hair piece we do not know of (see above); why this was not reported before b038 (the shadow body has
held the head's materials since the black-eyes fix of 2026-11-12) `[not established]`.

## The smallest fix

`dev-archive/native/src/shadows.cpp`, `make_copy()`: the shadow body on channel 1 only, exactly where the game keeps
the body it stands in for.

```
-    ue::call(c, L"SetLightingChannels", {true, true, false});
+    ue::call(c, L"SetLightingChannels", {settings::SHADOWS_COPY_ON_TORCH, true, false});   // 2026-11-22: off the torch's channel (the head torch sits inside the head: hair shadows on the wall)
```

plus `constexpr bool SHADOWS_COPY_ON_TORCH = false;` beside the other `SHADOWS_*` lines in `settings.h`.
Not compiled by the reader.

What it changes: the torch no longer draws any shadow of James's body or head (as in the flat game, which never
did). Every light that shadowed the game's own body still shadows the copy, now with the head: that was the point of
the copy. The weapon (channel 0 added by `weapon_channel()`) and our arms (channel 0) still shadow in the torch.
Not checked: which channels the room lights use; a room light on channel 0 only would lose the player's shadow, as
it does in the flat game.

If Tefa later carries the torch in a hand (`light_type` 2 or 3) and wants his body's shadow from it: channel 0 on the
copy only while the light is not on the head. Not needed for the fault.

## How it proves itself

- **In the log:** the existing line prints the copy's channels as read back from the game: it must say
  `channels 010` where b038 says `channels 110`. Cheap addition: in `body_fix()`, read the copy's channels each second
  and count a put-right if channel 0 is back.
- **Before any build, live through the connector (reversible, one call):** `SetLightingChannels(false, true, false)` on
  the component the log names ("the shadow body made on the pawn: ... (SkeletalMeshComponent ...)"), torch on, facing a
  near wall in a dark room: the strands and the blob go; calling it with `true, true, false` brings them back. That is
  the measure, act, re-measure for this change, and it also tests the whole chain above.
- **Logging gap worth closing while there:** body fixes 4 to 49 are silent, so the return from a cutscene (head
  hidden again) cannot be read in the log.

## Tags

- The copy is on channels 0 and 1 and holds the hair `[measured, b038 log]`.
- The torch light sits at the headset `[inferred-static: profile setting + main.lua]`.
- That this is what draws the strands `[inferred-static]`, to be confirmed by the one live call above.
