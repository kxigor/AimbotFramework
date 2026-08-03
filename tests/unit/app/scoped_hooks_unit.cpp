#include <gtest/gtest.h>

#include <afw/app/scoped_hooks.hpp>
#include <afw/hook/hook.hpp>
#include <expected>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace {

// A stand-in for a real hook, implementing the same afw::hook::Hook interface
// the set is written against. Every step appends to one journal, which is what
// the ordering assertions read. Only the hook carries an id: the binding
// records sit between two of its entries, so their order is unambiguous
// without one.
std::vector<std::string>& Journal() noexcept {
  static std::vector<std::string> steps;
  return steps;
}

void Record(int id, const char* step) {
  Journal().push_back(std::to_string(id) + ":" + step);
}

struct Plan {
  bool setDetourFails{false};
  bool enableFails{false};
};

void Detour() {}

class FakeHook final : public afw::hook::Hook {
 public:
  FakeHook(int id, Plan plan) noexcept : id_{id}, plan_{plan} {}

  FakeHook(const FakeHook&) = delete;
  FakeHook& operator=(const FakeHook&) = delete;

  FakeHook(FakeHook&&) = delete;
  FakeHook& operator=(FakeHook&&) = delete;

  ~FakeHook() override { Record(id_, "~hook"); }

  [[nodiscard]] std::expected<void, std::error_code> Enable() noexcept override {
    Record(id_, "enable");
    if (plan_.enableFails) {
      return std::unexpected{
          std::make_error_code(std::errc::permission_denied)};
    }

    return {};
  }

  [[nodiscard]] std::expected<void, std::error_code>
  Disable() noexcept override {
    Record(id_, "disable");

    return {};
  }

 private:
  [[nodiscard]] std::expected<void, std::error_code> SetDetourRaw(
      void* detour) noexcept override {
    Record(id_, "set");
    if (plan_.setDetourFails || detour == nullptr) {
      return std::unexpected{std::make_error_code(std::errc::invalid_argument)};
    }

    return {};
  }

  [[nodiscard]] void* OriginalRaw() const noexcept override { return nullptr; }

  int id_{};
  Plan plan_{};
};

class FakeBinding {
 public:
  explicit FakeBinding(const afw::hook::Hook& /*hook*/) noexcept {
    Journal().emplace_back("bind");
  }

  FakeBinding(const FakeBinding&) = delete;
  FakeBinding& operator=(const FakeBinding&) = delete;

  FakeBinding(FakeBinding&&) = delete;
  FakeBinding& operator=(FakeBinding&&) = delete;

  ~FakeBinding() { Journal().emplace_back("~bind"); }
};

// Returns the concrete type on purpose: the real call sites hand Add a
// std::expected<std::unique_ptr<VmtHook>, ...>, and this keeps that conversion
// under test.
[[nodiscard]] std::expected<std::unique_ptr<FakeHook>, std::error_code> AHook(
    int id, Plan plan = {}) {
  return std::make_unique<FakeHook>(id, plan);
}

[[nodiscard]] std::expected<std::unique_ptr<FakeHook>, std::error_code>
NoHook() {
  return std::unexpected{std::make_error_code(std::errc::no_such_device)};
}

}  // namespace

TEST(ScopedHooksUnit, ArmsTheHookInOrderAndDisablesBeforeTheBindingDies) {
  Journal().clear();

  {
    afw::app::ScopedHooks hooks;
    EXPECT_TRUE(hooks.Add<FakeBinding>(AHook(1), &Detour, "first"));
  }

  EXPECT_EQ(Journal(),
            (std::vector<std::string>{"1:set", "bind", "1:enable", "1:disable",
                                      "~bind", "1:~hook"}));
}

TEST(ScopedHooksUnit, TakesSeveralHooksDownInReverseOrder) {
  Journal().clear();

  {
    afw::app::ScopedHooks hooks;
    ASSERT_TRUE(hooks.Add<FakeBinding>(AHook(1), &Detour, "first"));
    ASSERT_TRUE(hooks.Add<FakeBinding>(AHook(2), &Detour, "second"));
  }

  EXPECT_EQ(Journal(),
            (std::vector<std::string>{"1:set", "bind", "1:enable", "2:set",
                                      "bind", "2:enable", "2:disable", "~bind",
                                      "2:~hook", "1:disable", "~bind",
                                      "1:~hook"}));
}

TEST(ScopedHooksUnit, RejectsAHookThatWasNeverCreated) {
  Journal().clear();

  {
    afw::app::ScopedHooks hooks;
    EXPECT_FALSE(hooks.Add<FakeBinding>(NoHook(), &Detour, "missing"));
  }

  EXPECT_TRUE(Journal().empty());
}

TEST(ScopedHooksUnit, BindsNothingWhenTheDetourIsRefused) {
  Journal().clear();

  {
    afw::app::ScopedHooks hooks;
    EXPECT_FALSE(hooks.Add<FakeBinding>(AHook(1, Plan{.setDetourFails = true}),
                                        &Detour, "first"));
  }

  EXPECT_EQ(Journal(), (std::vector<std::string>{"1:set", "1:~hook"}));
}

TEST(ScopedHooksUnit, KeepsAHookThatFailedToEnableOutOfTheScope) {
  Journal().clear();

  {
    afw::app::ScopedHooks hooks;
    EXPECT_FALSE(hooks.Add<FakeBinding>(AHook(1, Plan{.enableFails = true}),
                                        &Detour, "first"));
  }

  // No "1:disable": nothing was ever swapped in. The entry dies inside Add, so
  // the journal is already complete before the set itself goes away -- which is
  // what proves the failed hook was never kept.
  EXPECT_EQ(Journal(), (std::vector<std::string>{"1:set", "bind", "1:enable",
                                                 "~bind", "1:~hook"}));
}
