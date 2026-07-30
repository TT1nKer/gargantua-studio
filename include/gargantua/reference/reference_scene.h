#pragma once

#include <cstddef>
#include <string>

namespace gargantua::reference {

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
