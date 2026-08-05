#include <afw/config/config.hpp>
#include <afw/games/fc3/fc3.hpp>
#include <afw/hook/held_patch.hpp>

#include "internal.hpp"

namespace afw::dll {

void SteadyAim() noexcept {
  static hook::HeldPatch recoil{&games::fc3::SuppressRecoil};
  static hook::HeldPatch spread{&games::fc3::SuppressSpread};
  static hook::HeldPatch sway{&games::fc3::SuppressSway};
  static hook::HeldPatch spentRounds{&games::fc3::IgnoreSpentRounds};
  static hook::HeldPatch emptyMagazine{&games::fc3::IgnoreEmptyMagazine};

  const config::Settings& kSettings{config::Current()};
  recoil.Follow(kSettings.noRecoil.load());
  spread.Follow(kSettings.noSpread.load());
  sway.Follow(kSettings.noSway.load());
  spentRounds.Follow(kSettings.infiniteAmmo.load());
  emptyMagazine.Follow(kSettings.infiniteAmmo.load());
}

}  // namespace afw::dll
