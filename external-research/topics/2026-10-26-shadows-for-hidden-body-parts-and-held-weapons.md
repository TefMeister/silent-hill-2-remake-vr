# Shadows for a hidden head and for held weapons: the standard Unreal recipe

**Date:** 2026-10-26 (`/gr`, FULL pass) · **Status:** 🆕 new · **For:** the `[VR USER]` shadows row (weapons cast no
shadow in the flashlight; the player's shadow loses its head because `HideBoneByName("neck_02_bn")` also hides it from
shadows). That row is tagged `MODEL: FABLE`; this is the public groundwork, not a solution.

## What is publicly documented

- **A hidden primitive can still cast a shadow.** Unreal's primitive components carry `bCastHiddenShadow`: *if true,
  the primitive casts shadows even when hidden*; it only applies when `CastShadow` is on `[reported]` (Epic API docs).
- **The usual first-person body recipe** on Epic's forums: keep a second copy of the body mesh that the owner cannot see
  (`Owner No See`) with `Cast Shadow` and `Cast Hidden Shadow` on, and turn `Cast Shadow` off on the visible
  first-person mesh. The shadow then comes from the full body, head included, while the camera never sees the head
  `[reported]`.
- **Why the current route loses the head's shadow:** hiding a bone, or making a component invisible, also removes it
  from shadow and reflection passes, as several forum threads note `[reported]`. That is the board's observation,
  explained.
- **`bRenderInMainPass`** (`SetRenderInMainPass`) drops a primitive from the base pass, depth and velocity rendering;
  Epic documents it next to a separate depth-pass flag. Whether it also keeps the shadow-depth pass is NOT documented in
  anything found `[hypothesis]`, so it is a live test, not a known route.

## Applying it here (all `[hypothesis]`, to test)

1. **Head:** instead of hiding the neck bone on the visible body, put a shadow-only copy of the body beside it (same
   animation, following the main mesh's pose), invisible to the camera, `bCastHiddenShadow` on; keep the head hidden only
   on the visible mesh. Through UEVR this is reflection work the plugin already does (component properties by name).
2. **Weapons in the flashlight:** first read, live, what the held weapon's mesh component has for `CastShadow`,
   `bCastDynamicShadow` and `bCastHiddenShadow`, and what the flashlight's light has for casting shadows and its
   lighting channels. A first-person weapon is often authored with dynamic shadows off; one flag may be the whole fix.
3. The board also notes hidden arms still block the flashlight in the Lua pack: the same flags explain that from the
   other side (a hidden mesh still casting), so the read in step 2 should include the arm meshes.

## Sources

- Epic Games, Unreal API: `bCastHiddenShadow` —
  <https://docs.unrealengine.com/4.27/en-US/API/Runtime/Landscape/ALandscapeProxy/bCastHiddenShadow/index.html>
- Epic Games, Unreal API: `UPrimitiveComponent::SetRenderInMainPass` —
  <https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/Components/UPrimitiveComponent/SetRenderInMainPass/index.html>
- Epic Games, Unreal API: `FPrimitiveViewRelevance` —
  <https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/FPrimitiveViewRelevance>
- Unreal Engine forums, "How do you setup a First Person game but still use the third person mesh for shadowing?" —
  <https://forums.unrealengine.com/t/how-do-you-setup-a-first-person-game-but-still-use-the-third-person-mesh-for-shadowing/285114>
- Unreal Engine AnswerHub, "So how can I make character's model hidden when a button is pressed, but still cast a
  shadow?" — <https://answers.unrealengine.com/questions/72741/view.html>

⚠️ The API pages found are UE4.26/4.27; this game is UE5.1. The flags exist in UE5's `PrimitiveComponent`, but check
each by reflection in the running game before relying on it.
