#pragma once

namespace afw::core {

template <typename To, typename From>
[[nodiscard]] To PointerCast(From value) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<To>(value);
}

}  // namespace afw::core
