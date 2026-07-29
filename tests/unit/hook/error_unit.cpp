#include <gtest/gtest.h>

#include <afw/hook/error.hpp>
#include <system_error>

TEST(HookErrorUnit, ReportsAnUnknownCodeAsUnknown) {
  const std::error_code kUnknown{99, afw::hook::ErrorCategory()};

  EXPECT_EQ(kUnknown.message(), "unknown afw::hook error");
}
