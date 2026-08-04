#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <system_error>

namespace afw::hook {

constexpr std::size_t kMaxPatchBytes{32};

class CodePatch {
 public:
  [[nodiscard]] static std::expected<CodePatch, std::error_code> Apply(
      std::uintptr_t address, std::span<const std::uint8_t> bytes) noexcept;

  CodePatch(const CodePatch&) = delete;
  CodePatch& operator=(const CodePatch&) = delete;

  CodePatch(CodePatch&& other) noexcept;
  CodePatch& operator=(CodePatch&& other) noexcept;

  ~CodePatch();

  [[nodiscard]] std::uintptr_t Address() const noexcept { return address_; }

  void Restore() noexcept;

 private:
  CodePatch() = default;

  std::uintptr_t address_{};
  std::array<std::uint8_t, kMaxPatchBytes> original_{};
  std::size_t size_{};
};

}  // namespace afw::hook
