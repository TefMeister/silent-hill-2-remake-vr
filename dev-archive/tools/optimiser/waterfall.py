"""waterfall.py -- where the frame goes: switch things off ONE AFTER ANOTHER (each stays off) and read the frame time
after every step (the optimiser, 2026-11-10).

    python waterfall.py "label: name=value name=value" "label2: ..." ...

Prints the frame time at the start and after each step, with the drop that step gave, then puts every setting back and
reads the frame time once more (it should match the start). Uses the plugin's settings file (cvar_ab.py). The order
matters: a later step only shows what is left of it after the earlier ones.
"""
import sys

import cvar_ab as C


def main():
    start = C.frame_ms()
    print(f"{'as it is':44s} {start:6.2f} ms", flush=True)
    old = {}
    last = start
    for spec in sys.argv[1:]:
        label, _, rest = spec.partition(":")
        settings = dict(p.split("=", 1) for p in rest.split())
        before, missing = C.apply(settings)
        for k, v in before.items():
            old.setdefault(k, v)
        now = C.frame_ms()
        note = f"   (not in this build: {' '.join(missing)})" if missing else ""
        print(f"{'- ' + label.strip():44s} {now:6.2f} ms   {now - last:+6.2f}{note}", flush=True)
        last = now
    C.apply(old)
    end = C.frame_ms()
    print(f"{'everything back':44s} {end:6.2f} ms   (start was {start:.2f})", flush=True)
    print(f"left at the bottom: {last:.2f} ms of {start:.2f} ({last / start * 100:.0f} %)")
    try:
        import os
        os.remove(C.FILE)
    except OSError:
        pass


if __name__ == "__main__":
    main()
