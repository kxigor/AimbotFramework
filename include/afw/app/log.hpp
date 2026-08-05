#pragma once

#include <system_error>

namespace afw::app {

void Log(const char* text);

// printf into a fixed buffer, then out through Log. Every caller ran the same
// three lines -- a char array, a snprintf into it, a Log of its data -- and the
// buffer must stay on the stack: this runs inside a detour, on the engine's own
// thread, where an allocation is not ours to make.
[[gnu::format(printf, 1, 2)]] void Logf(const char* format, ...);

void LogFailure(const char* step, const std::error_code& error);

void LogInstalled(const char* what);

}  // namespace afw::app
