#include "gargantua/reference/reference_scene.h"

#include <array>
#include <cmath>

namespace gargantua::reference {
namespace {

constexpr double pi = 3.141592653589793238462643383279502884;
constexpr std::size_t max_dimension = 4096;
constexpr std::size_t max_pixels = 4096 * 4096;

SceneValidation invalid(const char* message) {
    return SceneValidation{false, message};
}

} // namespace

ReferenceScene reference_scene_defaults() noexcept {
    return ReferenceScene{
        1.0,
        0.5,
        30.0,
        85.0 * pi / 180.0,
        40.0 * pi / 180.0,
        64,
        36,
        60.0,
        200.0,
        0.02,
        0.25,
    };
}

SceneValidation validate_reference_scene(
    const ReferenceScene& scene) {
    const std::array<double, 9> numeric_values{{
        scene.mass_M,
        scene.spin_chi,
        scene.observer_radius_M,
        scene.inclination_radians,
        scene.vertical_fov_radians,
        scene.escape_radius_M,
        scene.max_affine_M,
        scene.initial_step_M,
        scene.max_step_M,
    }};
    for (const double value : numeric_values) {
        if (!std::isfinite(value)) {
            return invalid("scene numeric values must be finite");
        }
    }
    if (scene.mass_M <= 0.0) {
        return invalid("mass_M must be positive");
    }
    if (std::fabs(scene.spin_chi) >= 1.0) {
        return invalid("spin_chi must satisfy abs(spin_chi) < 1");
    }
    if (scene.inclination_radians < 1.0e-3 ||
        scene.inclination_radians > pi - 1.0e-3) {
        return invalid("inclination is outside the supported polar margin");
    }
    if (scene.vertical_fov_radians < pi / 180.0 ||
        scene.vertical_fov_radians > 2.0 * pi / 3.0) {
        return invalid("vertical field of view must be in [1,120] degrees");
    }
    if (scene.width == 0 || scene.height == 0 ||
        scene.width > max_dimension ||
        scene.height > max_dimension ||
        scene.width > max_pixels / scene.height) {
        return invalid("image dimensions exceed the reference-render limit");
    }

    const double horizon_radius =
        scene.mass_M *
        (1.0 + std::sqrt(
            1.0 - scene.spin_chi * scene.spin_chi));
    if (scene.observer_radius_M <=
        horizon_radius + scene.mass_M) {
        return invalid("observer must remain at least 1 M outside the horizon");
    }
    if (scene.escape_radius_M <= scene.observer_radius_M) {
        return invalid("escape radius must exceed observer radius");
    }
    if (scene.max_affine_M <= 0.0 ||
        scene.initial_step_M <= 0.0 ||
        scene.max_step_M <= 0.0) {
        return invalid("affine and step limits must be positive");
    }
    if (scene.initial_step_M > scene.max_step_M) {
        return invalid("initial step must not exceed maximum step");
    }
    if (scene.max_affine_M <
        scene.escape_radius_M - scene.observer_radius_M) {
        return invalid("maximum affine length cannot reach the escape radius");
    }
    return SceneValidation{true, {}};
}

} // namespace gargantua::reference
