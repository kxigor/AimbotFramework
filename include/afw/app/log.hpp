#pragma once

#include <system_error>

namespace afw::app {

void Log(const char* text);

void LogFailure(const char* step, const std::error_code& error);

void LogInstalled(const char* what);

}  // namespace afw::app
