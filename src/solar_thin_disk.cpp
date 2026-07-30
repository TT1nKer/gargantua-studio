#include "solar_thin_disk.h"

#include "solar/relativity/fluid_model.h"

#include <optional>

namespace gargantua::reference::detail {
namespace {

using namespace solar::relativity;

AnalyticCircularDiskConfig disk_config(
    const ReferenceScene& scene) {
    return AnalyticCircularDiskConfig{
        scene.mass_M,
        scene.spin_chi,
        OrbitSense::Prograde,
        std::nullopt,
        scene.disk.outer_radius_M,
        scene.disk.density_scale,
        scene.disk.temperature_scale,
        scene.disk.density_power,
        1.0e-8,
    };
}

} // namespace

void validate_solar_thin_disk_scene(
    const ReferenceScene& scene) {
    // Solar owns the ISCO calculation and disk-domain validation.
    static_cast<void>(
        AnalyticCircularDiskFluid(disk_config(scene)));
}

ThinDiskCrossingRecorder make_solar_thin_disk_recorder(
    const ReferenceScene& scene) {
    const DiskOpacityMode opacity =
        scene.disk.opacity == ReferenceDiskOpacity::Opaque
            ? DiskOpacityMode::Opaque
            : DiskOpacityMode::SemiTransparent;
    return ThinDiskCrossingRecorder{
        ThinDiskRecorderConfig{
            opacity, scene.disk.max_crossings},
        AnalyticCircularDiskFluid(disk_config(scene)),
        ThinDiskSurfaceEmission(
            scene.disk.specific_intensity_scale,
            scene.disk.bolometric_intensity_scale,
            scene.disk.surface_optical_depth),
    };
}

} // namespace gargantua::reference::detail
