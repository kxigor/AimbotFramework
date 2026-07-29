#include <gtest/gtest.h>
#include <windows.h>

#include <afw/remote/remote.hpp>
#include <array>
#include <cwctype>
#include <expected>
#include <filesystem>
#include <string>
#include <system_error>

namespace {

[[nodiscard]] std::wstring OwnImageName() {
  std::array<WCHAR, MAX_PATH> buffer{};
  const DWORD kLength{GetModuleFileNameW(
      /*hModule    = */ nullptr,
      /*lpFilename = */ buffer.data(),
      /*nSize      = */ static_cast<DWORD>(MAX_PATH))};
  if (kLength == 0 || kLength >= static_cast<DWORD>(MAX_PATH)) {
    return {};
  }
  return std::filesystem::path{buffer.data()}.filename().wstring();
}

[[nodiscard]] std::wstring ToUpper(std::wstring text) {
  for (wchar_t& character : text) {
    character =
        static_cast<wchar_t>(std::towupper(static_cast<wint_t>(character)));
  }
  return text;
}

}  // namespace

TEST(RemoteLookupUnit, FindsThisProcessByItsOwnImageName) {
  const std::wstring kName{OwnImageName()};
  ASSERT_FALSE(kName.empty());

  const std::expected<DWORD, std::error_code> kResult{
      afw::remote::FindProcessByImageName(kName.c_str())};

  ASSERT_TRUE(kResult.has_value()) << kResult.error().message();
  EXPECT_EQ(*kResult, GetCurrentProcessId());
}

TEST(RemoteLookupUnit, MatchesTheImageNameCaseInsensitively) {
  const std::wstring kShouted{ToUpper(OwnImageName())};
  ASSERT_FALSE(kShouted.empty());

  const std::expected<DWORD, std::error_code> kResult{
      afw::remote::FindProcessByImageName(kShouted.c_str())};

  ASSERT_TRUE(kResult.has_value()) << kResult.error().message();
  EXPECT_EQ(*kResult, GetCurrentProcessId());
}

TEST(RemoteLookupUnit, ReportsNoSuchProcessForAnUnknownName) {
  constexpr PCWSTR kAbsentName{L"afw:no*such<process>.exe"};

  const std::expected<DWORD, std::error_code> kResult{
      afw::remote::FindProcessByImageName(kAbsentName)};

  ASSERT_FALSE(kResult.has_value());
  EXPECT_EQ(kResult.error(), std::make_error_code(std::errc::no_such_process));
  EXPECT_EQ(&kResult.error().category(), &std::generic_category())
      << "category was: " << kResult.error().category().name();
}
