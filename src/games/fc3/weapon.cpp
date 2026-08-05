#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/layout/module.hpp>
#include <afw/games/fc3/internal/signatures.hpp>
#include <afw/hook/patch.hpp>
#include <afw/process/scan.hpp>
#include <array>
#include <cstdint>
#include <expected>
#include <span>
#include <system_error>

namespace afw::games::fc3 {

namespace {

// Every one of these finds its site by signature rather than by address, so a
// patched or relocated image either matches or reports that it did not, and
// never writes over something else.
[[nodiscard]] std::expected<hook::CodePatch, std::error_code> PatchAt(
    const char* signature, std::uintptr_t offset,
    std::span<const std::uint8_t> bytes) noexcept {
  const std::expected<std::uintptr_t, std::error_code> kSite{
      process::FindPattern(kModuleName, signature)};
  if (!kSite) {
    return std::unexpected{kSite.error()};
  }

  return hook::CodePatch::Apply(*kSite + offset, bytes);
}

}  // namespace

std::expected<hook::CodePatch, std::error_code> SuppressRecoil() noexcept {
  constexpr std::array<std::uint8_t, 5> kSkip{0x83, 0xC4, 0x04, 0x90, 0x90};

  return PatchAt(kRecoilStoreSignature, kRecoilStorePatchOffset, kSkip);
}

std::expected<hook::CodePatch, std::error_code> SuppressSway() noexcept {
  constexpr std::array<std::uint8_t, 5> kCopyNothing{0x90, 0x90, 0x90, 0x90,
                                                     0x90};

  return PatchAt(kSwayCopySignature, kSwayCopyPatchOffset, kCopyNothing);
}

std::expected<hook::CodePatch, std::error_code> SuppressSpread() noexcept {
  constexpr std::array<std::uint8_t, 6> kAlwaysZero{0xD9, 0xEE, 0x90,
                                                    0x90, 0x90, 0x90};

  return PatchAt(kSpreadReadSignature, kSpreadReadPatchOffset, kAlwaysZero);
}

std::expected<hook::CodePatch, std::error_code> IgnoreSpentRounds() noexcept {
  constexpr std::array<std::uint8_t, 2> kSkipTheFall{0x7C, 0x09};

  return PatchAt(kAmmoStoreSignature, kAmmoStorePatchOffset, kSkipTheFall);
}

std::expected<hook::CodePatch, std::error_code> IgnoreEmptyMagazine() noexcept {
  constexpr std::array<std::uint8_t, 7> kNeverEmpty{0x90, 0x90, 0x90, 0x90,
                                                    0x90, 0x90, 0x90};

  return PatchAt(kAmmoCheckSignature, kAmmoCheckPatchOffset, kNeverEmpty);
}

}  // namespace afw::games::fc3
