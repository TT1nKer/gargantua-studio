#pragma once

#include "gargantua/reference/perspective_camera.h"
#include "gargantua/reference/reference_ray.h"
#include "gargantua/reference/reference_scene.h"

#include <cstddef>

namespace solar::relativity {
class KerrBoyerLindquistMetric;
struct ObserverFrame;
} // namespace solar::relativity

namespace gargantua::reference::detail {

struct SolarKerrPathTrace {
    ReferenceRayResult ray;
    std::size_t ignored_surface_crossings = 0;
};

void validate_solar_kerr_path_scene(
    const solar::relativity::KerrBoyerLindquistMetric& metric,
    const ReferenceScene& scene);

SolarKerrPathTrace trace_solar_kerr_path(
    const solar::relativity::KerrBoyerLindquistMetric& metric,
    const solar::relativity::ObserverFrame& observer,
    const ReferenceScene& scene,
    const CameraRay& ray);

} // namespace gargantua::reference::detail
