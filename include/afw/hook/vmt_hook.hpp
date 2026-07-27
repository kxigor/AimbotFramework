#pragma once

// Hook over a single virtual method table slot: swaps one function pointer for
// a detour and puts the original back on destruction.

#include <windows.h>

#include <utility>

namespace afw::hook {
class VmtHook {
 public:
  VmtHook(void** vtable, std::size_t sourcePos, LPVOID target)
      : vtable_{vtable}, sourcePos_{sourcePos}, target_{target} {}

  VmtHook(const VmtHook& /*unused*/) = delete;
  VmtHook(VmtHook&& /*unused*/) = delete;

  ~VmtHook() {
    if (is_enabled_) {
      Disable();
    }
  }

  VmtHook& operator=(const VmtHook& /*unused*/) = delete;
  VmtHook& operator=(VmtHook&& /*unused*/) = delete;

  void Enable() {
    if (is_enabled_) {
      // TODO: error
    }
    
  }

  void Disable() {
    if (!is_enabled_) {
      // TODO error
    }
    //...
  }

 private:
  bool is_enabled_{false};

  void** vtable_{};
  std::size_t sourcePos_{};
  LPVOID target_{};
};
}  // namespace afw::hook
