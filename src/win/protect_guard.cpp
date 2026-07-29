#include <windows.h>

#include <afw/win/error.hpp>
#include <afw/win/protect_guard.hpp>
#include <expected>
#include <system_error>
#include <tuple>
#include <utility>

namespace afw::win {

std::expected<ProtectGuard, std::error_code> ProtectGuard::Create(
    HANDLE proc, LPVOID addr, SIZE_T size, DWORD newProtect) noexcept {
  DWORD oldProtect{};

  const BOOL kStatus{VirtualProtectEx(
      /*hProcess       = */ proc,
      /*lpAddress      = */ addr,
      /*dwSize         = */ size,
      /*flNewProtect   = */ newProtect,
      /*lpflOldProtect = */ &oldProtect)};
  if (kStatus == FALSE) {
    return std::unexpected{LastError()};
  }

  return std::expected<ProtectGuard, std::error_code>{
      std::in_place, AdoptTag{}, proc, addr, size, oldProtect};
}

ProtectGuard ProtectGuard::Adopt(HANDLE proc, LPVOID addr, SIZE_T size,
                                 DWORD oldProtect) noexcept {
  return ProtectGuard{AdoptTag{}, proc, addr, size, oldProtect};
}

ProtectGuard::ProtectGuard(AdoptTag /*tag*/, HANDLE proc, LPVOID addr,
                           SIZE_T size, DWORD oldProtect) noexcept
    : proc_{proc}, addr_{addr}, size_{size}, oldProtect_{oldProtect} {}

ProtectGuard::~ProtectGuard() { std::ignore = Restore(); }

std::expected<void, std::error_code> ProtectGuard::Restore() noexcept {
  if (addr_ == nullptr) {
    return {};
  }

  DWORD previous{};
  const BOOL kStatus{VirtualProtectEx(
      /*hProcess       = */ proc_,
      /*lpAddress      = */ addr_,
      /*dwSize         = */ size_,
      /*flNewProtect   = */ oldProtect_,
      /*lpflOldProtect = */ &previous)};
  addr_ = nullptr;

  if (kStatus == FALSE) {
    return std::unexpected{LastError()};
  }

  return {};
}

}  // namespace afw::win
