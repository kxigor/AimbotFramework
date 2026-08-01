# AimbotFramework

A personal C++23 project for learning the Windows API, reverse-engineering and
3D geometry, using a single-player copy of **Far Cry 3** (Dunia engine) as the
subject.

The pipeline works end to end: `injector.exe` loads `aimbot.dll` into the running
game, the DLL hooks `IDXGISwapChain::Present`, draws an ImGui overlay over the
frame, reads the game's live camera and projects world points into screen space.
What is left is the entity list and the aim assist on top of it.

`afw` = **A**imbot**F**rame**w**ork (namespace / target prefix).

## Status

Implemented:

- **`afw::win`** — RAII over raw Win32 and COM resources: `ScopedHandle`
  (`CloseHandle`), `RemoteMemory` (`VirtualFreeEx` in another process),
  `ProtectGuard` (`VirtualProtectEx`, restores the previous protection),
  `ComPtr` (COM `Release`), and `LastError()` turning `GetLastError` into a
  `std::error_code`.
- **`afw::remote`** — find a process by image name and inject a DLL into it via
  the classic CreateRemoteThread + LoadLibraryW technique. Success is decided by
  the remote thread's exit code, not by the API call returning.
- **`afw::hook`** — `Vtable` names the layout of a virtual method table and hands
  out the address of a slot; `VmtHook` swaps one such slot and puts the original
  back on destruction. The original is captured at construction, so a detour that
  chains to it can be published before the slot is ever written.
- **`afw::process`** — module bases and reads that report an unreadable address
  instead of faulting: a wrong offset becomes a `std::error_code`, not a crash in
  the host process.
- **`afw::render`** — obtains the swap chain vtable from a throwaway WARP probe,
  owns the `Present` binding, and draws the ImGui overlay, offering its users a
  per-frame callback.
- **`afw::esp`** — world-to-screen: view and projection matrices built by hand,
  `w`-clipping for points behind the camera, plus a diagnostics panel showing
  every address, offset and matrix input in play.
- **`afw::games::fc3`** — Far Cry 3 bindings: the pointer chain to the live
  camera, its basis and field of view, and the render viewport. Every step is a
  checked read, and all addresses are rebased from the Ghidra image base at run
  time, so ASLR is handled in one place.
- **`injector.exe`** / **`aimbot.dll`** — the working pair, verified in the game.

Skeletons — headers, targets and empty namespaces are in place, the code is not:

- **`afw::hook::WndProcHook`** — window procedure replacement through
  `SetWindowLongPtrW`, for routing input between the game and the overlay.
- **`afw::config`** — settings shared by the overlay's consumers.
- **`afw::menu`** — the overlay's user interface, drawn on top of `afw::config`.
- **`afw::aimbot`** — the aim-assist logic, kept engine-agnostic.

An inline/trampoline hook is still absent. It was not needed to reach a working
overlay; it is needed for the entity list, which the engine exposes through a
non-virtual per-frame function.

### Milestones

| | | |
|---|---|---|
| M0 | 32-bit build, injection | done |
| M1 | RAII wrappers (`ProtectGuard`, `ComPtr`, `VmtHook`) | done |
| M2 | `Present` hook, frames counted, nothing drawn | done |
| M3 | first pixel: ImGui with both backends | done |
| M6 | geometry: a marker pinned to a world point | done |
| M4 | input: `WndProcHook`, overlay toggle | next |
| M5 | `ResizeBuffers` | not blocking — the render target view is rebuilt every frame |
| M7 | ESP: entity list to boxes | needs an inline hook |
| M8 | menu | |

M6 was taken before M4 and M5 deliberately: the geometry is the part the project
exists to learn, and it needs neither input nor a cached render target.

### Direction

The ESP comes before the aim assist. It exercises the whole pipeline — locate
entities, read memory, project to screen, draw — while staying **read-only**, so
a mistake shows up as a visual glitch rather than as corrupted game memory. The
aim assist adds angle solving and writes into another process, which is a
separate class of problem and is cheaper to approach once the pipeline works.

The foundation is the frame loop, not the menu. Once `Present` is hooked there is
a per-frame callback holding a valid `ID3D11Device` / `ID3D11DeviceContext`;
`afw::menu` and `afw::esp` are **siblings** on top of it and never reference each
other, sharing state only through `afw::config`. That way the ESP runs with
settings alone and needs no user interface present.

## Layout

```
CMakeLists.txt          root: options (AFW_*) and subdirectories
CMakePresets.json       UCRT64 and MINGW32 presets
cmake/                  ProjectOptions / Sanitizers / Coverage / Format / Tidy / Gtest
third_party/
  imgui/                Dear ImGui, git submodule pinned to a release tag
  CMakeLists.txt        its build file, kept outside the submodule
include/afw/            public headers (sibling of src/)
  win/ remote/ hook/ process/ render/ esp/ games/fc3/    implemented
  menu/ config/ aimbot/ hook/wnd_proc_hook.hpp           skeletons
src/
  win/ remote/ hook/ process/ render/ esp/ games/fc3/    STATIC, Windows-only
  config/                                 STATIC, platform-agnostic, skeleton
  menu/ aimbot/                           STATIC, skeletons
injector/
  injector.cpp          injector.exe -> loads aimbot.dll via afw::remote
dll/
  dllmain.cpp           entry point -> probes, hooks Present, parks
tests/                  GTest + CTest wiring
  scripts/              check_format.py / check_tidy.py (no bash dependency)
```

Dependency direction: `games::* -> process`; `render -> hook -> win`; `esp` sits
on `render` and `games::fc3`; `menu` is its sibling and they share `config`. No
library contains a Windows entry point.

`afw::process` reads this process's memory through the Win32 API, so it and the
libraries built on it (`games::*`, `aimbot`) are added only under `WIN32`;
`afw::config` stays platform-agnostic.

## Target architecture

Far Cry 3's Direct3D 11 executable (`bin/farcry3_d3d11.exe`) is **32-bit**, and a
64-bit DLL cannot be loaded into a 32-bit process. `aimbot.dll` and
`injector.exe` therefore have to be built for x86 with an `i686-w64-mingw32`
toolchain (MSYS2 **MINGW32**); the library code itself is unaffected, and the
whole test suite passes on both architectures.

`-Wl,--high-entropy-va` requires a 64-bit image, so `cmake/ProjectOptions.cmake`
applies it only when `CMAKE_SIZEOF_VOID_P` is 8. `--dynamicbase` and
`--nxcompat` apply to both.

To confirm on your own copy, read `IMAGE_FILE_HEADER.Machine` from the PE header:
`0x014C` is x86, `0x8664` is x64.

## Getting the sources

Dear ImGui is a submodule, so a plain `git clone` leaves `third_party/imgui`
empty and the configure step stops with a message saying exactly this:

```sh
git clone --recurse-submodules <url>
# or, in an existing clone:
git submodule update --init --recursive
```

## Build (from an MSYS2 **UCRT64** shell)

`cmake`, `ninja` and `clang` live in UCRT64, not MINGW64.

```sh
# primary dev build: GCC, Debug, tests + DLL, no sanitizer
cmake --preset dev-debug
cmake --build --preset dev-debug
ctest --preset dev-debug

# release DLL only -> build/ci-release/dll/aimbot.dll
cmake --preset ci-release
cmake --build --preset ci-release
```

## Build 32-bit (from an MSYS2 **MINGW32** shell)

This is the build that runs in the game.

```sh
pacman -S --needed mingw-w64-i686-gcc mingw-w64-i686-gtest

cmake --preset dev-debug-x86
cmake --build --preset dev-debug-x86
ctest --preset dev-debug-x86
```

No toolchain paths are pinned anywhere: CMake picks up both the compiler and the
32-bit GTest from the shell's `PATH`, exactly as `dev-debug` does for the 64-bit
environment, so this works wherever MSYS2 is installed. `cmake` and `ninja` from
any environment will do — they are architecture-independent build drivers.

Running a preset from the wrong shell is a configure error rather than a silent
build of the other architecture: `dev-debug-x86` declares
`AFW_TARGET_POINTER_SIZE` and the top-level `CMakeLists.txt` checks it against
the compiler CMake actually found. Note that a **build** started from the wrong
shell fails differently: `cc1plus` cannot find its own DLLs and dies without
printing a diagnostic, so a compile that fails with no message is almost always
`PATH`, not the code.

The lint gate (`FormatCheck`, `TidyCheck`) is registered only in the 64-bit
build; clang-format and clang-tidy do not depend on the target architecture, so
running them twice would only cost time. The vendored ImGui is outside their file
globs and does not link `project_options`: the warning set is tuned for our own
code.

`aimbot.dll` is staged next to `injector.exe` by a build target of its own rather
than a `POST_BUILD` step, because a `POST_BUILD` command only runs when its own
target is relinked — a change confined to the DLL used to leave a stale copy
staged.

The DLL output is `aimbot.dll` (no `lib` prefix), with the MinGW runtime linked
statically so it does not need MSYS2 runtime DLLs on the host's search path, and
PE hardening enabled (`--dynamicbase --nxcompat --high-entropy-va`).

## Running it

Start Far Cry 3 (the DirectX 11 executable), then run `injector.exe` from the
32-bit build. The overlay appears with two windows: a frame-rate readout and an
`ESP` panel listing the whole camera pointer chain, the offsets it walked, the
camera basis and the projected marker positions.

The DLL also writes `aimbot.dll.log` next to itself — the injection steps, then a
status line every 1800 frames. It never unloads: unwinding would restore the
hooked slot while a frame may still be inside the detour.

### Presets

| Preset              | Type           | Notes                                  |
|---------------------|----------------|----------------------------------------|
| `dev-debug`         | Debug          | GCC, no sanitizer (default dev build)  |
| `dev-debug-x86`     | Debug          | MINGW32, the build that runs in game   |
| `dev-debug-asan`    | Debug          | ASan+UBSan, forces `clang++`           |
| `dev-debug-ubsan`   | Debug          | UBSan, forces `clang++`                |
| `ci-coverage`       | Debug          | gcov instrumentation                   |
| `ci-relwithdebinfo` | RelWithDebInfo | tests on                               |
| `ci-release`        | Release        | `aimbot.dll` only, tests off           |

> Thread/Memory sanitizers are intentionally absent: they do not exist on
> Windows for any toolchain. GCC ships no sanitizer runtime at all, so ASan/UBSan
> presets use Clang.

## Options

| Option                   | Default            | Meaning                          |
|--------------------------|--------------------|----------------------------------|
| `AFW_BUILD_DLL`          | ON (top-level+Win) | build `aimbot.dll`               |
| `AFW_BUILD_INJECTOR`     | ON (top-level+Win) | build `injector.exe`             |
| `AFW_BUILD_TESTS`        | ON (top-level)     | build the GTest suite            |
| `AFW_ENABLE_FORMAT_CHECK`| ON (top-level)     | register clang-format as a test  |
| `AFW_ENABLE_TIDY_CHECK`  | ON (top-level)     | register clang-tidy as a test    |
| `AFW_ENABLE_COVERAGE`    | OFF                | gcov targets (needs tests)       |

`AFW_TARGET_POINTER_SIZE` is not an option but a cache variable the presets set:
when present, the top-level `CMakeLists.txt` compares it against
`CMAKE_SIZEOF_VOID_P` and fails the configure if they disagree. Only
`dev-debug-x86` sets it, to `4`.

## License & scope

Educational Source-Available License (see `LICENSE`): view, study, compile and
modify for **educational, non-commercial** use, no warranty, no liability. For
single-player use only (Far Cry 3's online services are dead) — not for
multiplayer / anti-cheat evasion.
