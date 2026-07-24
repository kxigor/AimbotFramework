#pragma once

#include <windows.h>

#include <expected>
#include <system_error>

namespace afw::remote {

[[nodiscard]] std::expected<DWORD, std::error_code> GetPidByProcName(
    PCWSTR procName);

[[nodiscard]] std::expected<void, std::error_code> LoadDll(HANDLE process,
                                                           PCWSTR dllPath);

}  // namespace afw::remote
