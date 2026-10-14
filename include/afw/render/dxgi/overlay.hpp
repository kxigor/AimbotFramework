#pragma once

#include <afw/render/overlay.hpp>
#include <expected>
#include <system_error>

struct IDXGISwapChain;

namespace afw::render::dxgi {

[[nodiscard]] std::expected<void, std::error_code> DrawOverlayFrame(
    IDXGISwapChain* swapChain, FrameCallback onFrame) noexcept;

}  // namespace afw::render::dxgi
