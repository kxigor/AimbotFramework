#pragma once

#include <windows.h>

namespace afw::win {

class RemoteMemory {
 public:
  RemoteMemory() = default;
  RemoteMemory(HANDLE proc, LPVOID addr);

  RemoteMemory(const RemoteMemory&) = delete;
  RemoteMemory& operator=(const RemoteMemory&) = delete;

  RemoteMemory(RemoteMemory&& other) noexcept;
  RemoteMemory& operator=(RemoteMemory&& other) noexcept;

  ~RemoteMemory();

  [[nodiscard]] LPVOID Get() const;
  [[nodiscard]] explicit operator bool() const;

  [[nodiscard]] LPVOID Release();
  void Reset(HANDLE proc = nullptr, LPVOID addr = nullptr);

 private:
  HANDLE proc_{nullptr};
  LPVOID addr_{nullptr};
};

}  // namespace afw::win
