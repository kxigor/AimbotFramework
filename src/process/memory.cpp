#include <windows.h>

#include <afw/process/error.hpp>
#include <afw/process/memory.hpp>
#include <bit>
#include <cstdint>
#include <expected>
#include <system_error>

namespace afw::process {

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

  const std::uintptr_t kStart{std::bit_cast<std::uintptr_t>(address)};
  const std::uintptr_t kRegionEnd{
      std::bit_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize};

  return kStart + size <= kRegionEnd;
}

}  // namespace afw::process
