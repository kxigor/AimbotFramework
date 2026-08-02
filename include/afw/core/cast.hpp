#pragma once

namespace afw::core {

// std::bit_cast is the house spelling for reinterpreting bits, and it covers
// every integer-to-pointer conversion here without a suppression. It cannot
// cover pointer to pointer: bugprone-bitwise-pointer-cast rejects that
// outright. Win32 and COM boundaries still demand it -- an out parameter typed
// void**, a COM interface read as its own vtable -- so the suppression lives
// here once instead of at every call site.
template <typename To, typename From>
[[nodiscard]] To PointerCast(From value) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<To>(value);
}

}  // namespace afw::core
