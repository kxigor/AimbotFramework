#include <afw/hook/held_patch.hpp>
#include <expected>
#include <system_error>
#include <utility>

namespace afw::hook {

void HeldPatch::Follow(bool wanted) noexcept {
  if (wanted == patch_.has_value()) {
    return;
  }
  if (!wanted) {
    patch_.reset();
    return;
  }

  std::expected<CodePatch, std::error_code> applied{source_()};
  if (applied) {
    patch_.emplace(std::move(*applied));
  }
}

}  // namespace afw::hook
