#pragma once

#include "gargantua/reference/perspective_camera.h"
#include "gargantua/reference/reference_ray.h"

#include <memory>
#include <string>

namespace gargantua::reference {

struct ReferenceTracerInfo {
    std::string solar_version;
    std::string physics_contract;
    double capture_radius_M;
};

class ReferenceRayTracer {
public:
    virtual ~ReferenceRayTracer() = default;

    virtual const ReferenceTracerInfo&
    info() const noexcept = 0;
    virtual ReferenceRayResult trace(
        const CameraRay& ray) const = 0;
};

struct ReferenceTracerBuild {
    std::unique_ptr<ReferenceRayTracer> tracer;
    std::string message;

    explicit operator bool() const noexcept {
        return tracer != nullptr;
    }
};

ReferenceTracerBuild make_solar_kerr_ray_tracer(
    const ReferenceScene& scene);

} // namespace gargantua::reference
