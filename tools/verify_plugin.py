"""Check the built PE, exported entry points, and SKSE's actual runtime metadata."""
import struct
import sys

import pefile


def require(condition, message):
    if not condition:
        raise SystemExit(message)


pe = pefile.PE(sys.argv[1])
require(pe.FILE_HEADER.Machine == 0x8664, "Expected an AMD64 DLL")
require(pe.FILE_HEADER.Characteristics & 0x2000, "PE is not a DLL")
exports = {entry.name: entry for entry in pe.DIRECTORY_ENTRY_EXPORT.symbols}
for name in (b"SKSEPlugin_Load", b"SKSEPlugin_Query", b"SKSEPlugin_Version"):
    require(name in exports, f"Missing export: {name!r}")
data = pe.get_data(exports[b"SKSEPlugin_Version"].address, 0x350)
require(len(data) == 0x350, "Incomplete PluginVersionData")
require(struct.unpack_from("<II", data) == (1, 0x01000020), "Incorrect plugin version")
require(data[8:264].split(b"\0")[0] == b"HeadingNaNGuard", "Incorrect plugin name")
ex_flags, flags = struct.unpack_from("<II", data, 0x304)
require(ex_flags == 1, "Expected no-structures flag and no Address Library V5 dependency")
require(flags == 0, "Unexpected Address Library or post-1.6.629 structure flags")
versions = struct.unpack_from("<16I", data, 0x30C)
require(versions == (0x01061610,) + (0,) * 15, "Expected Skyrim 1.6.353 only")
imports = sorted(entry.dll.decode() for entry in getattr(pe, "DIRECTORY_ENTRY_IMPORT", []))
require(not any(name.lower().startswith(("vcruntime", "msvcp")) for name in imports),
        "Expected the static MSVC runtime")
print("PASS: AMD64 DLL; SKSE entry points; plugin 1.0.2; runtime 1.6.353 only; static CRT")
print("Imports:", ", ".join(imports))
