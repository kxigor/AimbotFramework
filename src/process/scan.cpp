#include <windows.h>

#include <afw/process/error.hpp>
#include <afw/process/memory.hpp>
#include <afw/process/scan.hpp>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string_view>
#include <system_error>

namespace afw::process {

namespace {

constexpr std::size_t kMaxPatternBytes{128};
constexpr std::uint16_t kAnyByte{0x100};
constexpr int kHexBase{16};
constexpr int kFirstLetterValue{10};

struct Pattern {
  std::array<std::uint16_t, kMaxPatternBytes> bytes{};
  std::size_t size{};
};

[[nodiscard]] std::expected<int, std::error_code> HexDigit(
    char digit) noexcept {
  if (digit >= '0' && digit <= '9') {
    return digit - '0';
  }
  if (digit >= 'a' && digit <= 'f') {
    return (digit - 'a') + kFirstLetterValue;
  }
  if (digit >= 'A' && digit <= 'F') {
    return (digit - 'A') + kFirstLetterValue;
  }

  return std::unexpected{make_error_code(Error::BadPattern)};
}

[[nodiscard]] std::expected<Pattern, std::error_code> Parse(
    std::string_view text) noexcept {
  Pattern pattern{};

  for (std::size_t at{0}; at < text.size();) {
    if (text.at(at) == ' ') {
      ++at;
      continue;
    }
    if (pattern.size >= kMaxPatternBytes) {
      return std::unexpected{make_error_code(Error::BadPattern)};
    }

    if (text.at(at) == '?') {
      ++at;
      if (at < text.size() && text.at(at) == '?') {
        ++at;
      }
      pattern.bytes.at(pattern.size) = kAnyByte;
      ++pattern.size;
      continue;
    }

    if (at + 1 >= text.size()) {
      return std::unexpected{make_error_code(Error::BadPattern)};
    }
    const std::expected<int, std::error_code> kHigh{HexDigit(text.at(at))};
    const std::expected<int, std::error_code> kLow{HexDigit(text.at(at + 1))};
    if (!kHigh || !kLow) {
      return std::unexpected{make_error_code(Error::BadPattern)};
    }

    pattern.bytes.at(pattern.size) =
        static_cast<std::uint16_t>((*kHigh * kHexBase) + *kLow);
    ++pattern.size;
    at += 2;
  }

  if (pattern.size == 0) {
    return std::unexpected{make_error_code(Error::BadPattern)};
  }

  return pattern;
}

[[nodiscard]] std::expected<std::size_t, std::error_code> ImageSize(
    std::uintptr_t base) noexcept {
  const std::expected<IMAGE_DOS_HEADER, std::error_code> kDos{
      ReadAt<IMAGE_DOS_HEADER>(base)};
  if (!kDos || kDos->e_magic != IMAGE_DOS_SIGNATURE) {
    return std::unexpected{make_error_code(Error::ModuleNotFound)};
  }

  const std::expected<IMAGE_NT_HEADERS, std::error_code> kNt{
      ReadAt<IMAGE_NT_HEADERS>(base +
                               static_cast<std::uintptr_t>(kDos->e_lfanew))};
  if (!kNt || kNt->Signature != IMAGE_NT_SIGNATURE) {
    return std::unexpected{make_error_code(Error::ModuleNotFound)};
  }

  return kNt->OptionalHeader.SizeOfImage;
}

[[nodiscard]] bool MatchesAt(const std::uint8_t* start,
                             const Pattern& pattern) noexcept {
  for (std::size_t index{0}; index < pattern.size; ++index) {
    const std::uint16_t kWanted{pattern.bytes.at(index)};
    if (kWanted != kAnyByte &&
        kWanted !=
            static_cast<std::uint16_t>(
                start
                    [index])) {  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      return false;
    }
  }

  return true;
}

}  // namespace

std::expected<std::uintptr_t, std::error_code> FindPattern(
    PCWSTR module, std::string_view pattern) noexcept {
  const std::expected<Pattern, std::error_code> kWanted{Parse(pattern)};
  if (!kWanted) {
    return std::unexpected{kWanted.error()};
  }

  const std::expected<std::uintptr_t, std::error_code> kBase{
      ModuleBase(module)};
  if (!kBase) {
    return std::unexpected{kBase.error()};
  }

  const std::expected<std::size_t, std::error_code> kSize{ImageSize(*kBase)};
  if (!kSize || *kSize <= kWanted->size) {
    return std::unexpected{make_error_code(Error::ModuleNotFound)};
  }

  const std::size_t kLast{*kSize - kWanted->size};
  for (std::size_t offset{0}; offset <= kLast; ++offset) {
    const std::uintptr_t kAt{*kBase + offset};
    if (!IsReadable(std::bit_cast<LPCVOID>(kAt), kWanted->size)) {
      continue;
    }
    if (MatchesAt(std::bit_cast<const std::uint8_t*>(kAt), *kWanted)) {
      return kAt;
    }
  }

  return std::unexpected{make_error_code(Error::PatternNotFound)};
}

}  // namespace afw::process
