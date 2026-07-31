#pragma once

namespace afw::win {

template <typename T>
class ComPtr {
 public:
  ComPtr() = default;
  explicit ComPtr(T* raw) noexcept : raw_{raw} {}

  ComPtr(const ComPtr&) = delete;
  ComPtr& operator=(const ComPtr&) = delete;

  ComPtr(ComPtr&& other) noexcept : raw_{other.Release()} {}

  ComPtr& operator=(ComPtr&& other) noexcept {
    if (this != &other) {
      Reset(other.Release());
    }
    return *this;
  }

  ~ComPtr() { Reset(); }

  [[nodiscard]] T* Get() const noexcept { return raw_; }
  [[nodiscard]] T* operator->() const noexcept { return raw_; }
  [[nodiscard]] explicit operator bool() const noexcept {
    return raw_ != nullptr;
  }

  [[nodiscard]] T** Put() noexcept {
    Reset();
    return &raw_;
  }

  [[nodiscard]] T* Release() noexcept {
    T* raw{raw_};
    raw_ = nullptr;
    return raw;
  }

  void Reset(T* raw = nullptr) noexcept {
    if (raw_ == raw) {
      return;
    }
    if (raw_ != nullptr) {
      raw_->Release();
    }
    raw_ = raw;
  }

 private:
  T* raw_{nullptr};
};

}  // namespace afw::win
