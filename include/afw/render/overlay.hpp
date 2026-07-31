#pragma once

#include <expected>
#include <system_error>

struct IDXGISwapChain;

namespace afw::render {

[[nodiscard]] std::expected<void, std::error_code> DrawOverlayFrame(
    IDXGISwapChain* swapChain) noexcept;

}  // namespace afw::render
