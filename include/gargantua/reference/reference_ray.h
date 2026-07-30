#pragma once

#include <cstddef>
#include <string>

namespace gargantua::reference {

enum class RayClassification {
    CapturedAtBlCutoff,
    Escaped,
    Unconverged,
    ConstraintViolation,
    InitializationError,
};

const char* ray_classification_name(
    RayClassification value) noexcept;
bool is_failed_classification(
    RayClassification value) noexcept;

struct ReferenceRayResult {
    RayClassification classification;
    std::string termination_reason;
    double final_radius_M;
    double max_constraint_error;
    double max_energy_rel_error;
    double max_lz_rel_error;
    double max_carter_rel_error;
    std::size_t accepted_steps;
    std::size_t rejected_steps;
};

} // namespace gargantua::reference
