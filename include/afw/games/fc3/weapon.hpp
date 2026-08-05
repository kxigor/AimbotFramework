#pragma once

#include <afw/hook/patch.hpp>
#include <expected>
#include <system_error>

namespace afw::games::fc3 {

// Each of these finds one site by signature and overwrites a handful of bytes,
// and each undoes itself when the patch it returns is dropped. Holding one is
// what keeps it applied.
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
