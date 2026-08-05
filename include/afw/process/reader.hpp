#pragma once

#include <afw/process/memory.hpp>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace afw::process {

template <typename EnginePointer>
concept ForeignPointer = std::is_unsigned_v<EnginePointer> &&
                         (sizeof(EnginePointer) <= sizeof(std::uintptr_t));

template <ForeignPointer EnginePointer>
class Reader {
 public:
  explicit Reader(std::uintptr_t base) noexcept : base_{base} {}

  [[nodiscard]] std::uintptr_t Base() const noexcept { return base_; }

  template <typename T>
  [[nodiscard]] std::optional<T> Field(std::uintptr_t offset) const noexcept {
    const std::expected<T, std::error_code> kValue{ReadAt<T>(base_ + offset)};
    if (!kValue) {
      return std::nullopt;
    }

    return *kValue;
  }

  template <typename T>
  [[nodiscard]] T FieldOr(std::uintptr_t offset, T fallback) const noexcept {
    return ReadAt<T>(base_ + offset).value_or(fallback);
  }

  [[nodiscard]] std::optional<std::uintptr_t> Pointer(
      std::uintptr_t offset) const noexcept {
    const std::optional<EnginePointer> kValue{Field<EnginePointer>(offset)};
    if (!kValue || *kValue == 0) {
      return std::nullopt;
    }

    return static_cast<std::uintptr_t>(*kValue);
  }

  [[nodiscard]] std::optional<std::uint32_t> CountAtMost(
      std::uintptr_t offset, std::uint32_t limit) const noexcept {
    const std::optional<std::uint32_t> kCount{Field<std::uint32_t>(offset)};
    if (!kCount || *kCount > limit) {
      return std::nullopt;
    }

    return kCount;
  }

 private:
  std::uintptr_t base_{};
};

template <ForeignPointer EnginePointer>
class PointerArray {
 public:
  [[nodiscard]] static std::optional<PointerArray> At(
      std::uintptr_t owner, std::uintptr_t arrayOffset,
      std::uintptr_t countOffset, std::uint32_t limit) noexcept {
    const Reader<EnginePointer> kOwner{owner};
    const std::optional<std::uintptr_t> kBase{kOwner.Pointer(arrayOffset)};
    const std::optional<std::uint32_t> kCount{
        kOwner.CountAtMost(countOffset, limit)};
    if (!kBase || !kCount) {
      return std::nullopt;
    }

    return PointerArray{*kBase, *kCount};
  }

  [[nodiscard]] std::uint32_t Size() const noexcept { return count_; }

  [[nodiscard]] std::optional<std::uintptr_t> Slot(
      std::uint32_t index) const noexcept {
    return Reader<EnginePointer>{base_}.Pointer(index * sizeof(EnginePointer));
  }

 private:
  PointerArray(std::uintptr_t base, std::uint32_t count) noexcept
      : base_{base}, count_{count} {}

  std::uintptr_t base_{};
  std::uint32_t count_{};
};

}  // namespace afw::process
