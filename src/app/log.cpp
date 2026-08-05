#include <windows.h>

#include <afw/app/log.hpp>
#include <afw/app/module.hpp>
#include <afw/win/scoped_handle.hpp>
#include <array>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <system_error>

namespace afw::app {

namespace {

constexpr std::size_t kMessageChars{256};
constexpr std::size_t kSuffixChars{5};

}  // namespace

void Log(const char* text) {
  OutputDebugStringA(text);

  const HMODULE kModule{SelfModule()};
  if (kModule == nullptr) {
    return;
  }

  std::array<WCHAR, MAX_PATH> path{};
  const DWORD kLength{GetModuleFileNameW(
      /*hModule    = */ kModule,
      /*lpFilename = */ path.data(),
      /*nSize      = */ static_cast<DWORD>(MAX_PATH))};
  if (kLength == 0 ||
      kLength + kSuffixChars > static_cast<std::size_t>(MAX_PATH)) {
    return;
  }
  std::wmemcpy(path.data() + kLength, L".log", kSuffixChars);

  const win::ScopedHandle kFile{CreateFileW(
      /*lpFileName            = */ path.data(),
      /*dwDesiredAccess       = */ FILE_APPEND_DATA,
      /*dwShareMode           = */ FILE_SHARE_READ | FILE_SHARE_WRITE,
      /*lpSecurityAttributes  = */ nullptr,
      /*dwCreationDisposition = */ OPEN_ALWAYS,
      /*dwFlagsAndAttributes  = */ FILE_ATTRIBUTE_NORMAL,
      /*hTemplateFile         = */ nullptr)};
  if (!kFile) {
    return;
  }

  DWORD written{};
  WriteFile(
      /*hFile                  = */ kFile.Get(),
      /*lpBuffer               = */ text,
      /*nNumberOfBytesToWrite  = */ static_cast<DWORD>(std::strlen(text)),
      /*lpNumberOfBytesWritten = */ &written,
      /*lpOverlapped           = */ nullptr);
}

void Logf(const char* format, ...) {
  std::array<char, kMessageChars> message{};

  // NOLINTNEXTLINE(cppcoreguidelines-init-variables)
  va_list arguments;
  va_start(arguments, format);
  std::vsnprintf(message.data(), message.size(), format, arguments);
  va_end(arguments);

  Log(message.data());
}

void LogFailure(const char* step, const std::error_code& error) {
  Logf("[aimbot] %s failed: %s\n", step, error.message().c_str());
}

void LogInstalled(const char* what) { Logf("[aimbot] hooked %s\n", what); }

}  // namespace afw::app
