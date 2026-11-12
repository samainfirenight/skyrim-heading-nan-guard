# Skyrim 1.6.353 verification

The compatibility build was checked against the user's supplied executable, rather
than assuming the 1.6.1170 address was valid on an older runtime.

| Item | Verified value |
| --- | --- |
| PE file version | 1.6.353.0 |
| Machine | AMD64 |
| Executable SHA-256 | `b99ade6e3673d18f32dbb33e50da19586300f8241af959836eac8a0145c3283a` |
| Heading helper RVA | `0xC7A230` |
| File offset | `0xC79630` |
| Function length through RET | 158 bytes |

Disassembly confirmed the two float inputs, `atan(x/y)`, the sign adjustment,
the special `y == 0` branches, and the two wrapping loops. The constants read
from the executable have exactly the same float values as the upstream source:
pi `0x40490FDB`, two pi `0x40C90FDB`, half pi `0x3FC90FDB`, and three half pi
`0x4096CBE4`. The lower wrap loop uses COMISS/JB, whose unordered result can
keep looping for NaN.

The patch compares all 158 original instruction bytes before writing the branch.
It rejects an unexpected function or an existing modification. The runtime is
checked both in the exported SKSE metadata and in the loading entry point before
accessing the patch address. The RVA is relative to the loaded module, so ASLR
is supported. Address Library is not used by this dedicated runtime build.

The upstream heading calculation and invalid-input logging are unchanged. The
build explicitly uses precise floating-point semantics to preserve NaN checks.
This plugin does not access game object layouts; the original post-1.6.629
structure flag is removed.

GitHub Actions builds x64 Release with a static MSVC runtime and the pinned
CommonLibSSE submodule/vcpkg registry. It checks the resulting DLL's exports,
runtime metadata, and CRT imports before uploading the installation archive.
The executable itself is never committed or uploaded by the workflow.

Binary inspection and a successful build do not constitute an in-game test.
On first launch, check `HeadingNaNGuard.log` for:

```
heading helper at SkyrimSE.exe+C7A230 replaced (NaN-safe)
```
