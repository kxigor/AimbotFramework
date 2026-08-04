#include <windows.h>

#include <afw/hook/patch.hpp>
#include <afw/win/error.hpp>
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <span>
#include <system_error>
#include <utility>

namespace afw::hook {

namespace {

[[nodiscard]] bool Overwrite(std::uintptr_t address, const std::uint8_t* bytes,
                             std::size_t size) noexcept {
  LPVOID target{std::bit_cast<LPVOID>(address)};

  DWORD previous{0};
  if (VirtualProtect(
          /*lpAddress       = */ target,
          /*dwSize          = */ size,
          /*flNewProtect    = */ PAGE_EXECUTE_READWRITE,
          /*lpflOldProtect  = */ &previous) == FALSE) {
    return false;
  }

  std::memcpy(target, bytes, size);

  DWORD restored{0};
  VirtualProtect(
      /*lpAddress       = */ target,
      /*dwSize          = */ size,
      /*flNewProtect    = */ previous,
      /*lpflOldProtect  = */ &restored);

  FlushInstructionCache(
      /*hProcess          = */ GetCurrentProcess(),
      /*lpBaseAddress     = */ target,
      /*dwSize            = */ size);

  return true;
}

}  // namespace

std::expected<CodePatch, std::error_code> CodePatch::Apply(
    std::uintptr_t address, std::span<const std::uint8_t> bytes) noexcept {
  if (address == 0 || bytes.empty() || bytes.size() > kMaxPatchBytes) {
    return std::unexpected{std::make_error_code(std::errc::invalid_argument)};
  }

  CodePatch patch{};
  patch.address_ = address;
  patch.size_ = bytes.size();
  std::memcpy(patch.original_.data(), std::bit_cast<const void*>(address),
              bytes.size());

  if (!Overwrite(address, bytes.data(), bytes.size())) {
    return std::unexpected{win::LastError()};
  }

  return patch;
}

CodePatch::CodePatch(CodePatch&& other) noexcept
    : address_{std::exchange(other.address_, 0)},
      original_{other.original_},
      size_{std::exchange(other.size_, 0)} {}

CodePatch& CodePatch::operator=(CodePatch&& other) noexcept {
  if (this != &other) {
    Restore();
    address_ = std::exchange(other.address_, 0);
    original_ = other.original_;
    size_ = std::exchange(other.size_, 0);
  }

  return *this;
}

CodePatch::~CodePatch() { Restore(); }

void CodePatch::Restore() noexcept {
  if (address_ == 0 || size_ == 0) {
    return;
  }

  static_cast<void>(Overwrite(address_, original_.data(), size_));
  address_ = 0;
  size_ = 0;
}

}  // namespace afw::hook
