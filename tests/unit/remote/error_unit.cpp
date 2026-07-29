#include <gtest/gtest.h>

#include <afw/remote/error.hpp>
#include <system_error>

TEST(RemoteErrorUnit, ReportsAnUnknownCodeAsUnknown) {
  const std::error_code kUnknown{99, afw::remote::ErrorCategory()};

  EXPECT_EQ(kUnknown.message(), "unknown afw::remote error");
}
