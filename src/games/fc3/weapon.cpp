#include <afw/games/fc3/fc3.hpp>
#include <afw/games/fc3/internal/layout.hpp>
#include <afw/hook/patch.hpp>
#include <afw/process/scan.hpp>
#include <array>
#include <cstdint>
#include <expected>
#include <system_error>

namespace afw::games::fc3 {

std::expected<hook::CodePatch, std::error_code> SuppressSpread() noexcept {
  const std::expected<std::uintptr_t, std::error_code> kSite{
      process::FindPattern(kModuleName, kSpreadReadSignature)};
  if (!kSite) {
    return std::unexpected{kSite.error()};
  }

  constexpr std::array<std::uint8_t, 6> kAlwaysZero{0xD9, 0xEE, 0x90,
                                                    0x90, 0x90, 0x90};

  return hook::CodePatch::Apply(*kSite + kSpreadReadPatchOffset, kAlwaysZero);
}

}  // namespace afw::games::fc3
