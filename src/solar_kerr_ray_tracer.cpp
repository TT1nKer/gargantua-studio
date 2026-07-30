#include "gargantua/reference/reference_ray_tracer.h"

#include "solar/relativity/geodesic_integrator.h"
#include "solar/relativity/kerr_bl_metric.h"
#include "solar/relativity/kerr_constants.h"
#include "solar/relativity/local_initialization.h"
#include "solar/relativity/observer.h"
#include "solar/version.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gargantua::reference {
namespace {

using namespace solar::relativity;

constexpr double hamiltonian_gate = 1.0e-10;
constexpr double stationary_invariant_gate = 1.0e-12;
constexpr double carter_gate = 1.0e-9;
constexpr double capture_margin_fraction = 1.0e-4;

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

ReferenceRayResult unavailable_result(
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
        0,
        0,
    };
}

ObserverFrame make_render_observer(
    const KerrBoyerLindquistMetric& metric,
    const ReferenceScene& scene) {
    const Contravariant4 position{
        Vec4{{0.0, scene.observer_radius_M,
              scene.inclination_radians, 0.0}}};
    const ObserverResult observer =
        make_zamo_observer(metric, position);
    if (!observer) {
        throw std::invalid_argument(
            "cannot construct render ZAMO: " + observer.message);
    }
    return *observer.frame;
}

class SolarKerrRayTracer final : public ReferenceRayTracer {
public:
    explicit SolarKerrRayTracer(const ReferenceScene& scene)
        : metric_(scene.mass_M, scene.spin_chi),
          observer_(make_render_observer(metric_, scene)),
          integrator_(metric_),
          config_(GeodesicIntegrationConfig::cpu_reference(
              GeodesicKind::Null,
              scene.mass_M,
              scene.initial_step_M,
              scene.max_step_M,
              scene.max_affine_M)),
          info_{
              std::string(solar::version),
              std::string(solar::physics_contract),
              metric_.outer_horizon_radius() +
                  capture_margin_fraction * scene.mass_M} {
        config_.monitor_energy = true;
        config_.monitor_lz = true;
        config_.carter_evaluator =
            [this](const PhaseSpaceState& state) {
                return evaluate_kerr_constants(
                    metric_, state, GeodesicKind::Null).Q;
            };

        const double root_tolerance = 1.0e-10 * scene.mass_M;
        const double capture_radius = info_.capture_radius_M;
        events_.push_back(GeodesicEvent{
            "bl-capture-cutoff",
            [capture_radius](const PhaseSpaceState& state) {
                return state.x.v[1] - capture_radius;
            },
            EventDirection::Decreasing,
            TerminationReason::InteriorCutoff,
            root_tolerance,
        });
        const double escape_radius = scene.escape_radius_M;
        events_.push_back(GeodesicEvent{
            "escape-radius",
            [escape_radius](const PhaseSpaceState& state) {
                return state.x.v[1] - escape_radius;
            },
            EventDirection::Increasing,
            TerminationReason::Escaped,
            root_tolerance,
        });
    }

    const ReferenceTracerInfo& info() const noexcept override {
        return info_;
    }

    ReferenceRayResult trace(
        const CameraRay& ray) const override {
        const InitialStateResult initial = initialize_local_photon(
            metric_,
            observer_,
            Vec3{{ray.local_direction[0],
                  ray.local_direction[1],
                  ray.local_direction[2]}});
        if (!initial) {
            return unavailable_result(
                RayClassification::InitializationError,
                "initialization_error");
        }

        try {
            return classify(integrator_.integrate(
                *initial.state, config_, events_));
        } catch (const std::exception&) {
            return unavailable_result(
                RayClassification::Unconverged,
                "integration_exception");
        }
    }

private:
    static ReferenceRayResult classify(
        const GeodesicIntegrationResult& integrated) {
        const IntegrationDiagnostics& diagnostics =
            integrated.diagnostics;
        RayClassification classification =
            RayClassification::Unconverged;
        if (diagnostics.reason == TerminationReason::InteriorCutoff) {
            classification =
                RayClassification::CapturedAtBlCutoff;
        } else if (diagnostics.reason == TerminationReason::Escaped) {
            classification = RayClassification::Escaped;
        } else if (
            diagnostics.reason ==
            TerminationReason::ConstraintViolation) {
            classification =
                RayClassification::ConstraintViolation;
        }

        const double final_radius =
            integrated.final_state.x.v[1];
        const bool invariant_gate_passed =
            std::isfinite(final_radius) &&
            std::isfinite(diagnostics.max_constraint_error) &&
            diagnostics.max_constraint_error < hamiltonian_gate &&
            std::isfinite(diagnostics.max_energy_rel_error) &&
            diagnostics.max_energy_rel_error <
                stationary_invariant_gate &&
            std::isfinite(diagnostics.max_lz_rel_error) &&
            diagnostics.max_lz_rel_error <
                stationary_invariant_gate &&
            std::isfinite(diagnostics.max_carter_rel_error) &&
            diagnostics.max_carter_rel_error < carter_gate;
        if (!invariant_gate_passed &&
            !is_failed_classification(classification)) {
            classification =
                RayClassification::ConstraintViolation;
        }

        return ReferenceRayResult{
            classification,
            termination_reason_name(diagnostics.reason),
            final_radius,
            diagnostics.max_constraint_error,
            diagnostics.max_energy_rel_error,
            diagnostics.max_lz_rel_error,
            diagnostics.max_carter_rel_error,
            diagnostics.accepted_steps,
            diagnostics.rejected_steps,
        };
    }

    KerrBoyerLindquistMetric metric_;
    ObserverFrame observer_;
    GeodesicIntegrator integrator_;
    GeodesicIntegrationConfig config_;
    ReferenceTracerInfo info_;
    std::vector<GeodesicEvent> events_;
};

} // namespace

ReferenceTracerBuild make_solar_kerr_ray_tracer(
    const ReferenceScene& scene) {
    const SceneValidation validation =
        validate_reference_scene(scene);
    if (!validation) {
        return ReferenceTracerBuild{nullptr, validation.message};
    }
    try {
        return ReferenceTracerBuild{
            std::make_unique<SolarKerrRayTracer>(scene),
            {},
        };
    } catch (const std::exception& error) {
        return ReferenceTracerBuild{nullptr, error.what()};
    }
}

} // namespace gargantua::reference
