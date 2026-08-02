#include <afw/math/vec3.hpp>

// Vec3 is defined entirely in the header: it is a twelve-byte type on the path
// every entity takes every frame, so nothing here may be out of line. This
// translation unit exists to give the library a compiled artefact, and it is
// where Matrix4 and the aim-angle solving will land.

namespace afw::math {}  // namespace afw::math
