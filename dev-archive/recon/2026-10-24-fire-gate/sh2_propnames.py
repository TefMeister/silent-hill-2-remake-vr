"""Name a reflected property from its byte offset, offline: UE5's FPropertyParams ends in {uint16 ArrayDim, uint16 Offset}
right after SetterFunc/GetterFunc, and starts with a pointer to the name string. For every "01 00 <offset>" in .rdata,
check that 0x28 bytes before it is a pointer to a printable ASCII name; print the candidates. Validated on offsets whose
meaning is already known (0x118 = what IsInRangedAim reads, 0x152 = what GetCombatInputMode reads)."""
import re, struct, sys
import pefile

EXE = r"C:\Steam\steamapps\common\SILENT HILL 2\SHProto\Binaries\Win64\SHProto-Win64-Shipping.exe"
pe = pefile.PE(EXE, fast_load=True)
base = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__
rd = next(s for s in pe.sections if s.Name.startswith(b".rdata"))
rd_off, rd_va, rd_end = rd.PointerToRawData, base + rd.VirtualAddress, base + rd.VirtualAddress + rd.SizeOfRawData
blob = data[rd_off:rd_off + rd.SizeOfRawData]

def str_at(va):
    o = rd_off + (va - rd_va)
    e = data.find(b"\0", o, o + 96)
    if e < 0: return None
    s = data[o:e]
    return s.decode() if s and re.fullmatch(rb"[A-Za-z_][A-Za-z0-9_]*", s) else None

for arg in sys.argv[1:]:
    off = int(arg, 16)
    needle = b"\x01\x00" + struct.pack("<H", off)
    names = []
    for m in re.finditer(re.escape(needle), blob):
        p = m.start()
        for back in (0x28, 0x18):   # 5.1+ (with setter/getter) and 5.0 layouts
            if p < back: continue
            nameptr = struct.unpack_from("<Q", blob, p - back)[0]
            if rd_va <= nameptr < rd_end:
                n = str_at(nameptr)
                if n: names.append("%s@0x%X" % (n, rd_va + p - back))
    print("offset 0x%X: %s" % (off, ", ".join(names) or "no reflected property found"))
