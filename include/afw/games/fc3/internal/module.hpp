#pragma once

#include <cstdint>

namespace afw::games::fc3 {

// Every address recovered from the disassembler is recorded against the image
// base Ghidra loaded the module at, and the loader is free to place it
// somewhere else. This turns one into the other, and it is the only way an
// address written down during static analysis may be used against the running
// process.
[[nodiscard]] std::uintptr_t Resolve(std::uintptr_t ghidraAddress) noexcept;

}  // namespace afw::games::fc3
