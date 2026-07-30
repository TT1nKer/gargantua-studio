#include "solar_kerr_path_setup.h"

#include "gargantua/reference/reference_numerics.h"

#include <cmath>

namespace gargantua::reference::detail {
namespace {

using namespace solar::relativity;

constexpr double half_pi =
    1.570796326794896619231321691639751442;
constexpr double equatorial_tolerance = 1.0e-12;
constexpr double polar_direction_tolerance = 1.0e-14;

} // namespace

bool is_solar_equatorial_plane(double theta) noexcept {
    return std::isfinite(theta) &&
           std::fabs(theta - half_pi) <=
               equatorial_tolerance;
}

std::optional<EventDirection> next_solar_disk_direction(
    const PhaseSpaceState& state,
    double mino_step) noexcept {
    const double polar_change =
        std::copysign(1.0, mino_step) * state.p.v[2];
    if (!std::isfinite(polar_change) ||
        std::fabs(polar_change) <= polar_direction_tolerance) {
        return std::nullopt;
    }
    return polar_change > 0.0
               ? EventDirection::Decreasing
               : EventDirection::Increasing;
}

std::vector<GeodesicEvent> make_solar_kerr_path_events(
    const ReferenceScene& scene,
    double capture_radius,
    EventDirection disk_direction) {
    const double radial_tolerance = 1.0e-10 * scene.mass_M;
    return {
        GeodesicEvent{
            "bl-capture-cutoff",
            [capture_radius](const PhaseSpaceState& state) {
                return state.x.v[1] - capture_radius;
            },
            EventDirection::Decreasing,
            TerminationReason::InteriorCutoff,
            radial_tolerance,
        },
        GeodesicEvent{
            "escape-radius",
            [escape_radius = scene.escape_radius_M](
                const PhaseSpaceState& state) {
                return state.x.v[1] - escape_radius;
            },
            EventDirection::Increasing,
            TerminationReason::Escaped,
            radial_tolerance,
        },
        GeodesicEvent{
            "equatorial-thin-disk",
            [](const PhaseSpaceState& state) {
                return state.x.v[2] - half_pi;
            },
            disk_direction,
            TerminationReason::DiskSurfaceHit,
            equatorial_tolerance,
        },
    };
}

KerrSeparatedConfig make_solar_kerr_separated_config(
    const ReferenceScene& scene,
    double remaining_affine) {
    KerrSeparatedConfig config =
        KerrSeparatedConfig::cpu_reference(
            GeodesicKind::Null,
            scene.mass_M,
            -scene.initial_step_M,
            scene.max_step_M,
            remaining_affine);
    config.dopri5.relative_tolerance =
        reference_separated_relative_tolerance;
    // Turning-potential conditioning is distinct from the stricter
    // Hamiltonian and invariant gates applied to every completed ray.
    config.potential_tolerance =
        reference_separated_potential_tolerance;
    config.root_tolerance =
        reference_separated_root_tolerance;
    return config;
}

} // namespace gargantua::reference::detail
