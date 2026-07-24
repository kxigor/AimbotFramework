#pragma once

#include <windows.h>

namespace afw::win {

class ScopedHandle {
 public:
  ScopedHandle() = default;
  explicit ScopedHandle(HANDLE handle);

  ScopedHandle(const ScopedHandle&) = delete;
  ScopedHandle& operator=(const ScopedHandle&) = delete;

  ScopedHandle(ScopedHandle&& other) noexcept;
  ScopedHandle& operator=(ScopedHandle&& other) noexcept;

  ~ScopedHandle();

  [[nodiscard]] HANDLE Get() const;
  [[nodiscard]] explicit operator bool() const;

  [[nodiscard]] HANDLE Release();
  void Reset(HANDLE handle = nullptr);

 private:
  HANDLE handle_{nullptr};
};

}  // namespace afw::win
