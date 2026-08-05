#pragma once

#include <afw/games/fc3/fc3.hpp>
#include <cstdint>

namespace afw::games::fc3 {

// The walk writes here and the frame that follows publishes it. Three buffers,
// so the reader keeps whatever it was handed for as long as it holds it: the
// overlay iterates one snapshot across five passes and a whole ImGui build,
// which is milliseconds, and before the triple buffer the walk wrote into the
// very array the reader was iterating.
[[nodiscard]] EntitySnapshot& WritingSnapshot() noexcept;

[[nodiscard]] EntityCensus& WritingCensus() noexcept;

// One pass is not a whole world -- the engine walks it across several -- so a
// generation accumulates until the frame that publishes it asks for a new one.
void OpenGenerationIfWanted() noexcept;

// The same entity arrives many times per generation: it sits in several update
// groups, and its components lead back to it as well.
[[nodiscard]] bool AlreadySeen(std::uintptr_t entity) noexcept;

}  // namespace afw::games::fc3
