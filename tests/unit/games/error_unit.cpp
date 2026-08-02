#include <gtest/gtest.h>

#include <afw/games/fc3/error.hpp>
#include <system_error>

TEST(Fc3ErrorUnit, ReportsAnUnknownCodeAsUnknown) {
  const std::error_code kUnknown{99, afw::games::fc3::ErrorCategory()};

  EXPECT_EQ(kUnknown.message(), "unknown afw::games::fc3 error");
}
