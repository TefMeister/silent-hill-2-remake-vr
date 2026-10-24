"""Offline, no game running: (1) every byte write to [reg+0x118] inside the ranged combat part's code range,
(2) RIP-relative LEAs to the UTF-16 class names (the StaticClass() bodies, which hand the class constructor to
GetPrivateStaticClassBody; the constructor writes the vtable pointer)."""
import re, struct, sys
import pefile

EXE = r"C:\Steam\steamapps\common\SILENT HILL 2\SHProto\Binaries\Win64\SHProto-Win64-Shipping.exe"
pe = pefile.PE(EXE, fast_load=True)
base = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__
secs = {s.Name.rstrip(b"\0").decode(): s for s in pe.sections}
tx = secs[".text"]
tx_off, tx_va = tx.PointerToRawData, base + tx.VirtualAddress
text = data[tx_off:tx_off + tx.SizeOfRawData]

def va_of(off):
    for s in pe.sections:
        if s.PointerToRawData <= off < s.PointerToRawData + s.SizeOfRawData:
            return base + s.VirtualAddress + (off - s.PointerToRawData)
    return None

lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)
print("byte writes to [reg+0x118] in 0x%X..0x%X:" % (lo, hi))
for m in re.finditer(re.escape(b"\x18\x01\x00\x00"), text):
    i = m.start()
    if i < 2: continue
    op, modrm = text[i - 2], text[i - 1]
    va = tx_va + i - 2
    if not (lo <= va < hi): continue
    if 0x80 <= modrm <= 0xBF and (modrm & 7) != 4:
        if op == 0xC6 and (modrm >> 3) & 7 == 0: print("  0x%X  mov byte [r+0x118], %d" % (va, text[i + 4]))
        elif op == 0x88: print("  0x%X  mov byte [r+0x118], reg" % va)
        elif op == 0x80: print("  0x%X  cmp/arith byte [r+0x118], %d" % (va, text[i + 4]))
        elif op == 0xB6 and text[i - 3] == 0x0F: print("  0x%X  movzx [r+0x118]" % (va - 1))

# the UTF-16 class names and who LEAs them
for cls in sys.argv[3:]:
    u = cls.encode("utf-16-le") + b"\0\0"
    for mm in re.finditer(re.escape(u), data):
        sva = va_of(mm.start())
        if sva is None: continue
        # lea r64, [rip+disp32]: REX(48/4c) 8D modrm(00 rrr 101)
        hits = []
        for m2 in re.finditer(rb"[\x48\x4c]\x8d[\x05\x0d\x15\x1d\x25\x2d\x35\x3d]", text):
            j = m2.start()
            disp = struct.unpack_from("<i", text, j + 3)[0]
            if tx_va + j + 7 + disp == sva: hits.append(tx_va + j)
        print("UTF-16 %-24s at 0x%X  lea from: %s" % (cls, sva, ", ".join("0x%X" % h for h in hits) or "none"))
