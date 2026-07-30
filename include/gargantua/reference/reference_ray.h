#pragma once

#include <cstddef>
#include <string>

namespace gargantua::reference {

enum class RayClassification {
    CapturedAtBlCutoff,
    Escaped,
    DiskSurfaceHit,
    Unconverged,
    ConstraintViolation,
    InitializationError,
    TransferFailure,
};

const char* ray_classification_name(
    RayClassification value) noexcept;
bool is_failed_classification(
    RayClassification value) noexcept;

struct ReferenceRayResult {
    RayClassification classification;
    std::string termination_reason;
    double final_affine_M;
    double final_radius_M;
    double min_radius_M;
    double winding;
    double max_constraint_error;
    double max_energy_rel_error;
    double max_lz_rel_error;
    double max_carter_rel_error;
    std::size_t accepted_steps;
    std::size_t rejected_steps;
    double disk_radius_M;
    double redshift_g;
    double observed_temperature;
    double observed_specific_intensity;
    double observed_bolometric_intensity;
    std::size_t disk_crossings;
};

} // namespace gargantua::reference
