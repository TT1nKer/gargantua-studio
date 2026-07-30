#include "gargantua/reference/reference_renderer.h"

#include "gargantua/reference/perspective_camera.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <utility>

namespace gargantua::reference {
namespace {

void retain_maximum(double value, double& maximum) {
    if (std::isfinite(value)) {
        maximum = std::max(maximum, value);
    }
}

void include_ray(
    const ReferenceRayResult& ray,
    ReferenceFrameSummary& summary) {
    switch (ray.classification) {
    case RayClassification::CapturedAtBlCutoff:
        ++summary.captured;
        break;
    case RayClassification::Escaped:
        ++summary.escaped;
        break;
    case RayClassification::Unconverged:
        ++summary.unconverged;
        break;
    case RayClassification::ConstraintViolation:
        ++summary.constraint_violations;
        break;
    case RayClassification::InitializationError:
        ++summary.initialization_errors;
        break;
    default:
        ++summary.unconverged;
        break;
    }
    if (is_failed_classification(ray.classification)) {
        ++summary.failed;
    }

    retain_maximum(
        ray.max_constraint_error,
        summary.max_constraint_error);
    retain_maximum(
        ray.max_energy_rel_error,
        summary.max_energy_rel_error);
    retain_maximum(
        ray.max_lz_rel_error,
        summary.max_lz_rel_error);
    retain_maximum(
        ray.max_carter_rel_error,
        summary.max_carter_rel_error);
    summary.max_accepted_steps =
        std::max(summary.max_accepted_steps, ray.accepted_steps);
    summary.max_rejected_steps =
        std::max(summary.max_rejected_steps, ray.rejected_steps);
}

} // namespace

ReferenceRenderResult render_reference_frame(
    const ReferenceScene& scene,
    const ReferenceRayTracer& tracer) {
    const SceneValidation validation =
        validate_reference_scene(scene);
    if (!validation) {
        return ReferenceRenderResult{
            std::nullopt, validation.message};
    }

    try {
        const std::size_t pixel_count =
            scene.width * scene.height;
        ReferenceFrame frame{
            scene,
            tracer.info(),
            {},
            {},
            FrameStatus::Complete,
        };
        frame.rays.reserve(pixel_count);
        for (std::size_t y = 0; y < scene.height; ++y) {
            for (std::size_t x = 0; x < scene.width; ++x) {
                ReferenceRayResult ray = tracer.trace(
                    perspective_camera_ray(scene, x, y));
                include_ray(ray, frame.summary);
                frame.rays.push_back(std::move(ray));
            }
        }
        if (frame.rays.size() != pixel_count) {
            return ReferenceRenderResult{
                std::nullopt,
                "reference renderer produced an incomplete frame"};
        }
        if (frame.summary.failed != 0) {
            frame.status = FrameStatus::DiagnosticFailed;
        }
        return ReferenceRenderResult{
            std::move(frame), {}};
    } catch (const std::exception& error) {
        return ReferenceRenderResult{
            std::nullopt,
            std::string("reference renderer failed: ") +
                error.what()};
    }
}

} // namespace gargantua::reference
