#include <windows.h>

#include <afw/win/scoped_handle.hpp>

namespace afw::win {

ScopedHandle::ScopedHandle(HANDLE handle) : handle_{handle} {}

ScopedHandle::ScopedHandle(ScopedHandle&& other) noexcept
    : handle_{other.Release()} {}

ScopedHandle& ScopedHandle::operator=(ScopedHandle&& other) noexcept {
  if (this != &other) {
    Reset(other.Release());
  }
  return *this;
}

ScopedHandle::~ScopedHandle() { Reset(); }

HANDLE ScopedHandle::Get() const { return handle_; }

ScopedHandle::operator bool() const {
  return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE;
}

HANDLE ScopedHandle::Release() {
  HANDLE handle{handle_};
  handle_ = nullptr;
  return handle;
}

void ScopedHandle::Reset(HANDLE handle) {
  if (handle_ == handle) {
    return;
  }
  if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(
        /*hObject = */ handle_);
  }
  handle_ = handle;
}

}  // namespace afw::win
