# AimbotFramework

A personal C++23 project for learning the Windows API, reverse-engineering and
3D geometry, using a single-player copy of **Far Cry 3** (Dunia engine) as the
subject. It is a work in progress: the low-level plumbing works, the
game-specific and aim-assist logic is still being written.

`afw` = **A**imbot**F**rame**w**ork (namespace / target prefix).

## Status

Working:

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

Still to write:

- **`afw::process`** — generic access to the host process (module bases, pattern
  scanning, memory helpers). Engine-agnostic.
- **`afw::games::fc3`** — Far Cry 3 (Dunia) bindings: offsets, structs,
  entity/bone/view-matrix reading. Depends on `afw::process`. One library per
  title, so more games can sit alongside as `afw::games::<title>`.
- **`afw::aimbot`** — the aim-assist logic, kept engine-agnostic (a game is wired
  in behind an interface).

## Layout

```
CMakeLists.txt          root: options (AFW_*) and subdirectories
CMakePresets.json       UCRT64-oriented presets
cmake/                  ProjectOptions / Sanitizers / Coverage / Format / Tidy / Gtest
include/afw/            public headers (sibling of src/)
  win/                  afw::win     (implemented)
  remote/               afw::remote  (implemented)
  process/ games/ aimbot/  headers (planned)
src/
  win/                  afw::win     (STATIC, Windows-only)
  remote/               afw::remote  (STATIC, Windows-only)
  process/ games/fc3/ aimbot/  (STATIC, empty namespaces for now)
injector/
  injector.cpp          injector.exe -> loads aimbot.dll via afw::remote
dll/
  dllmain.cpp           stub entry point -> aimbot.dll
tests/                  GTest + CTest wiring
  scripts/              check_format.py / check_tidy.py (no bash dependency)
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
