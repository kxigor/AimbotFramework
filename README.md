# AimbotFramework

A personal C++23 project for learning the Windows API, reverse-engineering and
3D geometry, using a single-player copy of **Far Cry 3** (Dunia engine) as the
subject. It is a work in progress: the low-level plumbing works, the overlay and
the game-specific logic are still being written.

`afw` = **A**imbot**F**rame**w**ork (namespace / target prefix).

## Status

Implemented:

- **`afw::win`** — RAII wrappers over raw Win32 resources: `ScopedHandle`
  (`CloseHandle`), `RemoteMemory` (`VirtualFreeEx` in another process), and a
  `LastError()` helper that turns `GetLastError` into a `std::error_code`.
- **`afw::remote`** — find a process by name and inject a DLL into it via the
  classic CreateRemoteThread + LoadLibraryW technique. Reports failures as
  `std::expected<T, std::error_code>`.
- **`injector.exe`** — standalone tool that locates the target and loads
  `aimbot.dll` into it.
- **`aimbot.dll`** — stub `DllMain` that compiles and will host the libraries
  below.

Skeletons — headers, targets and empty namespaces are in place, the code is not:

- **`afw::win`** — `ProtectGuard` (`VirtualProtect`) and `ComPtr` (COM
  `AddRef`/`Release`) alongside the wrappers above.
- **`afw::hook`** — function interception. `VmtHook` swaps one virtual method
  table slot, which is how COM interfaces such as `IDXGISwapChain` are hooked;
  `WndProcHook` replaces a window procedure through `SetWindowLongPtrW`. Both
  restore what they replaced on destruction. An inline/trampoline hook is
  deliberately deferred — it is not needed to reach a working overlay.
- **`afw::render`** — the overlay's foundation: owns the
  `IDXGISwapChain::Present` hook, the Direct3D 11 resources and the ImGui
  context, and offers its users a per-frame drawing callback.
- **`afw::config`** — settings shared by the overlay's consumers.
- **`afw::menu`** — the overlay's user interface, drawn on top of `afw::config`.
- **`afw::esp`** — game entities projected into screen space and drawn over the
  frame.
- **`afw::process`** — generic access to the host process (module bases, pattern
  scanning, memory helpers). Engine-agnostic.
- **`afw::games::fc3`** — Far Cry 3 (Dunia) bindings: offsets, structs,
  entity/bone/view-matrix reading. Depends on `afw::process`. One library per
  title, so more games can sit alongside as `afw::games::<title>`.
- **`afw::aimbot`** — the aim-assist logic, kept engine-agnostic (a game is
  wired in behind an interface).

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
CMakePresets.json       UCRT64-oriented presets
cmake/                  ProjectOptions / Sanitizers / Coverage / Format / Tidy / Gtest
include/afw/            public headers (sibling of src/)
  win/ remote/          implemented
  hook/ render/ menu/ esp/ config/    skeletons
  process/ games/ aimbot/             skeletons
src/
  win/ remote/          STATIC, Windows-only, implemented
  hook/ render/ menu/ esp/            STATIC, Windows-only, skeletons
  config/                             STATIC, platform-agnostic, skeleton
  process/ games/fc3/ aimbot/         STATIC, skeletons
injector/
  injector.cpp          injector.exe -> loads aimbot.dll via afw::remote
dll/
  dllmain.cpp           stub entry point -> aimbot.dll
tests/                  GTest + CTest wiring
  scripts/              check_format.py / check_tidy.py (no bash dependency)
```

Dependency direction: `games::* -> process`; `render -> hook -> win`; `menu` and
`esp` sit on `render` and share `config`. No library contains a Windows entry
point.

## Target architecture

Far Cry 3's Direct3D 11 executable (`bin/farcry3_d3d11.exe`) is **32-bit**, and a
64-bit DLL cannot be loaded into a 32-bit process. `aimbot.dll` and
`injector.exe` therefore have to be built for x86 with an `i686-w64-mingw32`
toolchain (MSYS2 **MINGW32**); the library code itself is unaffected. That
toolchain branch is not wired into the presets yet.

One build setting does not survive the switch: `-Wl,--high-entropy-va` in
`cmake/ProjectOptions.cmake` requires a 64-bit image, so it has to be dropped for
the x86 targets. `--dynamicbase` and `--nxcompat` still apply.

To confirm on your own copy, read `IMAGE_FILE_HEADER.Machine` from the PE header:
`0x014C` is x86, `0x8664` is x64.

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

The DLL output is `aimbot.dll` (no `lib` prefix), with the MinGW runtime linked
statically so it does not need MSYS2 runtime DLLs on the host's search path, and
PE hardening enabled (`--dynamicbase --nxcompat --high-entropy-va`).

### Presets

| Preset              | Type           | Notes                                  |
|---------------------|----------------|----------------------------------------|
| `dev-debug`         | Debug          | GCC, no sanitizer (default dev build)  |
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

## License & scope

Educational Source-Available License (see `LICENSE`): view, study, compile and
modify for **educational, non-commercial** use, no warranty, no liability. For
single-player use only (Far Cry 3's online services are dead) — not for
multiplayer / anti-cheat evasion.
