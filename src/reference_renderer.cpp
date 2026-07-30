#include "gargantua/reference/reference_renderer.h"

#include "gargantua/reference/perspective_camera.h"
#include "reference_frame_summary.h"

#include <exception>
#include <utility>

namespace gargantua::reference {

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
                frame.rays.push_back(std::move(ray));
            }
        }
        if (frame.rays.size() != pixel_count) {
            return ReferenceRenderResult{
                std::nullopt,
                "reference renderer produced an incomplete frame"};
        }
        if (!detail::summarize_reference_rays(
                frame.rays, frame.summary)) {
            return ReferenceRenderResult{
                std::nullopt,
                "reference tracer produced invalid ray evidence"};
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
