# 2026-11-06 (00:55-01:10): headset wear of 0.1.1 - the black screen, 30 fps at max settings, the loose left hand (home PC, Opus, Quest 3 / Virtual Desktop)

Tefa in the headset (Virtual Desktop "Ultra", 72 Hz, the game near max settings on purpose: "get it to work well and
stable like this, so lowering it then would give a really smooth gameplay"). Connector dll (uevr_mcp) OUT of the profile.
Build: 0.1.1 package launcher (`play --no-install`), sh2vr.dll as installed (deployed.sh: all 5 files match).

## 1. Black picture in the headset: the "Early" sync setting

- Run 1 (00:55, `VR_SynchronizationMode=0` = Early, set by the 11-05 optimiser): the game ran, UEVR loaded, but EVERY
  frame was refused by the runtime: `xrEndFrame failed: XR_ERROR_SWAPCHAIN_RECT_INVALID` from the first frame
  (00:55:59.705), then `xrBeginFrame failed: XR_FRAME_DISCARDED`, ~7,900 errors. Tefa: "picture is black in the headset".
  `[measured 2026-11-06, n=1]`. Its 9.7 ms "frame time" was a frame that never reached the headset: not a speed result.
- Run 2 (00:59, the ONLY change `VR_SynchronizationMode=2` = Very Late, as on every earlier wear): 0 errors, picture
  back `[verified-live 2026-11-06, n=1]`.
- The 10-23 profile backup (same UEVR AFW beta.5, same 5376-wide swapchain) also had 0 errors: the setting is the
  difference, not UEVR or the eye size. The OpenXR Simulator accepts Early without complaint, so the 11-05 "Early beats
  Very Late by 1.8 ms" result is simulator-only and does not carry to Virtual Desktop.
- FIXED: the profile and `mod/` config.txt back to 2. ⚠️ The packed 0.1.0 / 0.1.1 zips still carry Early: the next pack
  must be made from the fixed `mod/` (both zips would show a black headset to anyone).
- Logs kept: `D:\SH2 builds\_log-2026-11-06-0055-black-headset-sync0.txt`, `..._0059-headset-sync2.txt` (local, game logs).

## 2. Frame rate at max settings (Very Late, connector out)

- Armory spot, standing: ~33.5 ms a frame (~30 fps), graphics card 99 % `[measured 2026-11-06, n=1]`. GPU-bound at
  2688 x 2880 per eye (Virtual Desktop Ultra) with the game near max: what Tefa expects at these settings.
- 21 Sacraments on vs off (F8, Tefa, standing still): no change (33.4-34.5 ms both sides) `[measured 2026-11-06, n=1]`.
  It is not the 10-31 halving. (Its loader `LoadPlugins=1`, so it was loaded.)
- While TWO-HANDING a long gun the frame time rose to ~50 ms (49-53 ms, 01:06:48-01:07:03) and fell back to ~36 ms when
  let go; our own cost stayed 0.4-0.6 ms (`cost:` lines). Something the two-hand pose makes the GAME do costs ~15 ms
  `[measured 2026-11-06, n=1, cause: hypothesis]`.
- 72 vs 90 Hz: settled by Tefa, stays 72 (already on it; 90 would be worse).

## 3. The two-handed shotgun and rifle (Tefa's words and two screenshots)

Tefa: "the hands don't sit right and that is expected, need to pose them or use the games hands somehow. pump shotgun
still pumps the handle by itself. what does bugger me is the left hand not really being on the front of the gun, rather
like stuck on to it and it does visibly move and clip through the weapon, not much but enough for me to want to see if
it is possible to actually get it to sit there all solid and immersive".

- Screenshots: shotgun: the left hand open, palm up, fingers spread, beside the receiver, not wrapped round the pump.
  Rifle: the left hand under the gun, palm up, fingers open.
- Log: "the drawn left hand is X cm from the game's grip spot": rifle 7.0 / 0.4 / 4.3 cm, shotgun 0.1 / 0.0 cm.
  Ghost Hand (01:07): the game's left hand wanders up to 4.8 cm, jumped 21 times while the controller was calm.
- The finger pose: "the left fingers in the game's grip" logged, yet the screenshots show open fingers: the pose is
  not what is drawn (no game fingers captured yet, so the pack's pose is used; it may be overwritten later in the frame).

## 4. NEXT: why the left hand floats (the step order, PROTOCOL §11) - Tefa: "please write the next thing you are thinking"

The one line first: the gun is placed at step LATE (UEVR's view callback, `twohand::on_view`, after UEVR has put the
held gun on the controller), the left hand's target is read there from the gun's socket, but the arm IK that moves the
hand runs at step EARLY of the NEXT frame (`ik::tick` in `on_pre_engine_tick`). So the hand is solved against where the
gun WAS one frame ago, then UEVR moves the gun again: the hand trails the gun by a frame and slides on it whenever the
gun moves, more at 30 fps (each frame is longer) and more on the rifle (longer lever) `[inferred-static 2026-11-06]`.
That fits "stuck on and visibly moves/clips" and the 0-7 cm spread.

What to try, in order (no game needed to build; one headset or simulator run to prove):
1. Store the hand's target in the GUN's own frame (offset from the gun, not a world position), and turn it into world
   space at IK time from the gun's CURRENT transform, so a moved gun carries the hand with it.
   Better still: put the left hand bone on the gun in the same view callback, after the gun write (the last step).
2. Make the proof self-measuring: log the hand-to-socket distance AFTER the solve as drawn (Ghost Hand's number), not
   at the moment we set the target, so the "0.0 cm" line can no longer read good while the hand drifts.
3. Then the closed fingers: find what overwrites the left finger pose after `frontgrip::tick`, or capture the game's
   own grip fingers (F2 while the body hand sits on Holder_L) so the hand wraps the pump.
4. Separately: the shotgun's pump still moves by itself (the pack's pump animation): the board row for it stays.
5. The ~15 ms extra while two-handing: compare the game's stat (stat unit / GPU) two-handed vs not.

`MODEL: FABLE` for 1-2 (the frame order between UEVR's callbacks and the game's animation is engine behaviour not
written down anywhere, and earlier attempts at the hand on the gun have resisted several rounds), `OPUS` for 3-5.

## Not established
- That the one-frame order is the WHOLE cause (the game's own left-hand IK may also pull on the hand; check after 1).
- The two-handed ~15 ms. Whether Early would work on a different headset runtime (only Virtual Desktop tried).
