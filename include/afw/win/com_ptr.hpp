#pragma once

// Move-only owning pointer for COM interfaces. Direct3D and DXGI hand back
// AddRef'd pointers from their Get* methods, which the caller must release.

namespace afw::win {}
