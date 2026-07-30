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

const char* opacity_name(
    ReferenceDiskOpacity opacity) noexcept {
    return opacity == ReferenceDiskOpacity::Opaque
               ? "opaque"
               : "semi-transparent";
}

} // namespace

std::string serialize_reference_manifest(
    const ReferenceFrame& frame,
    std::uint64_t scene_hash,
    std::uint64_t beauty_ppm_checksum,
    std::size_t beauty_ppm_bytes,
    std::uint64_t classification_ppm_checksum,
    std::size_t classification_ppm_bytes,
    std::uint64_t csv_checksum,
    std::size_t csv_bytes) {
    const ReferenceScene& scene = frame.scene;
    const ReferenceFrameSummary& summary = frame.summary;

    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << std::setprecision(17)
           << '{'
           << "\"schema\":\"gargantua.reference-frame.v2\","
           << "\"status\":"
           << json_string(frame_status_name(frame.status)) << ','
           << "\"mode\":\"SCIENTIFIC_REFERENCE\","
           << "\"checksum_algorithm\":\"fnv1a64\","
           << "\"gargantua\":{"
           << "\"version\":"
           << json_string(std::string(gargantua::version)) << ','
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
           << "\"solver\":\"kerr-separated-mino\","
           << "\"direction\":\"observer-to-past\","
           << "\"photon_orientation\":\"future-directed\","
           << "\"capture_semantics\":\"interior_cutoff\","
           << "\"capture_radius_M\":"
           << frame.tracer.capture_radius_M << ','
           << "\"surface_model\":\"analytic-circular-thin-disk\","
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
           << "\"initial_mino_step\":"
           << scene.initial_step_M << ','
           << "\"max_mino_step\":" << scene.max_step_M << ','
           << "\"disk\":{"
           << "\"outer_radius_M\":"
           << scene.disk.outer_radius_M << ','
           << "\"inner_edge\":\"prograde-isco\","
           << "\"density_scale\":"
           << scene.disk.density_scale << ','
           << "\"temperature_scale\":"
           << scene.disk.temperature_scale << ','
           << "\"density_power\":"
           << scene.disk.density_power << ','
           << "\"specific_intensity_scale\":"
           << scene.disk.specific_intensity_scale << ','
           << "\"bolometric_intensity_scale\":"
           << scene.disk.bolometric_intensity_scale << ','
           << "\"opacity\":"
           << json_string(opacity_name(scene.disk.opacity)) << ','
           << "\"surface_optical_depth\":"
           << scene.disk.surface_optical_depth << ','
           << "\"max_crossings\":"
           << scene.disk.max_crossings
           << "}},"
           << "\"summary\":{"
           << "\"captured\":" << summary.captured << ','
           << "\"escaped\":" << summary.escaped << ','
           << "\"disk_surface_hits\":"
           << summary.disk_surface_hits << ','
           << "\"unconverged\":" << summary.unconverged << ','
           << "\"constraint_violations\":"
           << summary.constraint_violations << ','
           << "\"initialization_errors\":"
           << summary.initialization_errors << ','
           << "\"transfer_failures\":"
           << summary.transfer_failures << ','
           << "\"failed\":" << summary.failed << ','
           << "\"disk_crossings\":"
           << summary.disk_crossings << ','
           << "\"max_constraint_error\":"
           << summary.max_constraint_error << ','
           << "\"max_energy_rel_error\":"
           << summary.max_energy_rel_error << ','
           << "\"max_lz_rel_error\":"
           << summary.max_lz_rel_error << ','
           << "\"max_carter_rel_error\":"
           << summary.max_carter_rel_error << ','
           << "\"max_redshift_g\":"
           << summary.max_redshift_g << ','
           << "\"max_observed_specific_intensity\":"
           << summary.max_observed_specific_intensity << ','
           << "\"max_observed_bolometric_intensity\":"
           << summary.max_observed_bolometric_intensity << ','
           << "\"max_accepted_steps\":"
           << summary.max_accepted_steps << ','
           << "\"max_rejected_steps\":"
           << summary.max_rejected_steps
           << "},"
           << "\"display\":{"
           << "\"transform\":\"reinhard-srgb-v1\","
           << "\"source\":\"observed_bolometric_intensity\","
           << "\"exposure\":"
           << scene.disk.display_exposure
           << "},"
           << "\"scene_hash_fnv1a64\":"
           << json_string(hex64(scene_hash)) << ','
           << "\"files\":{"
           << "\"beauty.ppm\":{"
           << "\"bytes\":" << beauty_ppm_bytes << ','
           << "\"checksum_fnv1a64\":"
           << json_string(hex64(beauty_ppm_checksum)) << "},"
           << "\"classification.ppm\":{"
           << "\"bytes\":" << classification_ppm_bytes << ','
           << "\"checksum_fnv1a64\":"
           << json_string(
                  hex64(classification_ppm_checksum)) << "},"
           << "\"rays.csv\":{"
           << "\"bytes\":" << csv_bytes << ','
           << "\"checksum_fnv1a64\":"
           << json_string(hex64(csv_checksum)) << "}"
           << "},"
           << "\"missing_capabilities\":["
           << "\"volume_transfer\","
           << "\"grmhd\","
           << "\"polarization\","
           << "\"returning_radiation\","
           << "\"spectral_calibration\","
           << "\"kerr_schild_horizon_crossing\","
           << "\"bl_polar_axis_crossing\","
           << "\"cuda\","
           << "\"openexr_aces\","
           << "\"film_pipeline\""
           << "]}\n";
    return output.str();
}

} // namespace gargantua::reference::detail
