#include <afw/render/present_hook.hpp>

namespace afw::render {

namespace {

const hook::VmtHook*& BoundHook() noexcept {
  static const hook::VmtHook* hook{nullptr};
  return hook;
}

}  // namespace

PresentBinding::PresentBinding(const hook::VmtHook& hook) noexcept {
  BoundHook() = &hook;
}

PresentBinding::~PresentBinding() { BoundHook() = nullptr; }

PresentFn OriginalPresent() noexcept {
  const hook::VmtHook* const kHook{BoundHook()};
  if (kHook == nullptr) {
    return nullptr;
  }

  return kHook->Original<PresentFn>();
}

}  // namespace afw::render
