#pragma once

#include <expected>
#include <system_error>

namespace afw::render {

[[nodiscard]] std::expected<void**, std::error_code>
ProbeSwapChainVtable() noexcept;

}  // namespace afw::render
