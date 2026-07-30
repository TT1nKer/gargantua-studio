#include "reference_manifest.h"

#include "gargantua/build_metadata.h"
#include "gargantua/reference/reference_numerics.h"
#include "gargantua/version.h"

#include <iomanip>
#include <locale>
#include <sstream>

#ifndef GARGANTUA_BUILD_GIT_COMMIT
#error "Gargantua Git commit must be supplied by CMake"
#endif

#ifndef GARGANTUA_BUILD_DIRTY
#error "Gargantua dirty-state flag must be supplied by CMake"
#endif

#ifndef GARGANTUA_SOLAR_GIT_COMMIT
#error "Locked Solar Git commit must be supplied by CMake"
#endif

namespace gargantua::reference::detail {
namespace {

std::string json_string(const std::string& value) {
    std::ostringstream output;
    output << '"';
    for (const unsigned char character : value) {
        switch (character) {
        case '"':
            output << "\\\"";
            break;
        case '\\':
            output << "\\\\";
            break;
        case '\b':
            output << "\\b";
            break;
        case '\f':
            output << "\\f";
            break;
        case '\n':
            output << "\\n";
            break;
        case '\r':
            output << "\\r";
            break;
        case '\t':
            output << "\\t";
            break;
        default:
            if (character < 0x20) {
                output << "\\u"
                       << std::hex << std::setw(4)
                       << std::setfill('0')
                       << static_cast<unsigned int>(character)
                       << std::dec;
            } else {
                output << static_cast<char>(character);
            }
        }
    }
    output << '"';
    return output.str();
}

std::string hex64(std::uint64_t value) {
    std::ostringstream output;
    output << std::hex << std::setw(16)
           << std::setfill('0') << value;
    return output.str();
}

} // namespace

std::string serialize_reference_manifest(
    const ReferenceFrame& frame,
    std::uint64_t scene_hash,
    std::uint64_t ppm_checksum,
    std::size_t ppm_bytes,
    std::uint64_t csv_checksum,
    std::size_t csv_bytes) {
    const ReferenceScene& scene = frame.scene;
    const ReferenceFrameSummary& summary = frame.summary;

    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << std::setprecision(17)
           << '{'
           << "\"schema\":\"gargantua.reference-frame.v1\","
           << "\"status\":"
           << json_string(frame_status_name(frame.status)) << ','
           << "\"mode\":\"ENGINE_DEBUG\","
           << "\"checksum_algorithm\":\"fnv1a64\","
           << "\"gargantua\":{"
           << "\"version\":" << json_string(
                  std::string(gargantua::version)) << ','
           << "\"git_commit\":"
           << json_string(GARGANTUA_BUILD_GIT_COMMIT) << ','
           << "\"dirty\":"
           << (GARGANTUA_BUILD_DIRTY ? "true" : "false")
           << "},"
           << "\"solar\":{"
           << "\"version\":"
           << json_string(frame.tracer.solar_version) << ','
           << "\"physics_contract\":"
           << json_string(frame.tracer.physics_contract) << ','
           << "\"git_commit\":"
           << json_string(GARGANTUA_SOLAR_GIT_COMMIT)
           << "},"
           << "\"physics\":{"
           << "\"metric\":\"kerr-bl\","
           << "\"coordinates\":\"boyer-lindquist\","
           << "\"units\":\"G=c=1\","
           << "\"geodesic_kind\":\"null\","
           << "\"capture_semantics\":\"interior_cutoff\","
           << "\"capture_radius_M\":"
           << frame.tracer.capture_radius_M << ','
           << "\"hamiltonian_gate\":"
           << reference_hamiltonian_error_gate << ','
           << "\"energy_gate\":"
           << reference_stationary_invariant_error_gate << ','
           << "\"lz_gate\":"
           << reference_stationary_invariant_error_gate << ','
           << "\"carter_gate\":"
           << reference_carter_relative_error_gate
           << "},"
           << "\"scene\":{"
           << "\"mass_M\":" << scene.mass_M << ','
           << "\"spin_chi\":" << scene.spin_chi << ','
           << "\"observer_radius_M\":"
           << scene.observer_radius_M << ','
           << "\"inclination_radians\":"
           << scene.inclination_radians << ','
           << "\"vertical_fov_radians\":"
           << scene.vertical_fov_radians << ','
           << "\"width\":" << scene.width << ','
           << "\"height\":" << scene.height << ','
           << "\"escape_radius_M\":"
           << scene.escape_radius_M << ','
           << "\"max_affine_M\":" << scene.max_affine_M << ','
           << "\"initial_step_M\":" << scene.initial_step_M << ','
           << "\"max_step_M\":" << scene.max_step_M
           << "},"
           << "\"summary\":{"
           << "\"captured\":" << summary.captured << ','
           << "\"escaped\":" << summary.escaped << ','
           << "\"unconverged\":" << summary.unconverged << ','
           << "\"constraint_violations\":"
           << summary.constraint_violations << ','
           << "\"initialization_errors\":"
           << summary.initialization_errors << ','
           << "\"failed\":" << summary.failed << ','
           << "\"max_constraint_error\":"
           << summary.max_constraint_error << ','
           << "\"max_energy_rel_error\":"
           << summary.max_energy_rel_error << ','
           << "\"max_lz_rel_error\":"
           << summary.max_lz_rel_error << ','
           << "\"max_carter_rel_error\":"
           << summary.max_carter_rel_error << ','
           << "\"max_accepted_steps\":"
           << summary.max_accepted_steps << ','
           << "\"max_rejected_steps\":"
           << summary.max_rejected_steps
           << "},"
           << "\"scene_hash_fnv1a64\":"
           << json_string(hex64(scene_hash)) << ','
           << "\"files\":{"
           << "\"classification.ppm\":{"
           << "\"bytes\":" << ppm_bytes << ','
           << "\"checksum_fnv1a64\":"
           << json_string(hex64(ppm_checksum)) << "},"
           << "\"rays.csv\":{"
           << "\"bytes\":" << csv_bytes << ','
           << "\"checksum_fnv1a64\":"
           << json_string(hex64(csv_checksum)) << "}"
           << "},"
           << "\"missing_capabilities\":["
           << "\"trajectory_min_radius\","
           << "\"azimuthal_winding\","
           << "\"disk_intersections\","
           << "\"redshift_and_radiative_transfer\","
           << "\"kerr_schild_horizon_crossing\","
           << "\"bl_polar_axis_crossing\","
           << "\"separated_mino_solver\","
           << "\"cuda\","
           << "\"openexr_aces\","
           << "\"beauty_render\""
           << "]}\n";
    return output.str();
}

} // namespace gargantua::reference::detail
