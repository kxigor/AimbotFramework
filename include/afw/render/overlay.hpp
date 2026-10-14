#pragma once

#include <windows.h>

#include <cstdint>

namespace afw::render {

using FrameCallback = void (*)() noexcept;

struct SurfaceSize {
  std::uint32_t width{};
  std::uint32_t height{};
};

[[nodiscard]] SurfaceSize BackBufferSize() noexcept;

[[nodiscard]] HWND OverlayWindow() noexcept;

void ShutdownOverlay() noexcept;

}  // namespace afw::render
