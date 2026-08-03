#pragma once

namespace afw::app {

class DetourGuard {
 public:
  DetourGuard() noexcept;

  DetourGuard(const DetourGuard&) = delete;
  DetourGuard& operator=(const DetourGuard&) = delete;

  DetourGuard(DetourGuard&&) = delete;
  DetourGuard& operator=(DetourGuard&&) = delete;

  ~DetourGuard();
};

void WaitForDetours() noexcept;

}  // namespace afw::app
