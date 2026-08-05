#pragma once

#include <afw/games/fc3/internal/limits.hpp>
#include <afw/games/fc3/internal/structs.hpp>
#include <array>
#include <cstdint>

namespace afw::games::fc3 {

// The engine keeps no RTTI: class identity lives in a registry, and the only
// part of it that needs the engine is obtaining an object's class descriptor,
// which is virtual slot 1. Everything after that is a walk of an array of
// ancestor name hashes.
//
// One probe per class this framework asks about. A probe is its class
// definition plus the answers it has already given, filed against the vtable
// they came from -- so the virtual call happens once per class rather than
// once per object, which is the difference between asking a question and
// asking it four thousand times a frame.
class ClassProbe {
 public:
  explicit ClassProbe(std::uintptr_t classDef) noexcept;

  // False for an object of another class, and true when the game module is
  // absent -- which is every unit test, where the call can never be made and
  // refusing everything would leave the walk with nothing to walk.
  [[nodiscard]] bool Matches(std::uintptr_t address) noexcept;

 private:
  struct Verdict {
    std::uintptr_t vtable{};
    bool matches{};
  };

  std::uintptr_t classDef_;
  std::array<Verdict, kClassCacheSlots> slots_{};
  std::uint32_t used_{};
};

[[nodiscard]] ClassProbe& EntityProbe() noexcept;

// Siblings in the registry, and the engine's own discriminator between them:
// CastToCPawnAgent fails on an animal and CastToCAnimalAgentFC3 on a human.
[[nodiscard]] ClassProbe& AgentProbe(AgentKind kind) noexcept;

}  // namespace afw::games::fc3
