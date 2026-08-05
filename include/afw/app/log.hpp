#pragma once

#include <system_error>

namespace afw::app {

void Log(const char* text);

[[gnu::format(printf, 1, 2)]] void Logf(const char* format, ...);

void LogFailure(const char* step, const std::error_code& error);

void LogInstalled(const char* what);

}  // namespace afw::app
