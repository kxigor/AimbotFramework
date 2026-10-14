#include <afw/render/dxgi/present_hook.hpp>

namespace afw::render::dxgi {

namespace {

const hook::Hook*& BoundHook() noexcept {
  static const hook::Hook* hook{nullptr};
  return hook;
}

}  // namespace

PresentBinding::PresentBinding(const hook::Hook& hook) noexcept {
  BoundHook() = &hook;
}

PresentBinding::~PresentBinding() { BoundHook() = nullptr; }

PresentFn OriginalPresent() noexcept {
  const hook::Hook* const kHook{BoundHook()};
  if (kHook == nullptr) {
    return nullptr;
  }

  return kHook->Original<PresentFn>();
}

}  // namespace afw::render::dxgi
