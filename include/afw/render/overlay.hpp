#pragma once

#include <expected>
#include <system_error>

struct IDXGISwapChain;

namespace afw::render {

using FrameCallback = void (*)() noexcept;

[[nodiscard]] std::expected<void, std::error_code> DrawOverlayFrame(
    IDXGISwapChain* swapChain, FrameCallback onFrame) noexcept;

}  // namespace afw::render
