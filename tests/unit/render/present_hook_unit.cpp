#include <gtest/gtest.h>
#include <windows.h>

#include <afw/hook/vmt_hook.hpp>
#include <afw/hook/vtable.hpp>
#include <afw/render/present_hook.hpp>
#include <afw/render/render.hpp>
#include <array>
#include <bit>
#include <expected>
#include <system_error>

namespace {

constexpr std::size_t kEntries{16};

int detourCalls{0};

HRESULT __stdcall FakeOriginal(IDXGISwapChain* /*swapChain*/,
                               UINT /*syncInterval*/, UINT /*flags*/) {
  return S_OK;
}

HRESULT __stdcall FakeDetour(IDXGISwapChain* swapChain, UINT syncInterval,
                             UINT flags) {
  ++detourCalls;

  afw::render::PresentFn original{afw::render::OriginalPresent()};
  if (original == nullptr) {
    return E_FAIL;
  }

  return original(swapChain, syncInterval, flags);
}

class PresentHookUnit : public ::testing::Test {
 protected:
  void SetUp() override {
    detourCalls = 0;
    entries_.fill(nullptr);
    entries_[afw::render::kPresentSlot] = std::bit_cast<void*>(&FakeOriginal);
  }

  [[nodiscard]] afw::hook::Vtable Table() {
    return afw::hook::Vtable{entries_.data()};
  }

  [[nodiscard]] HRESULT CallPresentSlot() {
    return std::bit_cast<afw::render::PresentFn>(
        entries_[afw::render::kPresentSlot])(nullptr, 0, 0);
  }

  std::array<void*, kEntries> entries_{};
};

}  // namespace

TEST_F(PresentHookUnit, OriginalPresentIsNullWithoutABinding) {
  EXPECT_EQ(afw::render::OriginalPresent(), nullptr);
}

TEST_F(PresentHookUnit, BindingPublishesTheOriginal) {
  std::expected<afw::hook::VmtHook, std::error_code> hook{
      afw::hook::VmtHook::Create(Table(), afw::render::kPresentSlot)};
  ASSERT_TRUE(hook.has_value()) << hook.error().message();

  const afw::render::PresentBinding kBound{*hook};

  EXPECT_EQ(afw::render::OriginalPresent(), &FakeOriginal);
}

TEST_F(PresentHookUnit, DestroyingTheBindingClearsIt) {
  std::expected<afw::hook::VmtHook, std::error_code> hook{
      afw::hook::VmtHook::Create(Table(), afw::render::kPresentSlot)};
  ASSERT_TRUE(hook.has_value()) << hook.error().message();

  {
    const afw::render::PresentBinding kBound{*hook};
    ASSERT_EQ(afw::render::OriginalPresent(), &FakeOriginal);
  }

  EXPECT_EQ(afw::render::OriginalPresent(), nullptr);
}

TEST_F(PresentHookUnit, TheDetourReachesTheOriginalThroughTheBinding) {
  std::expected<afw::hook::VmtHook, std::error_code> hook{
      afw::hook::VmtHook::Create(Table(), afw::render::kPresentSlot)};
  ASSERT_TRUE(hook.has_value()) << hook.error().message();
  ASSERT_TRUE(hook->SetDetour(&FakeDetour).has_value());

  const afw::render::PresentBinding kBound{*hook};
  ASSERT_TRUE(hook->Enable().has_value());

  EXPECT_EQ(CallPresentSlot(), S_OK);
  EXPECT_EQ(detourCalls, 1);
}

TEST_F(PresentHookUnit, ASecondBindingWorksAfterTheFirstIsGone) {
  for (int round{0}; round < 2; ++round) {
    std::expected<afw::hook::VmtHook, std::error_code> hook{
        afw::hook::VmtHook::Create(Table(), afw::render::kPresentSlot)};
    ASSERT_TRUE(hook.has_value()) << hook.error().message();
    ASSERT_TRUE(hook->SetDetour(&FakeDetour).has_value());

    const afw::render::PresentBinding kBound{*hook};
    ASSERT_TRUE(hook->Enable().has_value());

    EXPECT_EQ(CallPresentSlot(), S_OK) << "round " << round;
  }

  EXPECT_EQ(detourCalls, 2);
  EXPECT_EQ(afw::render::OriginalPresent(), nullptr);
  EXPECT_EQ(entries_[afw::render::kPresentSlot],
            std::bit_cast<void*>(&FakeOriginal));
}
