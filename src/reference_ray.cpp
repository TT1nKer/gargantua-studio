#include "gargantua/reference/reference_ray.h"

namespace gargantua::reference {

const char* ray_classification_name(
    RayClassification value) noexcept {
    switch (value) {
    case RayClassification::CapturedAtBlCutoff:
        return "captured_at_bl_cutoff";
    case RayClassification::Escaped:
        return "escaped";
    case RayClassification::Unconverged:
        return "unconverged";
    case RayClassification::ConstraintViolation:
        return "constraint_violation";
    case RayClassification::InitializationError:
        return "initialization_error";
    }
    return "unknown";
}

bool is_failed_classification(
    RayClassification value) noexcept {
    return value != RayClassification::CapturedAtBlCutoff &&
           value != RayClassification::Escaped;
}

} // namespace gargantua::reference
