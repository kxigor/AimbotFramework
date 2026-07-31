#include <d3d11.h>
#include <dxgi.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <windows.h>

#include <afw/render/overlay.hpp>
#include <afw/win/com_ptr.hpp>
#include <array>
#include <expected>
#include <system_error>

namespace afw::render {

namespace {

template <typename T>
[[nodiscard]] void** AsVoid(T** slot) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<void**>(slot);
}

[[nodiscard]] std::error_code FromHresult(HRESULT result) noexcept {
  return std::error_code{result, std::system_category()};
}

struct Overlay {
  win::ComPtr<ID3D11Device> device;
  win::ComPtr<ID3D11DeviceContext> context;
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

  overlay.ready = true;

  return {};
}

[[nodiscard]] std::expected<void, std::error_code> BindBackBuffer(
    IDXGISwapChain* swapChain) noexcept {
  Overlay& overlay{State()};

  win::ComPtr<ID3D11Texture2D> backBuffer{};
  const HRESULT kBuffer{swapChain->GetBuffer(
      /*Buffer  = */ 0,
      /*riid    = */ IID_ID3D11Texture2D,
      /*ppSurface = */ AsVoid(backBuffer.Put()))};
  if (FAILED(kBuffer)) {
    return std::unexpected{FromHresult(kBuffer)};
  }

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

void BuildFrame() {
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("AimbotFramework");
  ImGui::Text("M3 -- first pixel");
  ImGui::Text("%.1f FPS", static_cast<double>(ImGui::GetIO().Framerate));
  ImGui::End();

  ImGui::Render();
}

}  // namespace

std::expected<void, std::error_code> DrawOverlayFrame(
    IDXGISwapChain* swapChain) noexcept {
  if (swapChain == nullptr) {
    return std::unexpected{FromHresult(E_POINTER)};
  }

  if (!State().ready) {
    const std::expected<void, std::error_code> kStarted{Initialize(swapChain)};
    if (!kStarted) {
      return std::unexpected{kStarted.error()};
    }
  }

  BuildFrame();

  const std::expected<void, std::error_code> kBound{BindBackBuffer(swapChain)};
  if (!kBound) {
    return std::unexpected{kBound.error()};
  }

  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

  return {};
}

}  // namespace afw::render
