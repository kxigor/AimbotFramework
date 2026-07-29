#pragma once

#include <windows.h>

#include <expected>
#include <system_error>

namespace afw::win {

class ProtectGuard {
  struct AdoptTag {};

 public:
  [[nodiscard]] static std::expected<ProtectGuard, std::error_code> Create(
      HANDLE proc, LPVOID addr, SIZE_T size, DWORD newProtect) noexcept;

  [[nodiscard]] static ProtectGuard Adopt(HANDLE proc, LPVOID addr, SIZE_T size,
                                          DWORD oldProtect) noexcept;

  ProtectGuard(AdoptTag tag, HANDLE proc, LPVOID addr, SIZE_T size,
               DWORD oldProtect) noexcept;

  ProtectGuard(const ProtectGuard&) = delete;
  ProtectGuard& operator=(const ProtectGuard&) = delete;

  ProtectGuard(ProtectGuard&&) = delete;
  ProtectGuard& operator=(ProtectGuard&&) = delete;

  ~ProtectGuard();

  [[nodiscard]] std::expected<void, std::error_code> Restore() noexcept;

 private:
  HANDLE proc_{nullptr};
  LPVOID addr_{nullptr};
  SIZE_T size_{};
  DWORD oldProtect_{};
};

}  // namespace afw::win
