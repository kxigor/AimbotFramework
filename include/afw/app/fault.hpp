#pragma once

namespace afw::app {

class FaultLogger {
 public:
  FaultLogger() noexcept;

  FaultLogger(const FaultLogger&) = delete;
  FaultLogger& operator=(const FaultLogger&) = delete;

  FaultLogger(FaultLogger&&) = delete;
  FaultLogger& operator=(FaultLogger&&) = delete;

  ~FaultLogger();
};

}  // namespace afw::app
