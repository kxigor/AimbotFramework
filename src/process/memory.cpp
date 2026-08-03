#include <windows.h>

#include <afw/process/error.hpp>
#include <afw/process/memory.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <system_error>

namespace afw::process {

namespace {

struct Region {
  std::uintptr_t begin{};
  std::uintptr_t end{};
};

constexpr std::size_t kCacheSlots{8};

[[nodiscard]] std::array<Region, kCacheSlots>& Cache() noexcept {
  thread_local std::array<Region, kCacheSlots> cache{};
  return cache;
}

[[nodiscard]] std::size_t& NextSlot() noexcept {
  thread_local std::size_t slot{0};
  return slot;
}

[[nodiscard]] bool CoveredByCache(std::uintptr_t begin,
                                  std::uintptr_t end) noexcept {
  return std::ranges::any_of(Cache(), [begin, end](const Region& region) {
    return begin >= region.begin && end <= region.end;
  });
}

void Remember(std::uintptr_t begin, std::uintptr_t end) noexcept {
  Cache().at(NextSlot()) = Region{.begin = begin, .end = end};
  NextSlot() = (NextSlot() + 1) % kCacheSlots;
}

}  // namespace

void ForgetReadableRegions() noexcept {
  Cache().fill(Region{});
  NextSlot() = 0;
}

std::expected<std::uintptr_t, std::error_code> ModuleBase(
    PCWSTR name) noexcept {
  const HMODULE kModule{GetModuleHandleW(
      /*lpModuleName = */ name)};
  if (kModule == nullptr) {
    return std::unexpected{make_error_code(Error::ModuleNotFound)};
  }

  return std::bit_cast<std::uintptr_t>(kModule);
}

bool IsReadable(LPCVOID address, SIZE_T size) noexcept {
  if (address == nullptr || size == 0) {
    return false;
  }

  const std::uintptr_t kBegin{std::bit_cast<std::uintptr_t>(address)};
  const std::uintptr_t kEnd{kBegin + size};
  // A range that wraps past the end of the address space would otherwise leave
  // kEnd below kBegin, and every cached region then covers it.
  if (kEnd < kBegin) {
    return false;
  }
  if (CoveredByCache(kBegin, kEnd)) {
    return true;
  }

  MEMORY_BASIC_INFORMATION info{};
  const SIZE_T kQueried{VirtualQuery(
      /*lpAddress = */ address,
      /*lpBuffer  = */ &info,
      /*dwLength  = */ sizeof(info))};
  if (kQueried != sizeof(info)) {
    return false;
  }
  if (info.State != MEM_COMMIT || info.Protect == PAGE_NOACCESS ||
      (info.Protect & PAGE_GUARD) != 0) {
    return false;
  }

  const std::uintptr_t kRegionBegin{
      std::bit_cast<std::uintptr_t>(info.BaseAddress)};
  const std::uintptr_t kRegionEnd{kRegionBegin + info.RegionSize};
  if (kEnd > kRegionEnd) {
    return false;
  }

  Remember(kRegionBegin, kRegionEnd);

  return true;
}

}  // namespace afw::process
