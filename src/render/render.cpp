#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

#include <afw/core/cast.hpp>
#include <afw/hook/vtable.hpp>
#include <afw/render/render.hpp>
#include <afw/win/error.hpp>
#include <bit>
#include <expected>
#include <system_error>

namespace afw::render {

namespace {

constexpr PCWSTR kProbeClassName{L"afw_swap_chain_probe"};
constexpr UINT kProbeExtent{1};

[[nodiscard]] std::expected<HWND, std::error_code> CreateProbeWindow(
    HINSTANCE instance) noexcept {
  const WNDCLASSEXW kClass{.cbSize = sizeof(WNDCLASSEXW),
                           .lpfnWndProc = DefWindowProcW,
                           .hInstance = instance,
                           .lpszClassName = kProbeClassName};
  if (RegisterClassExW(
          /*unnamedParam1 = */ &kClass) == 0) {
    return std::unexpected{win::LastError()};
  }

  HWND window{CreateWindowExW(
      /*dwExStyle    = */ 0,
      /*lpClassName  = */ kProbeClassName,
      /*lpWindowName = */ nullptr,
      /*dwStyle      = */ WS_OVERLAPPEDWINDOW,
      /*X            = */ 0,
      /*Y            = */ 0,
      /*nWidth       = */ static_cast<int>(kProbeExtent),
      /*nHeight      = */ static_cast<int>(kProbeExtent),
      /*hWndParent   = */ nullptr,
      /*hMenu        = */ nullptr,
      /*hInstance    = */ instance,
      /*lpParam      = */ nullptr)};
  if (window == nullptr) {
    const std::error_code kError{win::LastError()};
    UnregisterClassW(
        /*lpClassName = */ kProbeClassName,
        /*hInstance   = */ instance);
    return std::unexpected{kError};
  }

  return window;
}

void DestroyProbeWindow(HWND window, HINSTANCE instance) noexcept {
  DestroyWindow(
      /*hWnd = */ window);
  UnregisterClassW(
      /*lpClassName = */ kProbeClassName,
      /*hInstance   = */ instance);
}

[[nodiscard]] std::expected<hook::Vtable, std::error_code> ReadVtable(
    HWND window) noexcept {
  // clang-format off
  const DXGI_SWAP_CHAIN_DESC kDesc{
      .BufferDesc   = {.Width  = kProbeExtent,
                       .Height = kProbeExtent,
                       .Format = DXGI_FORMAT_R8G8B8A8_UNORM},
      .SampleDesc   = {.Count = 1},
      .BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount  = 1,
      .OutputWindow = window,
      .Windowed     = TRUE};
  // clang-format on

  IDXGISwapChain* swapChain{nullptr};
  ID3D11Device* device{nullptr};
  ID3D11DeviceContext* context{nullptr};

  const HRESULT kResult{D3D11CreateDeviceAndSwapChain(
      /*pAdapter           = */ nullptr,
      /*DriverType         = */ D3D_DRIVER_TYPE_WARP,
      /*Software           = */ nullptr,
      /*Flags              = */ 0,
      /*pFeatureLevels     = */ nullptr,
      /*FeatureLevels      = */ 0,
      /*SDKVersion         = */ D3D11_SDK_VERSION,
      /*pSwapChainDesc     = */ &kDesc,
      /*ppSwapChain        = */ &swapChain,
      /*ppDevice           = */ &device,
      /*pFeatureLevel      = */ nullptr,
      /*ppImmediateContext = */ &context)};
  if (FAILED(kResult)) {
    return std::unexpected{std::error_code{kResult, std::system_category()}};
  }

  void** const kVtable{*core::PointerCast<void***>(swapChain)};

  context->Release();
  device->Release();
  swapChain->Release();

  return hook::Vtable{kVtable};
}

}  // namespace

std::expected<hook::Vtable, std::error_code> ProbeSwapChainVtable() noexcept {
  HINSTANCE instance{GetModuleHandleW(
      /*lpModuleName = */ nullptr)};
  if (instance == nullptr) {
    return std::unexpected{win::LastError()};
  }

  const std::expected<HWND, std::error_code> kWindow{
      CreateProbeWindow(instance)};
  if (!kWindow) {
    return std::unexpected{kWindow.error()};
  }

  const std::expected<hook::Vtable, std::error_code> kVtable{
      ReadVtable(*kWindow)};

  DestroyProbeWindow(*kWindow, instance);

  return kVtable;
}

}  // namespace afw::render
