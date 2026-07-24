# AimbotFramework

A minimal C++23 project skeleton for a single-player Far Cry 3 experiment
(WinAPI / geometry / reverse-engineering practice). It is **structure only** —
the framework code is yours to write. What ships here:

- **`afw::process`** — generic access to the host process (module bases,
  pattern scanning, memory helpers). Engine-agnostic. Empty skeleton.
- **`afw::games::fc3`** — Far Cry 3 (Dunia) specific bindings: offsets, structs,
  entity/bone/view-matrix reading. Depends on `afw::process`. One library per
  title, so more games sit alongside as `afw::games::<title>`. Empty skeleton.
- **`afw::aimbot`** — the aim-assist logic, kept engine-agnostic (a game will be
  wired in later behind an interface). Empty skeleton.
- **`aimbot.dll`** — a DLL shim: a stub `DllMain` that compiles and hosts the
  libraries above.
- the build/QA machinery from the upstream template, fixed for **MSYS2 UCRT64**
  (PE hardening, platform-gated sanitizers, optional coverage, tool discovery).

`afw` = **A**imbot**F**rame**w**ork (namespace / target prefix).

## Layout

```
CMakeLists.txt          root: options (AFW_*), structure
CMakePresets.json       UCRT64-oriented presets
cmake/                  ProjectOptions / Sanitizers / Coverage / Format / Tidy / Gtest
include/afw/            public headers (sibling of src/)
  process/              afw::process headers      <- your headers
  games/fc3/            afw::games::fc3 headers    <- your headers
  aimbot/               afw::aimbot headers        <- your headers
src/
  process/              afw::process     (STATIC)  <- your code
  games/fc3/            afw::games::fc3  (STATIC)  <- your code
  aimbot/               afw::aimbot      (STATIC)  <- your code
dll/
  dllmain.cpp           stub entry point -> aimbot.dll
tests/                  GTest + CTest wiring (one placeholder test)
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

The output is `aimbot.dll` (no `lib` prefix), with the MinGW runtime linked
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
| `AFW_BUILD_TESTS`        | ON (top-level)     | build the GTest suite            |
| `AFW_ENABLE_FORMAT_CHECK`| ON (top-level)     | register clang-format as a test  |
| `AFW_ENABLE_TIDY_CHECK`  | ON (top-level)     | register clang-tidy as a test    |
| `AFW_ENABLE_COVERAGE`    | OFF                | gcov targets (needs tests)       |

## License & scope

Educational Source-Available License (see `LICENSE`): view, study, compile and
modify for **educational, non-commercial** use, no warranty, no liability. For
single-player use only (Far Cry 3's online services are dead) — not for
multiplayer / anti-cheat evasion.
