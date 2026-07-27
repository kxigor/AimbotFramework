#include <windows.h>

#include <afw/win/remote_memory.hpp>

namespace afw::win {

RemoteMemory::RemoteMemory(HANDLE proc, LPVOID addr)
    : proc_{proc}, addr_{addr} {}

RemoteMemory::RemoteMemory(RemoteMemory&& other) noexcept
    : proc_{other.proc_}, addr_{other.Release()} {}

RemoteMemory& RemoteMemory::operator=(RemoteMemory&& other) noexcept {
  if (this != &other) {
    HANDLE proc{other.proc_};
    Reset(proc, other.Release());
  }
  return *this;
}

RemoteMemory::~RemoteMemory() { Reset(); }

LPVOID RemoteMemory::Get() const { return addr_; }

RemoteMemory::operator bool() const { return addr_ != nullptr; }

LPVOID RemoteMemory::Release() {
  LPVOID addr{addr_};
  addr_ = nullptr;
  return addr;
}

void RemoteMemory::Reset(HANDLE proc, LPVOID addr) {
  if (addr_ != nullptr && addr_ != addr) {
    VirtualFreeEx(
        /*hProcess   = */ proc_,
        /*lpAddress  = */ addr_,
        /*dwSize     = */ 0,
        /*dwFreeType = */ MEM_RELEASE);
  }
  proc_ = proc;
  addr_ = addr;
}

}  // namespace afw::win
