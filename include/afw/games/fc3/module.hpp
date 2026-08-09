#pragma once

#include <cstdint>

namespace afw::games::fc3 {

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
