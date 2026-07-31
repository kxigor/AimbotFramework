#pragma once

#include <windows.h>

#include <afw/hook/vmt_hook.hpp>

struct IDXGISwapChain;

namespace afw::render {

using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);

class PresentBinding {
 public:
  explicit PresentBinding(const hook::VmtHook& hook) noexcept;

  PresentBinding(const PresentBinding&) = delete;
  PresentBinding& operator=(const PresentBinding&) = delete;

  PresentBinding(PresentBinding&&) = delete;
  PresentBinding& operator=(PresentBinding&&) = delete;

  ~PresentBinding();
};

[[nodiscard]] PresentFn OriginalPresent() noexcept;

}  // namespace afw::render
