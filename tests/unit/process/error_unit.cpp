#include <gtest/gtest.h>

#include <afw/process/error.hpp>
#include <system_error>

TEST(ProcessErrorUnit, ReportsAnUnknownCodeAsUnknown) {
  const std::error_code kUnknown{99, afw::process::ErrorCategory()};

  EXPECT_EQ(kUnknown.message(), "unknown afw::process error");
}
