#pragma once

#include <afw/app/game.hpp>
#include <afw/app/scoped_hooks.hpp>

namespace afw::dll {

class FarCry3 final : public app::Game {
 public:
  [[nodiscard]] const char* Name() const noexcept override;

  void InstallHooks(app::ScopedHooks& hooks) override;

  void OnFrame() noexcept override;
};

}  // namespace afw::dll
