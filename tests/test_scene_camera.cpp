#include "gargantua/reference/perspective_camera.h"
#include "gargantua/reference/reference_scene.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

using namespace gargantua::reference;

namespace {

int passed = 0;
int failed = 0;

void check(const std::string& name, bool condition) {
    std::cout << (condition ? "  PASS: " : "  FAIL: ") << name << '\n';
    condition ? ++passed : ++failed;
}

void check_near(
    const std::string& name,
    double actual,
    double expected,
    double tolerance) {
    check(
        name,
        std::isfinite(actual) &&
            std::fabs(actual - expected) <= tolerance);
}

} // namespace

int main() {
    constexpr double pi =
        3.141592653589793238462643383279502884;
    const ReferenceScene defaults = reference_scene_defaults();
    check("default reference scene is valid",
          bool(validate_reference_scene(defaults)));

    ReferenceScene invalid_spin = defaults;
    invalid_spin.spin_chi = 1.0;
    check("extremal spin is rejected",
          !validate_reference_scene(invalid_spin));

    ReferenceScene non_finite_mass = defaults;
    non_finite_mass.mass_M =
        std::numeric_limits<double>::quiet_NaN();
    check("non-finite mass is rejected",
          !validate_reference_scene(non_finite_mass));

    ReferenceScene zero_mass = defaults;
    zero_mass.mass_M = 0.0;
    check("zero mass is rejected",
          !validate_reference_scene(zero_mass));

    ReferenceScene negative_extremal_spin = defaults;
    negative_extremal_spin.spin_chi = -1.0;
    check("negative extremal spin is rejected",
          !validate_reference_scene(negative_extremal_spin));

    ReferenceScene polar_observer = defaults;
    polar_observer.inclination_radians = 0.0;
    check("polar observer is rejected",
          !validate_reference_scene(polar_observer));

    ReferenceScene excessive_fov = defaults;
    excessive_fov.vertical_fov_radians =
        121.0 * pi / 180.0;
    check("field of view above 120 degrees is rejected",
          !validate_reference_scene(excessive_fov));

    ReferenceScene zero_width = defaults;
    zero_width.width = 0;
    check("zero width is rejected",
          !validate_reference_scene(zero_width));

    ReferenceScene zero_height = defaults;
    zero_height.height = 0;
    check("zero height is rejected",
          !validate_reference_scene(zero_height));

    ReferenceScene invalid_width = defaults;
    invalid_width.width = 4097;
    check("oversized width is rejected",
          !validate_reference_scene(invalid_width));

    ReferenceScene invalid_height = defaults;
    invalid_height.height = 4097;
    check("oversized height is rejected",
          !validate_reference_scene(invalid_height));

    ReferenceScene near_horizon_observer = defaults;
    near_horizon_observer.spin_chi = 0.0;
    near_horizon_observer.observer_radius_M = 3.0;
    check("observer without 1 M horizon clearance is rejected",
          !validate_reference_scene(near_horizon_observer));

    ReferenceScene invalid_escape = defaults;
    invalid_escape.escape_radius_M =
        invalid_escape.observer_radius_M;
    check("escape radius must exceed observer radius",
          !validate_reference_scene(invalid_escape));

    ReferenceScene negative_affine = defaults;
    negative_affine.max_affine_M = -1.0;
    check("negative affine limit is rejected",
          !validate_reference_scene(negative_affine));

    ReferenceScene zero_initial_step = defaults;
    zero_initial_step.initial_step_M = 0.0;
    check("zero initial step is rejected",
          !validate_reference_scene(zero_initial_step));

    ReferenceScene reversed_steps = defaults;
    reversed_steps.initial_step_M = 0.5;
    reversed_steps.max_step_M = 0.25;
    check("initial step above maximum step is rejected",
          !validate_reference_scene(reversed_steps));

    ReferenceScene short_affine = defaults;
    short_affine.max_affine_M = 20.0;
    check("affine limit unable to reach escape radius is rejected",
          !validate_reference_scene(short_affine));

    ReferenceScene odd = defaults;
    odd.width = 3;
    odd.height = 3;
    const CameraRay exact_center =
        perspective_camera_ray(odd, 1, 1);
    check_near(
        "center ray points radially inward",
        exact_center.local_direction[0],
        -1.0,
        0.0);
    check_near(
        "center ray has no polar offset",
        exact_center.local_direction[1],
        0.0,
        0.0);
    check_near(
        "center ray has no azimuthal offset",
        exact_center.local_direction[2],
        0.0,
        0.0);

    const CameraRay top =
        perspective_camera_ray(odd, 1, 0);
    check(
        "top ray points toward decreasing theta",
        top.local_direction[1] < 0.0);
    const CameraRay right =
        perspective_camera_ray(odd, 2, 1);
    check(
        "right ray points toward increasing phi",
        right.local_direction[2] > 0.0);
    const CameraRay left =
        perspective_camera_ray(odd, 0, 1);
    check_near(
        "horizontal camera offsets are symmetric",
        left.local_direction[2],
        -right.local_direction[2],
        1.0e-16);

    bool out_of_range_rejected = false;
    try {
        (void)perspective_camera_ray(odd, 3, 0);
    } catch (const std::out_of_range&) {
        out_of_range_rejected = true;
    }
    check("out-of-range pixel is rejected", out_of_range_rejected);

    std::cout << "\n=== Results: " << passed << " passed, "
              << failed << " failed ===\n";
    return failed == 0 ? 0 : 1;
}
