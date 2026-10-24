"""From InternalConstructor<USHItemWeaponRanged> (0x14297FBB0, handed to GetPrivateStaticClassBody by StaticClass at
0x14297EAE0) to the class's vtable, then the virtual slots the fire path calls: 0x718 (IsFireReady), 0x750 (fire),
0x768 (called when the aim starts). Offline, capstone + pefile, no game running."""
import struct, subprocess, sys
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

EXE = r"C:\Steam\steamapps\common\SILENT HILL 2\SHProto\Binaries\Win64\SHProto-Win64-Shipping.exe"
TOOL = r"C:\Users\TD3KX\github-backups-pd\flat-to-vr-RE-toolkit\tools\static-disasm.py"
pe = pefile.PE(EXE, fast_load=True)
base = pe.OPTIONAL_HEADER.ImageBase
data = pe.__data__

def off_of(va):
    rva = va - base
    for s in pe.sections:
        if s.VirtualAddress <= rva < s.VirtualAddress + s.SizeOfRawData:
            return s.PointerToRawData + (rva - s.VirtualAddress)
    return None

def q(va): return struct.unpack_from("<Q", data, off_of(va))[0]

md = Cs(CS_ARCH_X86, CS_MODE_64)
md.detail = True
def insns(va, n=80):
    o = off_of(va)
    return list(md.disasm(data[o:o + n * 15], va, n))

ctor_thunk = int(sys.argv[1], 16)
print("InternalConstructor:")
target = None
for i in insns(ctor_thunk, 20):
    print("  0x%X  %s %s" % (i.address, i.mnemonic, i.op_str))
    if i.mnemonic in ("call", "jmp") and i.op_str.startswith("0x"):
        target = int(i.op_str, 16); break
print("constructor at 0x%X" % target)
vt = None
last_lea = None
for i in insns(target, 120):
    if i.mnemonic == "lea" and "rip" in i.op_str:
        d = i.operands[1].mem.disp
        last_lea = (i.operands[0].reg, i.address + i.size + d)
    if i.mnemonic == "mov" and i.operands[0].type == 3 and i.operands[0].mem.disp == 0 and i.operands[1].type == 1 and last_lea and i.operands[1].reg == last_lea[0]:
        vt = last_lea[1]
        print("  vtable store at 0x%X: [%s] = 0x%X" % (i.address, i.reg_name(i.operands[0].mem.base), vt))
        break
if vt is None:
    for i in insns(target, 60): print("  0x%X  %s %s" % (i.address, i.mnemonic, i.op_str))
    sys.exit("no vtable store found")
for slot in (0x718, 0x750, 0x768, 0x700, 0x3f0):
    print("slot 0x%X -> 0x%X" % (slot, q(vt + slot)))
for slot, name in ((0x718, "isfireready"), (0x750, "fire")):
    fn = q(vt + slot)
    out = subprocess.run([sys.executable, TOOL, EXE, "func", "0x%X" % fn, "--x64", "--count", "320"], capture_output=True, text=True).stdout
    open("slot_%s.txt" % name, "w").write(out)
    print("wrote slot_%s.txt (%d lines)" % (name, out.count("\n")))
