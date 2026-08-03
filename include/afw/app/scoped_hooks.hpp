#pragma once

#include <afw/app/log.hpp>
#include <afw/hook/hook.hpp>
#include <expected>
#include <memory>
#include <optional>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

namespace afw::app {

class InstalledHook {
 public:
  InstalledHook(const InstalledHook&) = delete;
  InstalledHook& operator=(const InstalledHook&) = delete;

  InstalledHook(InstalledHook&&) = delete;
  InstalledHook& operator=(InstalledHook&&) = delete;

  virtual ~InstalledHook() = default;

 protected:
  InstalledHook() = default;
};

template <typename Binding>
class HookEntry final : public InstalledHook {
 public:
  HookEntry(const HookEntry&) = delete;
  HookEntry& operator=(const HookEntry&) = delete;

  HookEntry(HookEntry&&) = delete;
  HookEntry& operator=(HookEntry&&) = delete;

  template <typename Detour>
  HookEntry(std::unique_ptr<hook::Hook> hook, Detour detour) noexcept
      : hook_{std::move(hook)} {
    const std::expected<void, std::error_code> kSet{hook_->SetDetour(detour)};
    if (!kSet) {
      failure_ = kSet.error();
      return;
    }

    binding_.emplace(*hook_);

    const std::expected<void, std::error_code> kEnabled{hook_->Enable()};
    if (!kEnabled) {
      failure_ = kEnabled.error();
    }
  }

  ~HookEntry() override {
    if (!failure_) {
      std::ignore = hook_->Disable();
    }
  }

  [[nodiscard]] std::error_code Failure() const noexcept { return failure_; }

 private:
  std::unique_ptr<hook::Hook> hook_;
  std::optional<Binding> binding_;
  std::error_code failure_;
};

class ScopedHooks {
 public:
  ScopedHooks() = default;

  ScopedHooks(const ScopedHooks&) = delete;
  ScopedHooks& operator=(const ScopedHooks&) = delete;

  ScopedHooks(ScopedHooks&&) = delete;
  ScopedHooks& operator=(ScopedHooks&&) = delete;

  ~ScopedHooks() {
    while (!entries_.empty()) {
      entries_.pop_back();
    }
  }

  template <typename Binding, typename Detour>
  bool Add(std::expected<std::unique_ptr<hook::Hook>, std::error_code> hook,
           Detour detour, const char* what) {
    if (!hook) {
      LogFailure(what, hook.error());
      return false;
    }

    std::unique_ptr<HookEntry<Binding>> entry{
        std::make_unique<HookEntry<Binding>>(std::move(*hook), detour)};

    const std::error_code kFailure{entry->Failure()};
    if (kFailure) {
      LogFailure(what, kFailure);
      return false;
    }

    LogInstalled(what);
    entries_.push_back(std::move(entry));

    return true;
  }

 private:
  std::vector<std::unique_ptr<InstalledHook>> entries_;
};

}  // namespace afw::app
