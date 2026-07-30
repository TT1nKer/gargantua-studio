#pragma once

#include "gargantua/reference/reference_scene.h"

#include <array>
#include <cstddef>

namespace gargantua::reference {

struct CameraRay {
    std::size_t pixel_x;
    std::size_t pixel_y;
    std::array<double, 3> local_direction;
};

CameraRay perspective_camera_ray(
    const ReferenceScene& scene,
    std::size_t pixel_x,
    std::size_t pixel_y);

} // namespace gargantua::reference
