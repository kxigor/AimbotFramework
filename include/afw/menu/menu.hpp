#pragma once

#include <windows.h>

namespace afw::menu {

void Draw() noexcept;

// True when the overlay consumed the message and the game must not see it.
[[nodiscard]] bool HandleMessage(HWND window, UINT message, WPARAM wParam,
                                 LPARAM lParam) noexcept;

}  // namespace afw::menu
