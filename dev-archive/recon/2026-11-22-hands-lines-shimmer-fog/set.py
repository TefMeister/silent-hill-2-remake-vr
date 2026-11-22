"""set.py key=value ...: UEVR settings through the connector; cvar:name=value for engine values"""
import sys, json, urllib.request
def post(path, body):
    r = urllib.request.Request("http://127.0.0.1:8899" + path, json.dumps(body).encode(), {"Content-Type": "application/json"})
    return urllib.request.urlopen(r, timeout=5).read().decode()[:160]
for a in sys.argv[1:]:
    k, v = a.split("=", 1)
    if k.startswith("cvar:"): print(k, v, post("/api/console/cvar", {"name": k[5:], "value": v}))
    else: print(k, v, post("/api/vr/settings", {"key": k, "value": v}))
