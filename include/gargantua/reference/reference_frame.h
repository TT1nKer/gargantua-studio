#pragma once

#include "gargantua/reference/reference_ray.h"
#include "gargantua/reference/reference_ray_tracer.h"
#include "gargantua/reference/reference_scene.h"

#include <cstddef>
#include <vector>

namespace gargantua::reference {

enum class FrameStatus {
    Complete,
    DiagnosticFailed,
};

const char* frame_status_name(FrameStatus status) noexcept;

struct ReferenceFrameSummary {
    std::size_t captured = 0;
    std::size_t escaped = 0;
    std::size_t unconverged = 0;
    std::size_t constraint_violations = 0;
    std::size_t initialization_errors = 0;
    std::size_t failed = 0;
    double max_constraint_error = 0.0;
    double max_energy_rel_error = 0.0;
    double max_lz_rel_error = 0.0;
    double max_carter_rel_error = 0.0;
    std::size_t max_accepted_steps = 0;
    std::size_t max_rejected_steps = 0;
};

struct ReferenceFrame {
    ReferenceScene scene;
    ReferenceTracerInfo tracer;
    std::vector<ReferenceRayResult> rays;
    ReferenceFrameSummary summary;
    FrameStatus status;
};

} // namespace gargantua::reference
