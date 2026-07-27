#pragma once

#include <windows.h>

#include <afw/win/remote_memory.hpp>
#include <expected>
#include <system_error>

namespace afw::remote {

inline constexpr DWORD kDefaultTimeoutMs{10000};

[[nodiscard]] std::expected<DWORD, std::error_code> FindProcessByImageName(
    PCWSTR imageName);

[[nodiscard]] std::expected<afw::win::RemoteMemory, std::error_code> StrDupEx(
    HANDLE proc, PCWSTR text);

[[nodiscard]] std::expected<void, std::error_code> InjectLibrary(
    HANDLE process, PCWSTR dllPath, DWORD timeoutMs = kDefaultTimeoutMs);

}  // namespace afw::remote
