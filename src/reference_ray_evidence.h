#pragma once

#include "gargantua/reference/reference_ray.h"

#include <string>

namespace gargantua::reference::detail {

ReferenceRayResult unavailable_reference_ray(
    RayClassification classification,
    std::string reason);

bool valid_reference_ray_evidence(
    const ReferenceRayResult& ray) noexcept;

} // namespace gargantua::reference::detail
