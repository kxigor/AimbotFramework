#pragma once

#include <afw/esp/internal/classify.hpp>
#include <optional>

namespace afw::esp {

// Which kinds the overlay draws, and which the aim assist will consider. Both
// are the same question -- what does the user want done with this kind -- asked
// once of the settings, and they belong together because they drifted apart
// once already: the aim assist kept its own copy of this switch inside the DLL
// shim, where a kind added to one was not added to the other.
[[nodiscard]] bool Shown(Kind kind) noexcept;

// Absent when the kind is not being targeted at all, so the caller cannot
// mistake "not wanted" for "wanted, at priority zero".
[[nodiscard]] std::optional<float> AimPriority(Kind kind) noexcept;

}  // namespace afw::esp
