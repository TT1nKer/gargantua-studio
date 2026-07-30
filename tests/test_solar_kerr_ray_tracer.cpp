#include "gargantua/reference/perspective_camera.h"
#include "gargantua/reference/reference_ray_tracer.h"
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

} // namespace

int main() {
    constexpr double half_pi =
        1.570796326794896619231321691639751442;

    ReferenceScene scene = reference_scene_defaults();
    scene.spin_chi = 0.0;
    scene.inclination_radians = half_pi;
    scene.width = 9;
    scene.height = 9;

    ReferenceTracerBuild built =
        make_solar_kerr_ray_tracer(scene);
    check("Solar Kerr tracer initializes", bool(built));
    if (!built) {
        std::cerr << built.message << '\n';
        return 1;
    }
    check(
        "capture cutoff remains outside Schwarzschild horizon",
        built.tracer->info().capture_radius_M > 2.0009 &&
            built.tracer->info().capture_radius_M < 2.0011);
    check(
        "Solar package version retained",
        built.tracer->info().solar_version == "0.2.0-alpha.1");
    check(
        "Solar physics contract retained",
        built.tracer->info().physics_contract ==
            "relativity-v3-phase2");

    const ReferenceRayResult center = built.tracer->trace(
        perspective_camera_ray(scene, 4, 4));
    std::cout << "  center classification="
              << ray_classification_name(center.classification)
              << " reason=" << center.termination_reason
              << " final_r=" << center.final_radius_M
              << " constraint=" << center.max_constraint_error
              << " accepted=" << center.accepted_steps
              << " rejected=" << center.rejected_steps << '\n';
    check(
        "radial center ray reaches BL capture cutoff",
        center.classification ==
            RayClassification::CapturedAtBlCutoff);
    check(
        "captured ray retains interior-cutoff reason",
        center.termination_reason == "interior_cutoff");
    check(
        "captured ray advances observer-to-past",
        center.final_affine_M < 0.0);
    check(
        "captured ray satisfies Hamiltonian gate",
        center.max_constraint_error < 1.0e-10);
    check(
        "captured ray satisfies energy gate",
        center.max_energy_rel_error < 1.0e-12);
    check(
        "captured ray satisfies axial angular momentum gate",
        center.max_lz_rel_error < 1.0e-12);
    check(
        "captured ray satisfies Carter gate",
        center.max_carter_rel_error < 1.0e-9);

    ReferenceScene near_cutoff_scene = scene;
    near_cutoff_scene.width = 64;
    near_cutoff_scene.height = 64;
    near_cutoff_scene.initial_step_M = 0.01;
    near_cutoff_scene.max_step_M = 0.1;
    ReferenceTracerBuild near_cutoff_built =
        make_solar_kerr_ray_tracer(near_cutoff_scene);
    check("near-cutoff regression tracer initializes",
          bool(near_cutoff_built));
    if (near_cutoff_built) {
        const ReferenceRayResult near_cutoff =
            near_cutoff_built.tracer->trace(
                perspective_camera_ray(near_cutoff_scene, 25, 32));
        check(
            "near-cutoff regression ray is captured",
            near_cutoff.classification ==
                RayClassification::CapturedAtBlCutoff);
        check(
            "near-cutoff regression ray satisfies Hamiltonian gate",
            near_cutoff.max_constraint_error < 1.0e-10);
    }

    const ReferenceRayResult corner = built.tracer->trace(
        perspective_camera_ray(scene, 0, 0));
    check(
        "wide corner ray escapes",
        corner.classification == RayClassification::Escaped);
    check(
        "escaped ray retains escape reason",
        corner.termination_reason == "escaped");
    check(
        "escaped ray advances observer-to-past",
        corner.final_affine_M < 0.0);
    check(
        "escaped ray satisfies Hamiltonian gate",
        corner.max_constraint_error < 1.0e-10);
    check(
        "escaped ray satisfies energy gate",
        corner.max_energy_rel_error < 1.0e-12);
    check(
        "escaped ray satisfies axial angular momentum gate",
        corner.max_lz_rel_error < 1.0e-12);
    check(
        "escaped ray satisfies Carter gate",
        corner.max_carter_rel_error < 1.0e-9);

    CameraRay non_finite =
        perspective_camera_ray(scene, 4, 4);
    non_finite.local_direction[1] =
        std::numeric_limits<double>::quiet_NaN();
    const ReferenceRayResult rejected =
        built.tracer->trace(non_finite);
    check(
        "non-finite camera ray is initialization error",
        rejected.classification ==
            RayClassification::InitializationError);
    check(
        "non-finite camera ray is never escape",
        rejected.classification != RayClassification::Escaped);
    check(
        "unknown classification name remains explicit",
        std::string(ray_classification_name(
            static_cast<RayClassification>(99))) == "unknown");
    check(
        "unknown classification counts as failure",
        is_failed_classification(
            static_cast<RayClassification>(99)));

    std::cout << "\n=== Results: " << passed << " passed, "
              << failed << " failed ===\n";
    return failed == 0 ? 0 : 1;
}
