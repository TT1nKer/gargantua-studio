#include "gargantua/reference/reference_frame.h"

#include "reference_frame_summary.h"
#include "reference_ray_evidence.h"

#include <algorithm>
#include <cmath>

namespace gargantua::reference {

const char* frame_status_name(FrameStatus status) noexcept {
    switch (status) {
    case FrameStatus::Complete:
        return "complete";
    case FrameStatus::DiagnosticFailed:
        return "diagnostic_failed";
    }
    return "unknown";
}

} // namespace gargantua::reference

namespace gargantua::reference::detail {
namespace {

void retain_maximum(double value, double& maximum) {
    if (std::isfinite(value)) {
        maximum = std::max(maximum, value);
    }
}

} // namespace

bool summarize_reference_rays(
    const std::vector<ReferenceRayResult>& rays,
    ReferenceFrameSummary& summary) noexcept {
    summary = {};
    for (const ReferenceRayResult& ray : rays) {
        if (!valid_reference_ray_evidence(ray)) {
            return false;
        }
        switch (ray.classification) {
        case RayClassification::CapturedAtBlCutoff:
            ++summary.captured;
            break;
        case RayClassification::Escaped:
            ++summary.escaped;
            break;
        case RayClassification::DiskSurfaceHit:
            ++summary.disk_surface_hits;
            break;
        case RayClassification::Unconverged:
            ++summary.unconverged;
            break;
        case RayClassification::ConstraintViolation:
            ++summary.constraint_violations;
            break;
        case RayClassification::InitializationError:
            ++summary.initialization_errors;
            break;
        case RayClassification::TransferFailure:
            ++summary.transfer_failures;
            break;
        default:
            return false;
        }
        if (is_failed_classification(ray.classification)) {
            ++summary.failed;
        }

        retain_maximum(
            ray.max_constraint_error,
            summary.max_constraint_error);
        retain_maximum(
            ray.max_energy_rel_error,
            summary.max_energy_rel_error);
        retain_maximum(
            ray.max_lz_rel_error,
            summary.max_lz_rel_error);
        retain_maximum(
            ray.max_carter_rel_error,
            summary.max_carter_rel_error);
        retain_maximum(
            ray.redshift_g,
            summary.max_redshift_g);
        retain_maximum(
            ray.observed_specific_intensity,
            summary.max_observed_specific_intensity);
        retain_maximum(
            ray.observed_bolometric_intensity,
            summary.max_observed_bolometric_intensity);
        summary.disk_crossings += ray.disk_crossings;
        summary.max_accepted_steps =
            std::max(summary.max_accepted_steps, ray.accepted_steps);
        summary.max_rejected_steps =
            std::max(summary.max_rejected_steps, ray.rejected_steps);
    }
    return true;
}

bool reference_frame_summaries_equal(
    const ReferenceFrameSummary& left,
    const ReferenceFrameSummary& right) noexcept {
    return left.captured == right.captured &&
           left.escaped == right.escaped &&
           left.disk_surface_hits == right.disk_surface_hits &&
           left.unconverged == right.unconverged &&
           left.constraint_violations ==
               right.constraint_violations &&
           left.initialization_errors ==
               right.initialization_errors &&
           left.transfer_failures == right.transfer_failures &&
           left.failed == right.failed &&
           left.disk_crossings == right.disk_crossings &&
           left.max_constraint_error ==
               right.max_constraint_error &&
           left.max_energy_rel_error ==
               right.max_energy_rel_error &&
           left.max_lz_rel_error == right.max_lz_rel_error &&
           left.max_carter_rel_error ==
               right.max_carter_rel_error &&
           left.max_redshift_g == right.max_redshift_g &&
           left.max_observed_specific_intensity ==
               right.max_observed_specific_intensity &&
           left.max_observed_bolometric_intensity ==
               right.max_observed_bolometric_intensity &&
           left.max_accepted_steps ==
               right.max_accepted_steps &&
           left.max_rejected_steps ==
               right.max_rejected_steps;
}

} // namespace gargantua::reference::detail
