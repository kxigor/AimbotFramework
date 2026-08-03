#include <windows.h>

#include <afw/app/fault.hpp>
#include <afw/app/log.hpp>
#include <afw/core/cast.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <tuple>

namespace afw::app {

namespace {

constexpr std::size_t kMessageChars{512};
constexpr DWORD kWriteFlag{1};

[[nodiscard]] LPTOP_LEVEL_EXCEPTION_FILTER& PreviousFilter() noexcept {
  static LPTOP_LEVEL_EXCEPTION_FILTER previous{nullptr};
  return previous;
}

void LogOrigin(const void* address) {
  std::array<WCHAR, MAX_PATH> path{};
  HMODULE module{nullptr};
  std::array<char, kMessageChars> message{};

  if (GetModuleHandleExW(
          /*dwFlags      = */ GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
          /*lpModuleName = */ static_cast<PCWSTR>(address),
          /*phModule     = */ &module) == FALSE ||
      GetModuleFileNameW(
          /*hModule    = */ module,
          /*lpFilename = */ path.data(),
          /*nSize      = */ static_cast<DWORD>(MAX_PATH)) == 0) {
    std::snprintf(message.data(), message.size(),
                  "[aimbot] fault in no known module\n");
    Log(message.data());
    return;
  }

  const std::uintptr_t kBase{core::PointerCast<std::uintptr_t>(module)};
  const std::uintptr_t kOffset{core::PointerCast<std::uintptr_t>(address) -
                               kBase};
  std::snprintf(message.data(), message.size(),
                "[aimbot] fault in %ls + %llX\n", path.data(),
                static_cast<std::uint64_t>(kOffset));
  Log(message.data());
}

void LogAccess(const EXCEPTION_RECORD& record) {
  if (record.ExceptionCode != EXCEPTION_ACCESS_VIOLATION ||
      record.NumberParameters < 2) {
    return;
  }

  std::array<char, kMessageChars> message{};
  std::snprintf(
      message.data(), message.size(), "[aimbot] %s at %llX\n",
      record.ExceptionInformation[0] == kWriteFlag ? "writing" : "reading",
      static_cast<std::uint64_t>(record.ExceptionInformation[1]));
  Log(message.data());
}

LONG WINAPI OnFault(EXCEPTION_POINTERS* pointers) {
  if (pointers != nullptr && pointers->ExceptionRecord != nullptr) {
    const EXCEPTION_RECORD& kRecord{*pointers->ExceptionRecord};

    std::array<char, kMessageChars> message{};
    std::snprintf(
        message.data(), message.size(),
        "[aimbot] unhandled exception %lX at %llX\n", kRecord.ExceptionCode,
        static_cast<std::uint64_t>(
            core::PointerCast<std::uintptr_t>(kRecord.ExceptionAddress)));
    Log(message.data());

    LogOrigin(kRecord.ExceptionAddress);
    LogAccess(kRecord);
  }

  LPTOP_LEVEL_EXCEPTION_FILTER previous{PreviousFilter()};
  if (previous != nullptr) {
    return previous(pointers);
  }

  return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

FaultLogger::FaultLogger() noexcept {
  PreviousFilter() = SetUnhandledExceptionFilter(
      /*lpTopLevelExceptionFilter = */ &OnFault);
}

FaultLogger::~FaultLogger() {
  std::ignore = SetUnhandledExceptionFilter(
      /*lpTopLevelExceptionFilter = */ PreviousFilter());
  PreviousFilter() = nullptr;
}

}  // namespace afw::app
