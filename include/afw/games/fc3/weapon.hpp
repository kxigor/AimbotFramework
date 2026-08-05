#pragma once

#include <afw/hook/patch.hpp>
#include <expected>
#include <system_error>

namespace afw::games::fc3 {

[[nodiscard]] std::expected<hook::CodePatch, std::error_code>
SuppressRecoil() noexcept;

[[nodiscard]] std::expected<hook::CodePatch, std::error_code>
SuppressSpread() noexcept;

[[nodiscard]] std::expected<hook::CodePatch, std::error_code>
SuppressSway() noexcept;

[[nodiscard]] std::expected<hook::CodePatch, std::error_code>
IgnoreSpentRounds() noexcept;

[[nodiscard]] std::expected<hook::CodePatch, std::error_code>
IgnoreEmptyMagazine() noexcept;

}  // namespace afw::games::fc3
