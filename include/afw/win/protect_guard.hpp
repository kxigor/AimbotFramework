#pragma once

// RAII wrapper over VirtualProtect: relaxes the page protection of a memory
// range on construction and restores the original protection on destruction.

namespace afw::win {}
