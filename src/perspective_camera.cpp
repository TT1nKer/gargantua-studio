#include "gargantua/reference/perspective_camera.h"

#include <cmath>
#include <stdexcept>

namespace gargantua::reference {

CameraRay perspective_camera_ray(
    const ReferenceScene& scene,
    std::size_t pixel_x,
    std::size_t pixel_y) {
    const SceneValidation validation =
        validate_reference_scene(scene);
    if (!validation) {
        throw std::invalid_argument(validation.message);
    }
    if (pixel_x >= scene.width || pixel_y >= scene.height) {
        throw std::out_of_range(
            "camera pixel is outside the reference image");
    }

    const double normalized_x =
        2.0 * (static_cast<double>(pixel_x) + 0.5) /
            static_cast<double>(scene.width) -
        1.0;
    const double normalized_y =
        1.0 -
        2.0 * (static_cast<double>(pixel_y) + 0.5) /
            static_cast<double>(scene.height);
    const double tangent_half_fov =
        std::tan(0.5 * scene.vertical_fov_radians);
    const double aspect =
        static_cast<double>(scene.width) /
        static_cast<double>(scene.height);

    return CameraRay{
        pixel_x,
        pixel_y,
        std::array<double, 3>{{
            -1.0,
            -normalized_y * tangent_half_fov,
            normalized_x * aspect * tangent_half_fov,
        }},
    };
}

} // namespace gargantua::reference
