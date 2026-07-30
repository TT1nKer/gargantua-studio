#include "reference_ray_evidence.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

using namespace gargantua::reference;
using namespace gargantua::reference::detail;

namespace {

int passed = 0;
int failed = 0;

void check(const std::string& name, bool condition) {
    std::cout << (condition ? "  PASS: " : "  FAIL: ")
              << name << '\n';
    condition ? ++passed : ++failed;
}

double unavailable() {
    return std::numeric_limits<double>::quiet_NaN();
}

ReferenceRayResult no_hit(
    RayClassification classification,
    const char* reason) {
    return ReferenceRayResult{
        classification,
        reason,
        -20.0,
        classification == RayClassification::Escaped
            ? 60.0
            : 2.001,
        2.001,
        0.25,
        1.0e-12,
        2.0e-14,
        3.0e-14,
        4.0e-12,
        80,
        1,
        unavailable(),
        unavailable(),
        unavailable(),
        0.0,
        0.0,
        0,
    };
}

ReferenceRayResult disk_hit() {
    ReferenceRayResult ray = no_hit(
        RayClassification::DiskSurfaceHit,
        "disk_surface_hit");
    ray.final_radius_M = 8.0;
    ray.min_radius_M = 4.0;
    ray.disk_radius_M = 8.0;
    ray.redshift_g = 0.8;
    ray.observed_temperature = 4.0;
    ray.observed_specific_intensity = 2.0;
    ray.observed_bolometric_intensity = 3.0;
    ray.disk_crossings = 1;
    return ray;
}

ReferenceRayResult semi_transparent_escape() {
    ReferenceRayResult ray =
        no_hit(RayClassification::Escaped, "escaped");
    ray.disk_radius_M = 7.0;
    ray.redshift_g = 1.2;
    ray.observed_temperature = 5.0;
    ray.observed_specific_intensity = 1.5;
    ray.observed_bolometric_intensity = 2.5;
    ray.disk_crossings = 2;
    return ray;
}

} // namespace

int main() {
    const ReferenceRayResult escaped =
        no_hit(RayClassification::Escaped, "escaped");
    const ReferenceRayResult captured =
        no_hit(
            RayClassification::CapturedAtBlCutoff,
            "interior_cutoff");
    const ReferenceRayResult disk = disk_hit();
    const ReferenceRayResult translucent =
        semi_transparent_escape();
    const ReferenceRayResult transfer_failure =
        unavailable_reference_ray(
            RayClassification::TransferFailure,
            "surface_transfer_failed");

    check("escaped no-hit evidence is valid",
          valid_reference_ray_evidence(escaped));
    check("captured no-hit evidence is valid",
          valid_reference_ray_evidence(captured));
    check("opaque disk evidence is valid",
          valid_reference_ray_evidence(disk));
    check("semi-transparent escape evidence is valid",
          valid_reference_ray_evidence(translucent));
    check("explicit unavailable transfer failure is valid",
          valid_reference_ray_evidence(transfer_failure));

    ReferenceRayResult invalid = disk;
    invalid.redshift_g = unavailable();
    check("disk evidence requires finite redshift",
          !valid_reference_ray_evidence(invalid));

    invalid = disk;
    invalid.observed_bolometric_intensity = -1.0;
    check("disk evidence rejects negative intensity",
          !valid_reference_ray_evidence(invalid));

    invalid = disk;
    invalid.disk_crossings = 0;
    check("disk evidence requires a crossing",
          !valid_reference_ray_evidence(invalid));

    invalid = escaped;
    invalid.observed_bolometric_intensity = 1.0;
    check("no-hit evidence cannot carry disk intensity",
          !valid_reference_ray_evidence(invalid));

    invalid = escaped;
    invalid.disk_radius_M = 10.0;
    check("no-hit evidence keeps disk radius unavailable",
          !valid_reference_ray_evidence(invalid));

    invalid = escaped;
    invalid.final_affine_M = 20.0;
    check("successful evidence must advance observer-to-past",
          !valid_reference_ray_evidence(invalid));

    invalid = escaped;
    invalid.max_constraint_error = 1.0e-10;
    check("successful evidence enforces Hamiltonian gate",
          !valid_reference_ray_evidence(invalid));

    invalid = captured;
    invalid.termination_reason = "escaped";
    check("classification and termination must agree",
          !valid_reference_ray_evidence(invalid));

    invalid = escaped;
    invalid.classification =
        static_cast<RayClassification>(99);
    check("unknown classification is rejected",
          !valid_reference_ray_evidence(invalid));

    invalid = transfer_failure;
    invalid.observed_specific_intensity = -1.0;
    check("failed evidence rejects negative finite intensity",
          !valid_reference_ray_evidence(invalid));

    const ReferenceRayResult initialization_failure =
        unavailable_reference_ray(
            RayClassification::InitializationError,
            "initialization_error");
    check("unavailable constructor preserves classification",
          initialization_failure.classification ==
              RayClassification::InitializationError);
    check("unavailable constructor preserves reason",
          initialization_failure.termination_reason ==
              "initialization_error");
    check("unavailable constructor leaves affine unavailable",
          std::isnan(initialization_failure.final_affine_M));
    check("unavailable constructor records no crossing",
          initialization_failure.disk_crossings == 0);

    std::cout << "\n=== Results: " << passed << " passed, "
              << failed << " failed ===\n";
    return failed == 0 ? 0 : 1;
}
