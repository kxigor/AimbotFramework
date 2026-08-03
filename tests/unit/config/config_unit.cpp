#include <gtest/gtest.h>

#include <afw/config/config.hpp>

TEST(ConfigUnit, StartsWithTheEspOnAndNothingPending) {
  const afw::config::Settings kDefaults{};

  EXPECT_TRUE(kDefaults.espEnabled);
  EXPECT_TRUE(kDefaults.menuVisible);
  EXPECT_FALSE(kDefaults.unloadRequested);
}

TEST(ConfigUnit, HandsOutOneSharedInstance) {
  afw::config::Settings& settings{afw::config::Current()};
  const bool kWas{settings.espEnabled.load()};

  settings.espEnabled.store(!kWas);
  EXPECT_EQ(afw::config::Current().espEnabled.load(), !kWas);

  settings.espEnabled.store(kWas);
  EXPECT_EQ(afw::config::Current().espEnabled.load(), kWas);
}

// The engine's update thread reads this toggle while the render thread writes
// it from the menu, so a plain bool would be a data race by the standard even
// where the hardware never tears it.
TEST(ConfigUnit, TheEspToggleIsSafeToShareBetweenThreads) {
  EXPECT_TRUE(afw::config::Current().espEnabled.is_lock_free());
}
