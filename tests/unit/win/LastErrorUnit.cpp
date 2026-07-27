// clang-format off
#include <windows.h>

#include <gtest/gtest.h>

#include <afw/win/error.hpp>

#include <system_error>

TEST(LastErrorUnit, WrapsTheThreadsLastWin32ErrorInSystemCategory) {
  SetLastError(ERROR_ACCESS_DENIED);

  const std::error_code kCode{afw::win::LastError()};

  EXPECT_EQ(kCode.value(), static_cast<int>(ERROR_ACCESS_DENIED));
  EXPECT_EQ(&kCode.category(), &std::system_category());
}

TEST(LastErrorUnit, ReportsSuccessAsAFalsyCode) {
  SetLastError(ERROR_SUCCESS);

  EXPECT_FALSE(static_cast<bool>(afw::win::LastError()));
}
