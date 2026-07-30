#pragma once

#include "gargantua/reference/reference_frame.h"

namespace gargantua::reference::detail {

bool summarize_reference_rays(
    const std::vector<ReferenceRayResult>& rays,
    ReferenceFrameSummary& summary) noexcept;

bool reference_frame_summaries_equal(
    const ReferenceFrameSummary& left,
    const ReferenceFrameSummary& right) noexcept;

} // namespace gargantua::reference::detail
