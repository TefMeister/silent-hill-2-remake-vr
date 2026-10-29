"""sim_ctl.py hand,x,y,z[,yaw,pitch,trigger,buttonA] [...]: move the OpenXR Simulator's controllers while a game runs.
hand 0 = left, 1 = right; metres, y from HEAD height (the simulator's quirk); radians; trigger 0..1; buttonA 0/1.
Writes the simulator's controller_pose_command.json (it deletes the file once read). Several poses in one call are
applied in order, `wait:<s>` between them. Used 2026-10-29 to press the left trigger alone for the button layer."""
import json, os, sys, time

CMD = os.path.join(os.environ["LOCALAPPDATA"], "OpenXR-Simulator", "controller_pose_command.json")
WAIT_TRIES, WAIT_S = 40, 0.05   # the simulator reads the file within about 50 ms while a game runs

for a in sys.argv[1:]:
    if a.startswith("wait:"):
        time.sleep(float(a[5:]))
        continue
    v = [float(x) for x in a.split(",")] + [0.0, 0.0, 0.0, 0.0]
    with open(CMD + ".tmp", "w") as f:
        json.dump({"hand": int(v[0]), "posX": v[1], "posY": v[2], "posZ": v[3], "yaw": v[4], "pitch": v[5],
                   "trigger": v[6], "buttonA": v[7] >= 0.5}, f)
    os.replace(CMD + ".tmp", CMD)
    for _ in range(WAIT_TRIES):
        if not os.path.exists(CMD):
            break
        time.sleep(WAIT_S)
    print("controller", a, flush=True)
