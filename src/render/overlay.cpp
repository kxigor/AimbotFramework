#include <d3d11.h>
#include <dxgi.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <windows.h>

#include <afw/core/cast.hpp>
#include <afw/render/overlay.hpp>
#include <afw/win/com_ptr.hpp>
#include <array>
#include <bit>
#include <expected>
#include <system_error>

namespace afw::render {

namespace {

template <typename T>
[[nodiscard]] void** AsVoid(T** slot) noexcept {
  return core::PointerCast<void**>(slot);
}

[[nodiscard]] std::error_code FromHresult(HRESULT result) noexcept {
  return std::error_code{result, std::system_category()};
}

struct Overlay {
  win::ComPtr<ID3D11Device> device;
  win::ComPtr<ID3D11DeviceContext> context;
  SurfaceSize backBuffer{};
  HWND window{nullptr};
  bool ready{false};
};

Overlay& State() noexcept {
  static Overlay overlay{};
  return overlay;
}

[[nodiscard]] std::expected<void, std::error_code> Initialize(
    IDXGISwapChain* swapChain) noexcept {
  DXGI_SWAP_CHAIN_DESC desc{};
  const HRESULT kDesc{swapChain->GetDesc(
      /*pDesc = */ &desc)};
  if (FAILED(kDesc)) {
    return std::unexpected{FromHresult(kDesc)};
  }

  Overlay& overlay{State()};

  const HRESULT kDevice{swapChain->GetDevice(
      /*riid      = */ IID_ID3D11Device,
      /*ppDevice  = */ AsVoid(overlay.device.Put()))};
  if (FAILED(kDevice)) {
    return std::unexpected{FromHresult(kDevice)};
  }

  overlay.device->GetImmediateContext(
      /*ppImmediateContext = */ overlay.context.Put());

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  if (!ImGui_ImplWin32_Init(desc.OutputWindow) ||
      !ImGui_ImplDX11_Init(overlay.device.Get(), overlay.context.Get())) {
    return std::unexpected{FromHresult(E_FAIL)};
  }

  overlay.window = desc.OutputWindow;
  overlay.ready = true;

  return {};
}

[[nodiscard]] std::expected<void, std::error_code> BindBackBuffer(
    IDXGISwapChain* swapChain) noexcept {
  Overlay& overlay{State()};

  win::ComPtr<ID3D11Texture2D> backBuffer{};
  const HRESULT kBuffer{swapChain->GetBuffer(
      /*Buffer    = */ 0,
      /*riid      = */ IID_ID3D11Texture2D,
      /*ppSurface = */ AsVoid(backBuffer.Put()))};
  if (FAILED(kBuffer)) {
    return std::unexpected{FromHresult(kBuffer)};
  }

  D3D11_TEXTURE2D_DESC surface{};
  backBuffer->GetDesc(
      /*pDesc = */ &surface);
  overlay.backBuffer =
      SurfaceSize{.width = surface.Width, .height = surface.Height};

  win::ComPtr<ID3D11RenderTargetView> target{};
  const HRESULT kTarget{overlay.device->CreateRenderTargetView(
      /*pResource = */ backBuffer.Get(),
      /*pDesc     = */ nullptr,
      /*ppRTView  = */ target.Put())};
  if (FAILED(kTarget)) {
    return std::unexpected{FromHresult(kTarget)};
  }

  const std::array<ID3D11RenderTargetView*, 1> kViews{target.Get()};
  overlay.context->OMSetRenderTargets(
      /*NumViews            = */ 1,
      /*ppRenderTargetViews = */ kViews.data(),
      /*pDepthStencilView   = */ nullptr);

  return {};
}

void BuildFrame(FrameCallback onFrame) {
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  if (onFrame != nullptr) {
    onFrame();
  }

  ImGui::Render();
}

}  // namespace

SurfaceSize BackBufferSize() noexcept { return State().backBuffer; }

HWND OverlayWindow() noexcept { return State().window; }

void ShutdownOverlay() noexcept {
  Overlay& overlay{State()};
  if (!overlay.ready) {
    return;
  }
  overlay.ready = false;

  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  overlay.context.Reset();
  overlay.device.Reset();
  overlay.window = nullptr;
}

std::expected<void, std::error_code> DrawOverlayFrame(
    IDXGISwapChain* swapChain, FrameCallback onFrame) noexcept {
  if (swapChain == nullptr) {
    return std::unexpected{FromHresult(E_POINTER)};
  }

  if (!State().ready) {
    const std::expected<void, std::error_code> kStarted{Initialize(swapChain)};
    if (!kStarted) {
      return std::unexpected{kStarted.error()};
    }
  }

  BuildFrame(onFrame);

  const std::expected<void, std::error_code> kBound{BindBackBuffer(swapChain)};
  if (!kBound) {
    return std::unexpected{kBound.error()};
  }

  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

  return {};
}

}  // namespace afw::render
