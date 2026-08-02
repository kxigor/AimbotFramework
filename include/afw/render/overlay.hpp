#pragma once

#include <windows.h>

#include <cstdint>
#include <expected>
#include <system_error>

struct IDXGISwapChain;

namespace afw::render {

using FrameCallback = void (*)() noexcept;

struct SurfaceSize {
  std::uint32_t width{};
  std::uint32_t height{};
};

[[nodiscard]] SurfaceSize BackBufferSize() noexcept;

[[nodiscard]] HWND OverlayWindow() noexcept;

[[nodiscard]] std::expected<void, std::error_code> DrawOverlayFrame(
    IDXGISwapChain* swapChain, FrameCallback onFrame) noexcept;

void ShutdownOverlay() noexcept;

}  // namespace afw::render
