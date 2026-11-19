"""saveswap.py: start an unattended run from a chosen save slot, and put every save back afterwards.

Why: the game's Continue loads the newest slot, and no game function loads a chosen one (KNOWN.md, 2026-11-18). The
newest slot can stand among enemies (James dies within a minute); the prison armory slot is calm and has every weapon.
Done by hand twice (2026-11-18, 2026-11-19), so it is a tool now.

    python saveswap.py start [slot=3]   game CLOSED: copies the whole save folder away, then puts slot N's file over
                                         the newest slot's file (Continue then loads slot N's place)
    python saveswap.py end              game CLOSED: puts every file back from the copy and proves it by hash
    python saveswap.py status           says whether a swap is in force

The copy lives beside the save folder's parent (Saved/sh2vr-saveswap/), never in the repo. `end` removes files the run
added and restores the ones it changed; it refuses to run twice. Nothing here reads or prints the Steam id.
"""
import glob, hashlib, json, os, shutil, subprocess, sys, time

ROOT = os.path.join(os.environ["LOCALAPPDATA"], "SilentHill2", "Saved")
KEEP = os.path.join(ROOT, "sh2vr-saveswap")
MARK = os.path.join(KEEP, "swap.json")
GAME_EXE = "SHProto-Win64-Shipping.exe"


def save_dir():
    dirs = [d for d in glob.glob(os.path.join(ROOT, "SaveGames", "*")) if os.path.isdir(d) and glob.glob(os.path.join(d, "SaveGameData_*.sav"))]
    if len(dirs) != 1:
        sys.exit("saveswap: expected one save folder with slots, found %d" % len(dirs))
    return dirs[0]


def sha(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


def hashes(d):
    return {n: sha(os.path.join(d, n)) for n in sorted(os.listdir(d)) if os.path.isfile(os.path.join(d, n))}


def game_running():
    out = subprocess.run(["tasklist", "/FI", "IMAGENAME eq " + GAME_EXE], capture_output=True, text=True).stdout
    return GAME_EXE.lower() in out.lower()


def start(slot):
    if os.path.exists(MARK):
        sys.exit("saveswap: a swap is already in force (run `end` first)")
    if game_running():
        sys.exit("saveswap: the game is running; close it first")
    d = save_dir()
    src = os.path.join(d, "SaveGameData_%d.sav" % slot)
    if not os.path.exists(src):
        sys.exit("saveswap: slot %d has no file" % slot)
    slots = glob.glob(os.path.join(d, "SaveGameData_*.sav"))
    newest = max(slots, key=os.path.getmtime)
    copy = os.path.join(KEEP, "saves-" + time.strftime("%Y%m%d-%H%M%S"))
    os.makedirs(KEEP, exist_ok=True)
    shutil.copytree(d, copy)
    before = hashes(d)
    if hashes(copy) != before:
        sys.exit("saveswap: the copy does not match the save folder; nothing changed")
    with open(MARK, "w") as f:
        json.dump({"copy": copy, "before": before, "slot": slot, "over": os.path.basename(newest)}, f, indent=1)
    if os.path.abspath(src) != os.path.abspath(newest):
        shutil.copyfile(src, newest)   # a fresh modified time: it stays the newest
    print("saveswap: slot %d now sits over %s (the newest); %d files copied away. Run `end` after the game closes."
          % (slot, os.path.basename(newest), len(before)))


def end():
    if not os.path.exists(MARK):
        sys.exit("saveswap: no swap in force")
    if game_running():
        sys.exit("saveswap: the game is running; close it first")
    with open(MARK) as f:
        m = json.load(f)
    d = save_dir()
    for n in os.listdir(d):
        p = os.path.join(d, n)
        if os.path.isfile(p) and n not in m["before"]:
            os.remove(p)
    for n in m["before"]:
        shutil.copy2(os.path.join(m["copy"], n), os.path.join(d, n))
    after = hashes(d)
    if after != m["before"]:
        bad = [n for n in m["before"] if after.get(n) != m["before"][n]]
        sys.exit("saveswap: NOT the same after putting back: %s (the copy is kept)" % ", ".join(bad))
    os.remove(MARK)
    print("saveswap: every save put back, %d files the same by hash (the copy is kept)" % len(after))


def main():
    cmd = sys.argv[1] if len(sys.argv) > 1 else "status"
    if cmd == "start":
        slot = 3
        for a in sys.argv[2:]:
            if a.startswith("slot="):
                slot = int(a[5:])
        start(slot)
    elif cmd == "end":
        end()
    elif cmd == "status":
        print("saveswap: a swap is in force" if os.path.exists(MARK) else "saveswap: no swap in force")
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
