#pragma once

#include <afw/hook/patch.hpp>
#include <expected>
#include <optional>
#include <system_error>

namespace afw::hook {

class HeldPatch {
 public:
  using Source = std::expected<CodePatch, std::error_code> (*)() noexcept;

  explicit HeldPatch(Source source) noexcept : source_{source} {}

  HeldPatch(const HeldPatch&) = delete;
  HeldPatch& operator=(const HeldPatch&) = delete;

  HeldPatch(HeldPatch&&) = delete;
  HeldPatch& operator=(HeldPatch&&) = delete;

  ~HeldPatch() = default;

  void Follow(bool wanted) noexcept;

 private:
  Source source_;
  std::optional<CodePatch> patch_;
};

}  // namespace afw::hook
