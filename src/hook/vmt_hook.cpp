#include <windows.h>

#include <afw/hook/error.hpp>
#include <afw/hook/vmt_hook.hpp>
#include <afw/win/protect_guard.hpp>
#include <cstddef>
#include <expected>
#include <system_error>
#include <tuple>

namespace afw::hook {

namespace {

[[nodiscard]] void* ReadSlot(void** slot) noexcept {
  MEMORY_BASIC_INFORMATION info{};
  const SIZE_T kQueried{VirtualQuery(
      /*lpAddress = */ static_cast<LPCVOID>(slot),
      /*lpBuffer  = */ &info,
      /*dwLength  = */ sizeof(info))};
  if (kQueried != sizeof(info) || info.State != MEM_COMMIT ||
      info.Protect == PAGE_NOACCESS || (info.Protect & PAGE_GUARD) != 0) {
    return nullptr;
  }

  return *slot;
}

}  // namespace

VmtHook::VmtHook(void** vtable, std::size_t index) noexcept
    : kSlot_{vtable + index}, kOriginal_{ReadSlot(kSlot_)} {}

VmtHook::~VmtHook() { std::ignore = Disable(); }

std::expected<void, std::error_code> VmtHook::SetDetourRaw(
    void* detour) noexcept {
  if (kOriginal_ == nullptr) {
    return std::unexpected{make_error_code(Error::SlotNotReadable)};
  }

  detour_ = detour;

  return {};
}

std::expected<void, std::error_code> VmtHook::Enable() noexcept {
  if (detour_ == nullptr) {
    return std::unexpected{make_error_code(Error::DetourNotSet)};
  }
  if (isEnabled_) {
    return std::unexpected{make_error_code(Error::HookAlreadyApplied)};
  }

  const std::expected<void, std::error_code> kSwapped{SwapSlot(detour_)};
  if (!kSwapped) {
    return std::unexpected{kSwapped.error()};
  }

  isEnabled_ = true;

  return {};
}

std::expected<void, std::error_code> VmtHook::Disable() noexcept {
  if (!isEnabled_) {
    return std::unexpected{make_error_code(Error::HookNotApplied)};
  }

  const std::expected<void, std::error_code> kSwapped{SwapSlot(kOriginal_)};
  if (!kSwapped) {
    return std::unexpected{kSwapped.error()};
  }

  isEnabled_ = false;

  return {};
}

std::expected<void, std::error_code> VmtHook::SwapSlot(void* value) noexcept {
  std::expected<win::ProtectGuard, std::error_code> guard{
      win::ProtectGuard::Create(GetCurrentProcess(),
                                static_cast<LPVOID>(kSlot_), sizeof(void*),
                                PAGE_READWRITE)};
  if (!guard) {
    return std::unexpected{guard.error()};
  }

  *kSlot_ = value;

  return guard->Restore();
}

}  // namespace afw::hook
