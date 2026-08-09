#include <gtest/gtest.h>

#include <afw/games/fc3/error.hpp>
#include <string>
#include <system_error>

TEST(Fc3ErrorUnit, ReportsAnUnknownCodeAsUnknown) {
  const std::error_code kUnknown{99, afw::games::fc3::ErrorCategory()};

  EXPECT_EQ(kUnknown.message(), "unknown afw::games::fc3 error");
}

// The three camera codes are what the overlay puts in front of the user when
// the view cannot be read, so each has to describe the test that actually
// failed. CameraNotFound used to say the entry held nothing whose vtable
// matched the known camera class -- ReadCamera performs no vtable test at all
// and returns this when any of the five field reads misses, so the message sent
// a reader hunting through the class registry for a fault in the field reads.
TEST(Fc3ErrorUnit, DescribesWhyTheCameraWasNotFound) {
  const std::error_code kNotFound{
      make_error_code(afw::games::fc3::Error::CameraNotFound)};
  const std::string kMessage{kNotFound.message()};

  EXPECT_EQ(kMessage.find("vtable"), std::string::npos);
  EXPECT_NE(kMessage.find("field"), std::string::npos);
}

// The other two stay pinned so a later edit cannot quietly swap them: one is
// "nothing is active", the other is "it is active and the numbers are wrong".
TEST(Fc3ErrorUnit, KeepsTheOtherTwoCameraCodesDistinct) {
  const std::string kUnavailable{
      make_error_code(afw::games::fc3::Error::CameraUnavailable).message()};
  const std::string kNotSane{
      make_error_code(afw::games::fc3::Error::CameraNotSane).message()};

  EXPECT_NE(kUnavailable, kNotSane);
  EXPECT_NE(kUnavailable.find("no camera is active"), std::string::npos);
  EXPECT_NE(kNotSane.find("sanity check"), std::string::npos);
}
