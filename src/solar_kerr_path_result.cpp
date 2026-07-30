#include "solar_kerr_path_result.h"

#include "gargantua/reference/reference_numerics.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>

namespace gargantua::reference::detail {
namespace {

using namespace solar::relativity;

const char* termination_reason_name(
    TerminationReason reason) noexcept {
    switch (reason) {
    case TerminationReason::HorizonCrossing:
        return "horizon_crossing";
    case TerminationReason::InteriorCutoff:
        return "interior_cutoff";
    case TerminationReason::Escaped:
        return "escaped";
    case TerminationReason::DiskSurfaceHit:
        return "disk_surface_hit";
    case TerminationReason::MaterialSurfaceHit:
        return "material_surface_hit";
    case TerminationReason::RadialTurningPoint:
        return "radial_turning_point";
    case TerminationReason::PolarTurningPoint:
        return "polar_turning_point";
    case TerminationReason::NearCriticalOrbit:
        return "near_critical_orbit";
    case TerminationReason::MaxAffine:
        return "max_affine";
    case TerminationReason::MaxProperTime:
        return "max_proper_time";
    case TerminationReason::MaxCoordinateTime:
        return "max_coordinate_time";
    case TerminationReason::MaxSteps:
        return "max_steps";
    case TerminationReason::StepUnderflow:
        return "step_underflow";
    case TerminationReason::InvalidMetricPoint:
        return "invalid_metric_point";
    case TerminationReason::NonFiniteState:
        return "non_finite_state";
    case TerminationReason::ConstraintViolation:
        return "constraint_violation";
    case TerminationReason::EventRootFailure:
        return "event_root_failure";
    case TerminationReason::UserEvent:
        return "user_event";
    }
    return "unknown";
}

double relative_drift(double initial, double final) noexcept {
    return std::fabs(final - initial) /
           std::max(1.0, std::fabs(initial));
}

RayClassification classification_for(
    TerminationReason reason) noexcept {
    if (reason == TerminationReason::InteriorCutoff) {
        return RayClassification::CapturedAtBlCutoff;
    }
    if (reason == TerminationReason::Escaped) {
        return RayClassification::Escaped;
    }
    if (reason == TerminationReason::DiskSurfaceHit) {
        return RayClassification::DiskSurfaceHit;
    }
    if (reason == TerminationReason::ConstraintViolation) {
        return RayClassification::ConstraintViolation;
    }
    return RayClassification::Unconverged;
}

double accumulated_maximum(
    double accumulated,
    double candidate) noexcept {
    return std::isfinite(accumulated) &&
                   std::isfinite(candidate)
               ? std::max(accumulated, candidate)
               : std::numeric_limits<double>::quiet_NaN();
}

} // namespace

void SolarKerrPathEvidence::include(
    const KerrSeparatedDiagnostics& diagnostics) {
    accepted_steps += diagnostics.accepted_steps;
    rejected_steps += diagnostics.rejected_steps;
    if (std::isnan(min_radius) ||
        !std::isfinite(diagnostics.min_radius_M)) {
        min_radius =
            std::numeric_limits<double>::quiet_NaN();
    } else {
        min_radius =
            std::min(min_radius, diagnostics.min_radius_M);
    }
    winding += diagnostics.winding;
    max_constraint = accumulated_maximum(
        max_constraint, diagnostics.max_constraint_error);
    max_carter = accumulated_maximum(
        max_carter, diagnostics.max_carter_rel_error);
}

ReferenceRayResult build_solar_kerr_path_result(
    const KerrBoyerLindquistMetric& metric,
    const PhaseSpaceState& final_state,
    const KerrConstants& initial_constants,
    const SolarKerrPathEvidence& evidence,
    const ThinDiskCrossingRecorder& recorder,
    TerminationReason reason) {
    KerrConstants final_constants = initial_constants;
    try {
        final_constants = evaluate_kerr_constants(
            metric, final_state, GeodesicKind::Null);
    } catch (const std::exception&) {
        reason = TerminationReason::NonFiniteState;
    }

    const double energy_drift =
        relative_drift(initial_constants.E, final_constants.E);
    const double lz_drift =
        relative_drift(initial_constants.Lz, final_constants.Lz);
    const double carter_drift = std::max(
        evidence.max_carter,
        relative_drift(initial_constants.Q, final_constants.Q));
    RayClassification classification =
        classification_for(reason);
    if (!is_failed_classification(classification) &&
        (!std::isfinite(evidence.max_constraint) ||
         evidence.max_constraint >=
             reference_hamiltonian_error_gate ||
         !std::isfinite(energy_drift) ||
         energy_drift >=
             reference_stationary_invariant_error_gate ||
         !std::isfinite(lz_drift) ||
         lz_drift >=
             reference_stationary_invariant_error_gate ||
         !std::isfinite(carter_drift) ||
         carter_drift >=
             reference_carter_relative_error_gate)) {
        classification = RayClassification::ConstraintViolation;
        reason = TerminationReason::ConstraintViolation;
    }

    const auto& crossings = recorder.crossings();
    const bool has_disk = !crossings.empty();
    const double unavailable =
        std::numeric_limits<double>::quiet_NaN();
    const ThinDiskCrossing* last =
        has_disk ? &crossings.back() : nullptr;
    return ReferenceRayResult{
        classification,
        termination_reason_name(reason),
        final_state.affine,
        final_state.x.v[1],
        evidence.min_radius,
        evidence.winding,
        evidence.max_constraint,
        energy_drift,
        lz_drift,
        carter_drift,
        evidence.accepted_steps,
        evidence.rejected_steps,
        last ? last->disk_radius : unavailable,
        last ? last->redshift_g : unavailable,
        last ? last->observed_temperature : unavailable,
        recorder.observed().specific_intensity,
        recorder.observed().bolometric_intensity,
        crossings.size(),
    };
}

} // namespace gargantua::reference::detail
