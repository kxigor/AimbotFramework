#pragma once

#include <afw/hook/vtable.hpp>
#include <cstddef>
#include <expected>
#include <system_error>

namespace afw::render::dxgi {

constexpr std::size_t kPresentSlot{8};

[[nodiscard]] std::expected<hook::Vtable, std::error_code>
ProbeSwapChainVtable() noexcept;

}  // namespace afw::render::dxgi
