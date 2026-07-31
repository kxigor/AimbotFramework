#include <gtest/gtest.h>
#include <windows.h>

#include <afw/hook/vtable.hpp>
#include <afw/render/render.hpp>
#include <expected>
#include <system_error>

namespace {

[[nodiscard]] LPVOID AllocationBaseOf(LPCVOID addr) {
  MEMORY_BASIC_INFORMATION info{};
  const SIZE_T kQueried{VirtualQuery(
      /*lpAddress = */ addr,
      /*lpBuffer  = */ &info,
      /*dwLength  = */ sizeof(info))};
  return kQueried == sizeof(info) ? info.AllocationBase : nullptr;
}

}  // namespace

TEST(RenderUnit, ReturnsATableThatLivesInsideDxgi) {
  const std::expected<afw::hook::Vtable, std::error_code> kVtable{
      afw::render::ProbeSwapChainVtable()};
  ASSERT_TRUE(kVtable.has_value()) << kVtable.error().message();
  ASSERT_NE(kVtable->Get(), nullptr);

  const HMODULE kDxgi{GetModuleHandleW(/*lpModuleName = */ L"dxgi.dll")};
  ASSERT_NE(kDxgi, nullptr);

  EXPECT_EQ(AllocationBaseOf(kVtable->Get()), static_cast<LPVOID>(kDxgi));
}

TEST(RenderUnit, PresentSlotPointsAtCodeInsideDxgi) {
  const std::expected<afw::hook::Vtable, std::error_code> kVtable{
      afw::render::ProbeSwapChainVtable()};
  ASSERT_TRUE(kVtable.has_value()) << kVtable.error().message();

  void* const kPresent{kVtable->At(afw::render::kPresentSlot)};
  ASSERT_NE(kPresent, nullptr);

  const HMODULE kDxgi{GetModuleHandleW(/*lpModuleName = */ L"dxgi.dll")};
  ASSERT_NE(kDxgi, nullptr);

  EXPECT_EQ(AllocationBaseOf(kPresent), static_cast<LPVOID>(kDxgi));
}

TEST(RenderUnit, TwoProbesAgreeOnTheAddress) {
  // The whole design rests on the table belonging to the class rather than to
  // the object. If that ever stops holding, hooking a throwaway swap chain
  // would no longer affect the game's, and this is what would notice.
  const std::expected<afw::hook::Vtable, std::error_code> kFirst{
      afw::render::ProbeSwapChainVtable()};
  ASSERT_TRUE(kFirst.has_value()) << kFirst.error().message();

  const std::expected<afw::hook::Vtable, std::error_code> kSecond{
      afw::render::ProbeSwapChainVtable()};
  ASSERT_TRUE(kSecond.has_value()) << kSecond.error().message();

  EXPECT_EQ(kFirst->Get(), kSecond->Get());
}

TEST(RenderUnit, LeavesNoProbeWindowClassBehind) {
  const std::expected<afw::hook::Vtable, std::error_code> kVtable{
      afw::render::ProbeSwapChainVtable()};
  ASSERT_TRUE(kVtable.has_value()) << kVtable.error().message();

  // A second run would fail at RegisterClassExW with ERROR_CLASS_ALREADY_EXISTS
  // if the first had not unregistered its class.
  const std::expected<afw::hook::Vtable, std::error_code> kAgain{
      afw::render::ProbeSwapChainVtable()};

  EXPECT_TRUE(kAgain.has_value()) << kAgain.error().message();
}
