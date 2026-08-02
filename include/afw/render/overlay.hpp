#pragma once

#include <cstdint>
#include <expected>
#include <system_error>

struct IDXGISwapChain;

namespace afw::render {

using FrameCallback = void (*)() noexcept;

// The swap chain's own surface; the game's internal render size may differ.
struct SurfaceSize {
  std::uint32_t width{};
  std::uint32_t height{};
};

[[nodiscard]] SurfaceSize BackBufferSize() noexcept;

[[nodiscard]] std::expected<void, std::error_code> DrawOverlayFrame(
    IDXGISwapChain* swapChain, FrameCallback onFrame) noexcept;

}  // namespace afw::render
