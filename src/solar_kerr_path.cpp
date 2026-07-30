#include "solar_kerr_path.h"

#include "gargantua/reference/reference_numerics.h"
#include "reference_ray_evidence.h"
#include "solar_kerr_path_result.h"
#include "solar_kerr_path_setup.h"
#include "solar_thin_disk.h"
#include "solar/relativity/kerr_bl_metric.h"
#include "solar/relativity/kerr_constants.h"
#include "solar/relativity/kerr_separated.h"
#include "solar/relativity/local_initialization.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace gargantua::reference::detail {
namespace {

using namespace solar::relativity;

constexpr std::size_t base_path_segment_budget = 64;

SolarKerrPathTrace transfer_failure(
    std::string reason,
    std::size_t ignored_crossings) {
    return SolarKerrPathTrace{
        unavailable_reference_ray(
            RayClassification::TransferFailure,
            std::move(reason)),
        ignored_crossings,
    };
}

} // namespace

void validate_solar_kerr_path_scene(
    const KerrBoyerLindquistMetric& metric,
    const ReferenceScene& scene) {
    const double horizon_radius =
        metric.outer_horizon_radius();
    if (scene.observer_radius_M <=
        horizon_radius + scene.mass_M) {
        throw std::invalid_argument(
            "observer must remain at least 1 M outside the horizon");
    }
    if (scene.disk.outer_radius_M <= horizon_radius) {
        throw std::invalid_argument(
            "disk outer radius must remain outside the horizon");
    }
    validate_solar_thin_disk_scene(scene);
    if (is_solar_equatorial_plane(
            scene.inclination_radians) &&
        scene.observer_radius_M <= scene.disk.outer_radius_M) {
        throw std::invalid_argument(
            "observer cannot lie on the emitting disk surface");
    }
}

SolarKerrPathTrace trace_solar_kerr_path(
    const KerrBoyerLindquistMetric& metric,
    const ObserverFrame& observer,
    const ReferenceScene& scene,
    const CameraRay& ray) {
    const InitialStateResult initialized =
        initialize_local_photon(
            metric,
            observer,
            Vec3{{ray.local_direction[0],
                  ray.local_direction[1],
                  ray.local_direction[2]}});
    if (!initialized) {
        return SolarKerrPathTrace{
            unavailable_reference_ray(
                RayClassification::InitializationError,
                "initialization_error"),
            0,
        };
    }

    const KerrConstants initial_constants =
        evaluate_kerr_constants(
            metric, *initialized.state, GeodesicKind::Null);
    ThinDiskCrossingRecorder recorder =
        make_solar_thin_disk_recorder(scene);
    KerrSeparatedIntegrator integrator(metric);
    PhaseSpaceState current = *initialized.state;
    EventDirection disk_direction = EventDirection::Any;
    std::size_t ignored_crossings = 0;

    if (is_solar_equatorial_plane(current.x.v[2])) {
        const auto departure_direction =
            next_solar_disk_direction(
                current, -scene.initial_step_M);
        if (!departure_direction) {
            return transfer_failure(
                "equatorial_tangent_ray", ignored_crossings);
        }
        disk_direction = *departure_direction;
        ++ignored_crossings;
    }

    const double capture_radius =
        metric.outer_horizon_radius() +
        reference_capture_margin_fraction * scene.mass_M;
    const std::size_t maximum_path_segments =
        base_path_segment_budget +
        2 * scene.disk.max_crossings;
    SolarKerrPathEvidence evidence;
    for (std::size_t segment = 0;
         segment < maximum_path_segments;
         ++segment) {
        const double remaining =
            scene.max_affine_M - std::fabs(current.affine);
        if (!std::isfinite(remaining) || remaining <= 0.0) {
            return SolarKerrPathTrace{
                build_solar_kerr_path_result(
                    metric,
                    current,
                    initial_constants,
                    evidence,
                    recorder,
                    TerminationReason::MaxAffine),
                ignored_crossings,
            };
        }

        const KerrSeparatedConfig config =
            make_solar_kerr_separated_config(
                scene, remaining);
        const KerrSeparatedIntegrationResult integrated =
            integrator.integrate(
                current,
                config,
                make_solar_kerr_path_events(
                    scene, capture_radius, disk_direction));
        evidence.include(integrated.diagnostics);
        current = integrated.final_state;

        if (integrated.diagnostics.reason !=
            TerminationReason::DiskSurfaceHit) {
            return SolarKerrPathTrace{
                build_solar_kerr_path_result(
                    metric,
                    current,
                    initial_constants,
                    evidence,
                    recorder,
                    integrated.diagnostics.reason),
                ignored_crossings,
            };
        }
        if (!integrated.event) {
            return transfer_failure(
                "disk_event_without_exact_state",
                ignored_crossings);
        }
        current = integrated.event->state;

        const ThinDiskRecordResult recorded = recorder.record(
            metric,
            current,
            initialized.measured_frequency);
        if (!recorded) {
            return transfer_failure(
                recorded.message.empty()
                    ? "thin_disk_transfer_failure"
                    : recorded.message,
                ignored_crossings);
        }
        if (!recorded.recorded) {
            ++ignored_crossings;
        }
        if (recorded.closed) {
            return SolarKerrPathTrace{
                build_solar_kerr_path_result(
                    metric,
                    current,
                    initial_constants,
                    evidence,
                    recorder,
                    TerminationReason::DiskSurfaceHit),
                ignored_crossings,
            };
        }

        const auto following_direction =
            next_solar_disk_direction(
                current, config.initial_mino_step);
        if (!following_direction) {
            return transfer_failure(
                "indeterminate_disk_crossing_direction",
                ignored_crossings);
        }
        disk_direction = *following_direction;
    }
    return SolarKerrPathTrace{
        build_solar_kerr_path_result(
            metric,
            current,
            initial_constants,
            evidence,
            recorder,
            TerminationReason::MaxSteps),
        ignored_crossings,
    };
}

} // namespace gargantua::reference::detail
