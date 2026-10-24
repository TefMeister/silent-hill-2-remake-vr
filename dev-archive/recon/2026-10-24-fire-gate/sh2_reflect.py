"""Offline lookups in SHProto-Win64-Shipping.exe (no game running):
 1. RTTI type descriptors for the two classes (if the build kept RTTI)
 2. UE reflection: {name string -> native exec function} pairs, found by pointer to the name string
 3. every `mov byte [reg+0x118], imm/reg` in .text (who writes the subcomp flag at +0x118)
"""
import re, struct, sys
import pefile

EXE = r"C:\Steam\steamapps\common\SILENT HILL 2\SHProto\Binaries\Win64\SHProto-Win64-Shipping.exe"
pe = pefile.PE(EXE, fast_load=True)
base = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__
secs = {s.Name.rstrip(b"\0").decode(): s for s in pe.sections}

def va_of(off):
    for s in pe.sections:
        if s.PointerToRawData <= off < s.PointerToRawData + s.SizeOfRawData:
            return base + s.VirtualAddress + (off - s.PointerToRawData)
    return None

def off_of(va):
    rva = va - base
    for s in pe.sections:
        if s.VirtualAddress <= rva < s.VirtualAddress + s.SizeOfRawData:
            return s.PointerToRawData + (rva - s.VirtualAddress)
    return None

def section_bytes(name):
    s = secs[name]
    return s.PointerToRawData, data[s.PointerToRawData:s.PointerToRawData + s.SizeOfRawData]

# 1. RTTI
for cls in (b".?AVSHItemWeaponRanged@@", b".?AVSHRangedCmbSubcomp@@", b".?AVSHCombatComponent@@"):
    hits = [m.start() for m in re.finditer(re.escape(cls), data)]
    print("RTTI", cls.decode(), ["0x%X" % va_of(h) for h in hits] or "not in the file (RTTI stripped)")

# 2. reflection name -> exec pairs
rd_off, rd = section_bytes(".rdata")
da_off, da = section_bytes(".data")
def ptrs_to(va):
    needle = struct.pack("<Q", va)
    out = []
    for off0, blob in ((rd_off, rd), (da_off, da)):
        for m in re.finditer(re.escape(needle), blob):
            out.append(va_of(off0 + m.start()))
    return out

names = ["RequestManualFire", "IsInRangedAim", "IsFireReady", "GetCombatInputMode", "HasAnyAmmoInClip", "OnFirePerformed",
         "SetRangedAim", "SetIsInRangedAim", "RequestRangedAim", "StartRangedAim", "StopRangedAim", "ProcessSprintRequestedChangeEvent",
         "GetEquippedRangedWeapon", "SHDebug_Character_Combat_ForceRangedAim", "FindSubcomponentByClass"]
for n in names:
    nb = n.encode() + b"\0"
    hits = [m.start() for m in re.finditer(b"\0" + re.escape(nb), data)]
    for h in hits:
        sva = va_of(h + 1)
        if sva is None: continue
        for p in ptrs_to(sva):
            o = off_of(p)
            nxt = struct.unpack_from("<Q", data, o + 8)[0]
            prv = struct.unpack_from("<Q", data, o - 8)[0]
            txt = ".text" if secs[".text"].VirtualAddress + base <= nxt < secs[".text"].VirtualAddress + base + secs[".text"].Misc_VirtualSize else "-"
            print("%-42s str 0x%X  ptr at 0x%X  next qword 0x%X (%s)  prev 0x%X" % (n, sva, p, nxt, txt, prv))

# 3. writers of byte [reg+0x118]
tx_off, tx = section_bytes(".text")
disp = b"\x18\x01\x00\x00"
found = []
for m in re.finditer(re.escape(disp), tx):
    i = m.start()
    if i < 2: continue
    op, modrm = tx[i - 2], tx[i - 1]
    if 0x80 <= modrm <= 0xBF and (modrm & 7) != 4:   # mod=10 disp32, no SIB
        if op == 0xC6 and (modrm >> 3) & 7 == 0:   # mov byte [r+d32], imm8
            found.append((va_of(tx_off + i - 2), "C6 imm=%d" % tx[i + 4]))
        elif op == 0x88:                              # mov byte [r+d32], r8
            found.append((va_of(tx_off + i - 2), "88 reg"))
print("byte writes to [reg+0x118]:", len(found))
for va, k in found[:80]:
    print("  0x%X %s" % (va, k))
