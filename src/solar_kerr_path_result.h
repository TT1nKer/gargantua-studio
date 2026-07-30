#pragma once

#include "gargantua/reference/reference_ray.h"
#include "solar/relativity/kerr_constants.h"
#include "solar/relativity/kerr_separated.h"
#include "solar/relativity/thin_disk.h"

#include <cstddef>
#include <limits>

namespace gargantua::reference::detail {

struct SolarKerrPathEvidence {
    std::size_t accepted_steps = 0;
    std::size_t rejected_steps = 0;
    double min_radius =
        std::numeric_limits<double>::infinity();
    double winding = 0.0;
    double max_constraint = 0.0;
    double max_carter = 0.0;

    void include(
        const solar::relativity::KerrSeparatedDiagnostics&
            diagnostics);
};

ReferenceRayResult build_solar_kerr_path_result(
    const solar::relativity::KerrBoyerLindquistMetric& metric,
    const solar::relativity::PhaseSpaceState& final_state,
    const solar::relativity::KerrConstants& initial_constants,
    const SolarKerrPathEvidence& evidence,
    const solar::relativity::ThinDiskCrossingRecorder& recorder,
    solar::relativity::TerminationReason reason);

} // namespace gargantua::reference::detail
