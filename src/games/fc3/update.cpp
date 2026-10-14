#include <afw/games/fc3/internal/layout/scheduler.hpp>
#include <afw/games/fc3/internal/module.hpp>
#include <afw/games/fc3/update.hpp>
#include <afw/hook/hook.hpp>
#include <bit>

namespace afw::games::fc3 {

namespace {

[[nodiscard]] const hook::Hook*& BoundUpdate() noexcept {
  static const hook::Hook* hook{nullptr};
  return hook;
}

}  // namespace

void** RunAllGroupSlot() noexcept {
  return std::bit_cast<void**>(Resolve(kRunAllGroupSlotAddress));
}

UpdateBinding::UpdateBinding(const hook::Hook& hook) noexcept {
  BoundUpdate() = &hook;
}

UpdateBinding::~UpdateBinding() { BoundUpdate() = nullptr; }

RunAllGroupFn OriginalRunAllGroup() noexcept {
  const hook::Hook* const kHook{BoundUpdate()};
  if (kHook == nullptr) {
    return nullptr;
  }

  return kHook->Original<RunAllGroupFn>();
}

}  // namespace afw::games::fc3
