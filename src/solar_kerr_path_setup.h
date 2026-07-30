#pragma once

#include "gargantua/reference/reference_scene.h"
#include "solar/relativity/geodesic_types.h"
#include "solar/relativity/kerr_separated.h"

#include <optional>
#include <vector>

namespace gargantua::reference::detail {

bool is_solar_equatorial_plane(double theta) noexcept;

std::optional<solar::relativity::EventDirection>
next_solar_disk_direction(
    const solar::relativity::PhaseSpaceState& state,
    double mino_step) noexcept;

std::vector<solar::relativity::GeodesicEvent>
make_solar_kerr_path_events(
    const ReferenceScene& scene,
    double capture_radius,
    solar::relativity::EventDirection disk_direction);

solar::relativity::KerrSeparatedConfig
make_solar_kerr_separated_config(
    const ReferenceScene& scene,
    double remaining_affine);

} // namespace gargantua::reference::detail
