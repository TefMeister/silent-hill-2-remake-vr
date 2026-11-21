# The seven save slots: where each one stands, and is it calm? (2026-11-21, the /lm reader)

From: the static reader of today's /lm session (Opus). For The Optimizer's Silent Hill 2 pack (spot tour over
`SaveGameData_0..6.sav`). I copied the slot files to my own scratch folder and read only the copies; nothing was
written in the save folder, nothing launched. No account id is printed here.

## How the file reads (new, for the dossier owner)

`[measured from the files]`
- Each slot is a small wrapper (`VASb`, class `/Script/SHProto.SHSaveGame`) with a plain header, then one zlib
  stream holding a normal GVAS (UE 5.1) body.
- The header says, in the clear: the save kind (`Autosave` or `SavePoint`), a 16-byte playthrough marker, the save
  time (UE ticks, UTC), a number (1 / 2 / 136, meaning unknown), and **the checkpoint name** (for example
  `PrisonGunRoom`).
- The body starts with James's transform (quaternion, position, scale 1,1,1 as doubles) right after the class name.
- The save point names a slot has discovered sit in the body as `ObjectID` NameProperty values (`SAVE_03`,
  `SAVE_DESK_GunRoom` ...).
- A slot standing next to a live enemy carries that enemy's AI blackboard (`LyingFigure_AI_FW_C`, `_Aggro`,
  `ai.anim.Attack.FirstEncounter` ...); a calm one does not.
- Position check: slot 2's position (310.99, 1.02, 92.3 m once reordered) is exactly the "first save" origin shift in
  dossier section 14b, and the prison slots give the armory's (409.0, -181.45, 543.56 m).
- So a pack can read the checkpoint name from the header without unpacking anything.

## Per slot

| slot | checkpoint in the header | plain label | saved (file's own time, UTC) | verdict |
|---|---|---|---|---|
| 0 | `Autosave`, `EastCity_Start_A` | South Vale East, the autosave as James first walks into town | 2026-11-13 12:06 | **probably safe** `[inferred]` |
| 1 | `Autosave`, `EastCity_Start_A` | the same spot as slot 0 (0.75 m away, facing another way) | 2026-11-14 00:03 | **probably safe** `[inferred]` |
| 2 | `Autosave`, `EastCity_FirstEncounter_A` | South Vale East, the first Lying Figure fight (Martin Street) | 2026-11-15 22:48 | **RISKY** `[measured from file strings]` + seen live |
| 3 | `SavePoint`, `PrisonGunRoom` | Toluca Prison, the armory save point | 2026-10-31 11:22 | **safe** (known, seen live) |
| 4 | `SavePoint`, `PrisonGunRoom` | Toluca Prison, the armory save point (a copy of 3) | 2026-10-31 11:23 | **safe** `[inferred]` |
| 5 | `SavePoint`, `EastCity_RoomAfterGarage_M` | South Vale East, the save point in the room after the garage, just before the first fight | 2026-11-13 12:10 | **probably safe, close to a trigger** `[hypothesis]` |
| 6 | `SavePoint`, `PrisonGunRoom` | Toluca Prison, the armory save point (an older imported copy) | 2024-11-03 23:46 | **safe** `[inferred]` |

### What shows it

- **Slot 0 and 1**
  - The header names `EastCity_Start_A`.
  - No enemy AI class or blackboard is in the body; the only AI assets are `Angela_CH` / `Human_AI`, which every
    early slot carries.
  - No save point found yet (no `SAVE_03`), so this is earlier than slot 5.
  - Whether a cutscene starts a few steps away is not in the file: unknown.
- **Slot 2: the one that killed James on 2026-11-18**
  - The header names `EastCity_FirstEncounter_A`.
  - The body holds a live Lying Figure: `LyingFigure_AI_FW_C`, `LyingFigure_CH_FW_C`, `LyingFigure_Standing_State`,
    `RunningStruggle_FirstEncounter`, `ai.anim.Attack.FirstEncounter`, `_Aggro`, `MaiAggroMeter`, `WaitUnderCar`,
    `WoodenPlank`.
  - Its position equals the "first save" origin in dossier 14b. The 2026-11-18 note (lines 144-145) says the
    Continue save killed James quickly. So this is the enemy save `[verified-live 2026-11-18 by that note, n=1]`.
  - **Keep it out of the unattended tour.**
- **Slot 5**
  - The header names `EastCity_RoomAfterGarage_M`, a real save point (`SAVE_03`).
  - Its strings are a strict subset of slot 2's: everything slot 5 has, slot 2 has too, so slot 2 is later in the
    same run. Slot 5 has none of the Lying Figure AI and no `WoodenPlank`.
  - **But James stands 8.0 m from slot 2's spot.** Standing still and only turning the head should not walk him into
    the encounter trigger, but the file cannot say where the trigger sits `[hypothesis]`. Worth a first watched visit.
- **Slots 3, 4, 6**
  - The header names `PrisonGunRoom`; the body has `SAVE_DESK_GunRoom`, `PUZZLE_Armory`, `PriOW_GunRoomKey`.
  - All three stand at the same place (within 1 cm) with the same playthrough marker. Their body strings differ by
    one prop name each, so 4 and 6 are the same calm room as 3 `[inferred]`.
  - They carry the whole game's history: hospital chase, the Pyramid Head and Flesh Lip boss fights, the
    `SAVE_1F_CHASE_STAIRCASE` save point. These are past events: the checkpoint is the armory.
  - Slot 6's save time of 2024 and the files all dated 2026-11-13 02:11 on disk say they were copied in from
    elsewhere `[inferred]`.

## For the pack

- Tour order, calmest first: **3, 4, 6** (the armory three times: one is enough for ranking, the others add
  nothing), then **0** (town start; 1 is the same spot), then **5** (watched once first).
- **Never 2.**
- The tour has three really different places: the prison armory, the town start and the room after the garage.
