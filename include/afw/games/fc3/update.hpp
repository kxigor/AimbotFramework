#pragma once

#include <afw/abi/abi.hpp>
#include <afw/hook/hook.hpp>

namespace afw::games::fc3 {

using RunAllGroupFn = void(AFW_ENGINE_CALL*)(void* scheduler);

[[nodiscard]] void** RunAllGroupSlot() noexcept;

class UpdateBinding {
 public:
  explicit UpdateBinding(const hook::Hook& hook) noexcept;

  UpdateBinding(const UpdateBinding&) = delete;
  UpdateBinding& operator=(const UpdateBinding&) = delete;

  UpdateBinding(UpdateBinding&&) = delete;
  UpdateBinding& operator=(UpdateBinding&&) = delete;

  ~UpdateBinding();
};

[[nodiscard]] RunAllGroupFn OriginalRunAllGroup() noexcept;

}  // namespace afw::games::fc3
