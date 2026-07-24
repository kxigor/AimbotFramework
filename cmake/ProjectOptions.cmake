# cmake/ProjectOptions.cmake

# ========================================
# 1. ARCHITECTURE SETUP & PURPOSE
# ========================================
# Defines core compiler/linker flags, optimization levels and hardening for the
# whole project via the 'project_options' INTERFACE library, linked by ALL
# targets (core lib, DLL shim, tests) for consistent build settings.
#
# Platform note: this project targets Windows / MSYS2 (UCRT64) and produces a
# DLL. The ELF-only hardening from the upstream template (-fPIE/-pie,
# -Wl,-z,relro/now) is NOT valid for the PE/COFF linker and is meaningless for a
# shared library, so it is gated behind `if(NOT WIN32)` and replaced with the
# PE equivalents (--dynamicbase / --nxcompat / --high-entropy-va) on Windows.

include_guard()

add_library(project_options INTERFACE)

# ========================================
# 2. COMPILER PRESETS (COMPILE OPTIONS)
# ========================================

# --- GCC/Clang Compiler ---
set(GCC_BASE_COMPILE
  -Wall -Wextra -Wpedantic # Standard, recommended warnings
  -fstack-protector-strong # Basic stack overflow protection (security)
)
# Position-independent code only makes sense on ELF targets. On Windows PIC is
# implicit for DLLs and -fPIE actively conflicts with shared-library codegen.
if(NOT WIN32)
  list(APPEND GCC_BASE_COMPILE -fPIE)
endif()

# Aggressive flags: maximal strictness for Debug/CI builds.
# NOTE: -Wmissing-declarations is intentionally omitted. A DLL's loader-invoked
# entry point (DllMain) legitimately has no prior declaration, and adding one to
# satisfy the warning would trip -Wredundant-decls instead -- so the check is a
# net nuisance for this project.
set(GCC_AGGRESSIVE_COMPILE
  -Werror -Waggressive-loop-optimizations -Wcast-align
  -Wcast-qual -Wchar-subscripts -Wconversion -Wempty-body -Wformat-nonliteral
  -Wsuggest-final-methods -Wsuggest-final-types -Wswitch-default  -Werror=vla
  -Wsign-conversion -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wunused
  -Wno-varargs -fcheck-new -fstrict-overflow -fno-omit-frame-pointer -Winline
  -Wvariadic-macros  -Wno-missing-field-initializers  -Wno-narrowing -Wpacked
  -Wswitch-enum   -Wsync-nand    -Wundef  -Wunreachable-code   -Wno-self-move
  -Wformat-security      -Wformat-signedness      -Wformat=2     -Wlogical-op
  -Wopenmp-simd       -Wpointer-arith      -Winit-self      -Wredundant-decls
)

set(GCC_DEBUG_COMPILE -g -D_DEBUG -ggdb3 -O0)  # Debugging and no optimization
set(GCC_RELWITHDEB -O2 -g -DNDEBUG)            # Moderate optimization + debug info
set(GCC_RELEASE -O3 -DNDEBUG)                  # Full optimization

# --- MSVC Compiler ---
set(MSVC_BASE_COMPILE /W4 /WX /permissive- /Zc:__cplusplus)
set(MSVC_DEBUG_COMPILE /Zi /Ob0 /Od /RTC1)
set(MSVC_RELWITHDEB /Zi /O2 /DNDEBUG)
set(MSVC_RELEASE /O2 /Oi /Gy /DNDEBUG /Zi /GL)

# ========================================
# 3. LINKER PRESETS (LINK OPTIONS)
# ========================================

# --- GCC/Clang Linker ---
if(WIN32)
  # PE/COFF hardening understood by the MinGW/LLD linkers. --high-entropy-va
  # requires --dynamicbase and a 64-bit image (UCRT64 x86_64) -> ASLR + top-of
  # -address-space entropy + DEP.
  set(GCC_BASE_LINK
    -Wl,--dynamicbase
    -Wl,--nxcompat
    -Wl,--high-entropy-va
  )
else()
  # ELF hardening: PIE + full RELRO (lazy binding disabled).
  set(GCC_BASE_LINK
    -pie
    -Wl,-z,relro
    -Wl,-z,now
  )
endif()

# --- MSVC Linker ---
set(MSVC_BASE_LINK
  /DYNAMICBASE
  /NXCOMPAT
  /HIGHENTROPYVA
)

# ========================================
# 4. APPLYING LOGIC VIA GENERATOR EXPRESSIONS
# ========================================

target_compile_options(project_options INTERFACE
  $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>: ${GCC_BASE_COMPILE}>
  $<$<COMPILE_LANG_AND_ID:CXX,MSVC>: ${MSVC_BASE_COMPILE}>

  $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>>: ${GCC_DEBUG_COMPILE} ${GCC_AGGRESSIVE_COMPILE}>
  $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>: ${MSVC_DEBUG_COMPILE}>

  $<$<AND:$<CONFIG:RelWithDebInfo>,$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>>: ${GCC_RELWITHDEB}>
  $<$<AND:$<CONFIG:RelWithDebInfo>,$<CXX_COMPILER_ID:MSVC>>: ${MSVC_RELWITHDEB}>

  $<$<AND:$<CONFIG:Release>,$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>>: ${GCC_RELEASE}>
  $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>: ${MSVC_RELEASE}>
)

target_link_options(project_options INTERFACE
  $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>: ${GCC_BASE_LINK}>
  $<$<COMPILE_LANG_AND_ID:CXX,MSVC>: ${MSVC_BASE_LINK}>

  $<$<AND:$<CONFIG:Release>,$<CXX_COMPILER_ID:MSVC>>: /LTCG>
)
