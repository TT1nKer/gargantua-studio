#pragma once

#include "gargantua/reference/reference_frame.h"

#include <optional>
#include <string>

namespace gargantua::reference {

struct ReferenceRenderResult {
    std::optional<ReferenceFrame> frame;
    std::string message;

    explicit operator bool() const noexcept {
        return frame.has_value();
    }
};

ReferenceRenderResult render_reference_frame(
    const ReferenceScene& scene,
    const ReferenceRayTracer& tracer);

} // namespace gargantua::reference
