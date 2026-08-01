#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <system_error>

namespace afw::games::fc3 {

constexpr std::size_t kAxisCount{3};

using Triple = std::array<float, kAxisCount>;

struct CameraBasis {
  Triple position{};
  Triple forward{};
  Triple upAxis{};
  Triple right{};
  float fovRadians{};
};

struct Viewport {
  float width{};
  float height{};
};

struct Diagnostics {
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

[[nodiscard]] Diagnostics ReadDiagnostics() noexcept;

[[nodiscard]] std::uintptr_t Resolve(std::uintptr_t ghidraAddress) noexcept;

[[nodiscard]] std::expected<CameraBasis, std::error_code> ReadCamera() noexcept;

[[nodiscard]] std::expected<Viewport, std::error_code> ReadViewport() noexcept;

}  // namespace afw::games::fc3
