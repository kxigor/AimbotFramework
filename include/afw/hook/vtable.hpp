#pragma once

#include <cstddef>

namespace afw::hook {

class Vtable {
 public:
  explicit Vtable(void** table) noexcept;

  [[nodiscard]] void** Get() const noexcept;
  [[nodiscard]] void** Slot(std::size_t index) const noexcept;
  [[nodiscard]] void* At(std::size_t index) const noexcept;

 private:
  void** table_{nullptr};
};

}  // namespace afw::hook
