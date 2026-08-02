#pragma once

#include <windows.h>

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

// The window the swap chain presents to, and the one ImGui was initialised
// with. Null until the first frame has gone through DrawOverlayFrame.
[[nodiscard]] HWND OverlayWindow() noexcept;

[[nodiscard]] std::expected<void, std::error_code> DrawOverlayFrame(
    IDXGISwapChain* swapChain, FrameCallback onFrame) noexcept;

// Releases the ImGui contexts and the device references. Only safe once the
// Present hook is gone and no frame can still be inside the callback.
void ShutdownOverlay() noexcept;

}  // namespace afw::render
