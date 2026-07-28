#include <windows.h>

#include <afw/hook/error.hpp>
#include <afw/hook/vmt_hook.hpp>
#include <afw/win/protect_guard.hpp>
#include <cstddef>
#include <expected>
#include <system_error>
#include <tuple>
#include <utility>

namespace afw::hook {

VmtHook::VmtHook(void** vtable, std::size_t sourcePos, void* detour) noexcept
    : vtable_{vtable}, kSourcePos_{sourcePos}, kDetour_{detour} {}

VmtHook::~VmtHook() { std::ignore = Disable(); }

std::expected<void, std::error_code> VmtHook::Enable() noexcept {
  if (isEnabled_) {
    return std::unexpected{make_error_code(Error::HookAlreadyApplied)};
  }

  const std::expected<void*, std::error_code> kPrevious{SwapSlot(kDetour_)};
  if (!kPrevious) {
    return std::unexpected{kPrevious.error()};
  }

  original_ = *kPrevious;
  isEnabled_ = true;

  return {};
}

std::expected<void, std::error_code> VmtHook::Disable() noexcept {
  if (!isEnabled_) {
    return std::unexpected{make_error_code(Error::HookNotApplied)};
  }

  const std::expected<void*, std::error_code> kPrevious{SwapSlot(original_)};
  if (!kPrevious) {
    return std::unexpected{kPrevious.error()};
  }

  original_ = {};
  isEnabled_ = false;

  return {};
}

std::expected<void*, std::error_code> VmtHook::SwapSlot(void* value) noexcept {
  std::expected<win::ProtectGuard, std::error_code> guard{
      win::ProtectGuard::Create(GetCurrentProcess(),
                                static_cast<LPVOID>(&vtable_[kSourcePos_]),
                                sizeof(void*), PAGE_READWRITE)};
  if (!guard) {
    return std::unexpected{guard.error()};
  }

  std::swap(vtable_[kSourcePos_], value);

  const std::expected<void, std::error_code> kRestored{guard->Restore()};
  if (!kRestored) {
    return std::unexpected{kRestored.error()};
  }

  return value;
}

}  // namespace afw::hook
