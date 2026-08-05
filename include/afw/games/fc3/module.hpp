#pragma once

#include <cstdint>

namespace afw::games::fc3 {

// Turns an address read off the disassembly into one in the running image.
// Every constant in internal/layout is a Ghidra address, so nothing reaches the
// game without passing through here.
[[nodiscard]] std::uintptr_t Resolve(std::uintptr_t ghidraAddress) noexcept;

// What the chain to the camera looks like right now, whole, whether or not it
// leads anywhere. Reading it is how a missing overlay is traced to the link
// that broke rather than guessed at.
struct Diagnostics {
  [[nodiscard]] static Diagnostics Read() noexcept;

  std::uintptr_t moduleBase{};
  std::uintptr_t containerAddress{};
  std::uintptr_t container{};
  std::int32_t liveFlag{};
  std::uintptr_t manager{};
  std::uintptr_t cameraView{};
  std::uintptr_t viewportAddress{};
  std::uintptr_t viewportBlock{};
  std::int32_t viewportWidth{};
  std::int32_t viewportHeight{};
};

}  // namespace afw::games::fc3
