#include "reference_ray_evidence.h"

#include "gargantua/reference/reference_numerics.h"

#include <cmath>
#include <limits>
#include <utility>

namespace gargantua::reference::detail {
namespace {

bool recognized_classification(
    RayClassification classification) noexcept {
    switch (classification) {
    case RayClassification::CapturedAtBlCutoff:
    case RayClassification::Escaped:
    case RayClassification::DiskSurfaceHit:
    case RayClassification::Unconverged:
    case RayClassification::ConstraintViolation:
    case RayClassification::InitializationError:
    case RayClassification::TransferFailure:
        return true;
    }
    return false;
}

bool is_successful(
    RayClassification classification) noexcept {
    return classification ==
               RayClassification::CapturedAtBlCutoff ||
           classification == RayClassification::Escaped ||
           classification ==
               RayClassification::DiskSurfaceHit;
}

bool termination_matches(
    const ReferenceRayResult& ray) noexcept {
    if (ray.classification ==
        RayClassification::CapturedAtBlCutoff) {
        return ray.termination_reason == "interior_cutoff";
    }
    if (ray.classification == RayClassification::Escaped) {
        return ray.termination_reason == "escaped";
    }
    if (ray.classification ==
        RayClassification::DiskSurfaceHit) {
        return ray.termination_reason == "disk_surface_hit";
    }
    return !ray.termination_reason.empty();
}

bool finite_nonnegative(double value) noexcept {
    return std::isfinite(value) && value >= 0.0;
}

bool finite_positive(double value) noexcept {
    return std::isfinite(value) && value > 0.0;
}

bool unavailable_disk_evidence(
    const ReferenceRayResult& ray) noexcept {
    return ray.disk_crossings == 0 &&
           std::isnan(ray.disk_radius_M) &&
           std::isnan(ray.redshift_g) &&
           std::isnan(ray.observed_temperature) &&
           ray.observed_specific_intensity == 0.0 &&
           ray.observed_bolometric_intensity == 0.0;
}

bool valid_disk_evidence(
    const ReferenceRayResult& ray) noexcept {
    return ray.disk_crossings > 0 &&
           finite_positive(ray.disk_radius_M) &&
           finite_positive(ray.redshift_g) &&
           finite_nonnegative(ray.observed_temperature) &&
           finite_nonnegative(
               ray.observed_specific_intensity) &&
           finite_nonnegative(
               ray.observed_bolometric_intensity);
}

bool valid_success_diagnostics(
    const ReferenceRayResult& ray) noexcept {
    return std::isfinite(ray.final_affine_M) &&
           ray.final_affine_M < 0.0 &&
           finite_positive(ray.final_radius_M) &&
           finite_positive(ray.min_radius_M) &&
           std::isfinite(ray.winding) &&
           finite_nonnegative(ray.max_constraint_error) &&
           ray.max_constraint_error <
               reference_hamiltonian_error_gate &&
           finite_nonnegative(ray.max_energy_rel_error) &&
           ray.max_energy_rel_error <
               reference_stationary_invariant_error_gate &&
           finite_nonnegative(ray.max_lz_rel_error) &&
           ray.max_lz_rel_error <
               reference_stationary_invariant_error_gate &&
           finite_nonnegative(ray.max_carter_rel_error) &&
           ray.max_carter_rel_error <
               reference_carter_relative_error_gate &&
           ray.accepted_steps > 0;
}

bool failed_scalars_are_safe(
    const ReferenceRayResult& ray) noexcept {
    const auto nonnegative_if_finite = [](double value) {
        return !std::isfinite(value) || value >= 0.0;
    };
    return (!std::isfinite(ray.final_affine_M) ||
            ray.final_affine_M <= 0.0) &&
           nonnegative_if_finite(ray.final_radius_M) &&
           nonnegative_if_finite(ray.min_radius_M) &&
           nonnegative_if_finite(ray.max_constraint_error) &&
           nonnegative_if_finite(ray.max_energy_rel_error) &&
           nonnegative_if_finite(ray.max_lz_rel_error) &&
           nonnegative_if_finite(ray.max_carter_rel_error) &&
           nonnegative_if_finite(
               ray.observed_temperature) &&
           nonnegative_if_finite(
               ray.observed_specific_intensity) &&
           nonnegative_if_finite(
               ray.observed_bolometric_intensity);
}

} // namespace

ReferenceRayResult unavailable_reference_ray(
    RayClassification classification,
    std::string reason) {
    const double unavailable =
        std::numeric_limits<double>::quiet_NaN();
    return ReferenceRayResult{
        classification,
        std::move(reason),
        unavailable,
        unavailable,
        unavailable,
        unavailable,
        unavailable,
        unavailable,
        unavailable,
        unavailable,
        0,
        0,
        unavailable,
        unavailable,
        unavailable,
        unavailable,
        unavailable,
        0,
    };
}

bool valid_reference_ray_evidence(
    const ReferenceRayResult& ray) noexcept {
    if (!recognized_classification(ray.classification) ||
        !termination_matches(ray)) {
        return false;
    }

    if (is_successful(ray.classification)) {
        if (!valid_success_diagnostics(ray)) {
            return false;
        }
        const bool has_disk_evidence =
            ray.disk_crossings > 0;
        if (has_disk_evidence) {
            if (!valid_disk_evidence(ray)) {
                return false;
            }
        } else if (!unavailable_disk_evidence(ray)) {
            return false;
        }
        return ray.classification !=
                   RayClassification::DiskSurfaceHit ||
               has_disk_evidence;
    }

    if (!failed_scalars_are_safe(ray)) {
        return false;
    }
    return ray.disk_crossings == 0 ||
           valid_disk_evidence(ray);
}

} // namespace gargantua::reference::detail
