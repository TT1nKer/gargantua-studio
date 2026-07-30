#pragma once

#include <cstddef>
#include <string>

namespace gargantua::reference {

enum class ReferenceDiskOpacity {
    Opaque,
    SemiTransparent,
};

struct ScientificDiskScene {
    double outer_radius_M = 20.0;
    double density_scale = 1.0;
    double temperature_scale = 1.0;
    double density_power = 0.75;
    double specific_intensity_scale = 1.0;
    double bolometric_intensity_scale = 1.0;
    ReferenceDiskOpacity opacity = ReferenceDiskOpacity::Opaque;
    double surface_optical_depth = 1.0;
    std::size_t max_crossings = 8;
    double display_exposure = 1.0;
};

struct ReferenceScene {
    double mass_M;
    double spin_chi;
    double observer_radius_M;
    double inclination_radians;
    double vertical_fov_radians;
    std::size_t width;
    std::size_t height;
    double escape_radius_M;
    double max_affine_M;
    double initial_step_M;
    double max_step_M;
    ScientificDiskScene disk;
};

struct SceneValidation {
    bool valid;
    std::string message;

    explicit operator bool() const noexcept {
        return valid;
    }
};

ReferenceScene reference_scene_defaults() noexcept;
SceneValidation validate_reference_scene(
    const ReferenceScene& scene);

} // namespace gargantua::reference
