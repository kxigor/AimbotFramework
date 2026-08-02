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
  const bool kWas{settings.espEnabled};

  settings.espEnabled = !kWas;
  EXPECT_EQ(afw::config::Current().espEnabled, !kWas);

  settings.espEnabled = kWas;
  EXPECT_EQ(afw::config::Current().espEnabled, kWas);
}
