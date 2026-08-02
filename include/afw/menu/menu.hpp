#pragma once

#include <windows.h>

namespace afw::menu {

void Draw() noexcept;

[[nodiscard]] bool HandleMessage(HWND window, UINT message, WPARAM wParam,
                                 LPARAM lParam) noexcept;

}  // namespace afw::menu
