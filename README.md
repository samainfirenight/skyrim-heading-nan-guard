# Heading NaN Guard

**Version 1.0.2: dedicated Skyrim SE/AE 1.6.353.0 build.** This fork verifies the
original 158-byte heading helper at `SkyrimSE.exe+C7A230` before patching it.
Other game versions are rejected. See [verification details](docs/runtime-1.6.353.md).

Stops a kind of **permanent freeze**: the game hangs with no crash and no crash log, and never recovers. It usually happens in
the open world, a few seconds after NPCs or animals load in.

## What it does

The game turns an actor's movement direction into a facing angle with a small engine function. If a mod hands that function
an invalid direction (NaN, "not a number"), the function loops forever. The game waits for it every frame and freezes.

This plugin replaces that function with an identical one that **returns a safe value instead of looping**. The affected NPC
faces north for one frame, and the game carries on.

Vanilla rarely produces such a direction, but **mods that change NPC movement or pathing can**. The freeze was tracked down
with a full memory dump in a heavily modded game, where a pathing mod produced a bad direction for an animal every few minutes.

## Requirements

- Skyrim Special Edition executable **1.6.353.0**.
- [SKSE64](https://skse.silverlock.org/) **2.1.5**, the build for 1.6.353.
- This dedicated build does **not** require Address Library.

## Installation

Download the ZIP from the successful **Build DLL for Skyrim 1.6.353** workflow's
Artifacts section. If GitHub wraps it in an artifact ZIP, extract that outer ZIP
first. Open `HeadingNaNGuard-1.0.2-Skyrim-1.6.353.zip` and copy its `SKSE` folder
into your game's `Data` folder. The installed file is:

```
Data/SKSE/Plugins/HeadingNaNGuard.dll
```

Replace any earlier copy of that DLL. On launch, check
`Documents/My Games/Skyrim Special Edition/SKSE/HeadingNaNGuard.log` for
`heading helper at SkyrimSE.exe+C7A230 replaced (NaN-safe)`.

## Compatibility

- **Game 1.6.353.0 only.** The target function and constants were verified in the
  supplied executable. Compilation and binary checks are automated; in-game
  operation still needs confirmation on the user's installation.
- No ESP, no scripts, nothing stored in your save. You can add or remove it at any time, mid-playthrough.
- No known conflicts. If another SKSE plugin has already patched the same engine function, this one leaves it alone and
  says so in its log.

## Finding the mod that causes it

Each prevented freeze is written to a log, with the address of the game code that produced the bad direction:

```
Documents/My Games/Skyrim Special Edition/SKSE/HeadingNaNGuard.log
[20:02:10.429] [warning] bad vector (-nan, 4.0177e-41) from SkyrimSE.exe+11B0D00 (hit 1)
```

It logs the first 50 hits, then every 1000th. An empty log (only the start-up lines) means nothing in your game produced a bad
direction. If you see hits, post the log in the comments: the address tells which part of the game, and usually which mod, is
involved.

## Credits and permissions

By WhiteLionNeren and Claude (Anthropic).

Built with [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR) by alandtse, powerof3 and contributors
(GPL-3.0-or-later), and with [spdlog](https://github.com/gabime/spdlog), [fmt](https://github.com/fmtlib/fmt) (both MIT) and
[rapidcsv](https://github.com/d99kris/rapidcsv) (BSD-3-Clause).
Requires [SKSE64](https://skse.silverlock.org/) by the SKSE team. The original
multi-version approach used Address Library by meh321; this build uses a verified
1.6.353 RVA instead.

Source code: https://github.com/WhiteLionNeren/skyrim-heading-nan-guard (public domain, Unlicense). You may reupload, modify,
translate or include this mod in a collection without asking. The DLL itself is GPL-3.0-or-later, because it is built with
CommonLibSSE-NG; the archive includes the license.

<!-- end of the Nexus description: everything below is GitHub only -->

## Building from source

The source is `src/main.cpp`; it uses [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR) v8.3.0 as a git submodule.

```bash
git clone --recursive https://github.com/WhiteLionNeren/skyrim-heading-nan-guard
```

For this fork, clone `https://github.com/samainfirenight/skyrim-heading-nan-guard`
instead. The GitHub workflow runs on pushes to `main`, pull requests, or manually
through **Actions > Build DLL for Skyrim 1.6.353 > Run workflow**. It uploads an
installation ZIP after checking the compiled DLL's SKSE metadata.

**Windows (MSVC):** configure with vcpkg's toolchain file and the static triplet,
then build. The workflow pins vcpkg to the registry baseline in `vcpkg.json`:

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static
cmake --build build --config Release
```

**Linux (cross-compile with clang-cl):** `linux/build.sh`. The script's header lists what it needs: LLVM 20, an
[xwin](https://github.com/Jake-Shadle/xwin) MSVC sysroot, vcpkg, and Wine + llvm-mingw for DirectXTK's shader step.

## Technical details

The 1.6.353 engine helper is at **`SkyrimSE.exe+C7A230`**, verified directly in the
executable (the upstream 1.6.1170 version used Address Library ID 70172). It computes `atan(x / y)`, adds π when
`y < 0`, then wraps the result into [0, 2π] with `while (angle < 0) angle += 2π;`. Every comparison with NaN is unordered, so
that loop never ends. The helper runs on the AI/movement job threads (actor movement → path follower update, ID 90711), and
the main thread waits for that job every frame (`WaitForJobTask`, ID 69516).

The replacement uses the same maths and constants (`y == 0`: π/2 when `x > 0`, else 3π/2), returns 0 for a NaN or infinite
input, and is installed with a 5-byte branch after checking all 158 bytes of the
1.6.353 function. It adds no Papyrus scripts, timers, or recurring scans.

## License

**This repository's own source is public domain** ([Unlicense](LICENSE)).

**The compiled DLL is GPL-3.0-or-later.** It statically links [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR)
v8.3.0, which is GPL-3.0-or-later with a Modding Exception and a GPL-3.0 Linking Exception (see
`extern/CommonLibSSE/COPYING.txt` and `EXCEPTIONS.md`), so the binary is a combined work under those terms. This repository,
with its pinned submodule, is the binary's Corresponding Source. The release zip carries a `NOTICE.txt`, the GPL text
(`COPYING.txt`) and `THIRD-PARTY-LICENSES.txt` (spdlog, fmt and rapidcsv, which CommonLibSSE-NG compiles into the DLL).
