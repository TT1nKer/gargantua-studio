#include "reference_serialization.h"

#include "reference_frame_summary.h"
#include "reference_image_encoding.h"
#include "reference_manifest.h"

#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <utility>

namespace gargantua::reference::detail {
namespace {

constexpr std::uint64_t fnv_offset = 14695981039346656037ULL;
constexpr std::uint64_t fnv_prime = 1099511628211ULL;

std::uint64_t fnv1a64(
    const unsigned char* bytes,
    std::size_t size) noexcept {
    std::uint64_t hash = fnv_offset;
    for (std::size_t index = 0; index < size; ++index) {
        hash ^= bytes[index];
        hash *= fnv_prime;
    }
    return hash;
}

std::uint64_t fnv1a64(const std::string& value) noexcept {
    return fnv1a64(
        reinterpret_cast<const unsigned char*>(value.data()),
        value.size());
}

std::string csv_field(const std::string& value) {
    if (value.find_first_of(",\"\r\n") == std::string::npos) {
        return value;
    }
    std::string escaped{"\""};
    for (const char character : value) {
        if (character == '"') {
            escaped += '"';
        }
        escaped += character;
    }
    escaped += '"';
    return escaped;
}

bool frame_shape_is_consistent(const ReferenceFrame& frame) {
    const SceneValidation validation =
        validate_reference_scene(frame.scene);
    if (!validation) {
        return false;
    }
    if (frame.tracer.solar_version.empty() ||
        frame.tracer.physics_contract.empty() ||
        !std::isfinite(frame.tracer.capture_radius_M)) {
        return false;
    }
    const double horizon_radius =
        frame.scene.mass_M *
        (1.0 + std::sqrt(
            1.0 -
            frame.scene.spin_chi * frame.scene.spin_chi));
    if (frame.tracer.capture_radius_M <= horizon_radius ||
        frame.tracer.capture_radius_M >=
            frame.scene.observer_radius_M) {
        return false;
    }
    const std::size_t pixel_count =
        frame.scene.width * frame.scene.height;
    if (frame.rays.size() != pixel_count) {
        return false;
    }

    ReferenceFrameSummary derived_summary;
    if (!summarize_reference_rays(
            frame.rays, derived_summary) ||
        !reference_frame_summaries_equal(
            frame.summary, derived_summary)) {
        return false;
    }
    const FrameStatus expected_status =
        derived_summary.failed == 0
            ? FrameStatus::Complete
            : FrameStatus::DiagnosticFailed;
    return frame.status == expected_status;
}

const char* opacity_name(
    ReferenceDiskOpacity opacity) noexcept {
    return opacity == ReferenceDiskOpacity::Opaque
               ? "opaque"
               : "semi-transparent";
}

std::string canonical_scene(const ReferenceScene& scene) {
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << std::setprecision(17)
           << "mass_M=" << scene.mass_M << '\n'
           << "spin_chi=" << scene.spin_chi << '\n'
           << "observer_radius_M=" << scene.observer_radius_M << '\n'
           << "inclination_radians=" << scene.inclination_radians << '\n'
           << "vertical_fov_radians=" << scene.vertical_fov_radians << '\n'
           << "width=" << scene.width << '\n'
           << "height=" << scene.height << '\n'
           << "escape_radius_M=" << scene.escape_radius_M << '\n'
           << "max_affine_M=" << scene.max_affine_M << '\n'
           << "initial_step_M=" << scene.initial_step_M << '\n'
           << "max_step_M=" << scene.max_step_M << '\n'
           << "disk.outer_radius_M="
           << scene.disk.outer_radius_M << '\n'
           << "disk.density_scale="
           << scene.disk.density_scale << '\n'
           << "disk.temperature_scale="
           << scene.disk.temperature_scale << '\n'
           << "disk.density_power="
           << scene.disk.density_power << '\n'
           << "disk.specific_intensity_scale="
           << scene.disk.specific_intensity_scale << '\n'
           << "disk.bolometric_intensity_scale="
           << scene.disk.bolometric_intensity_scale << '\n'
           << "disk.opacity="
           << opacity_name(scene.disk.opacity) << '\n'
           << "disk.surface_optical_depth="
           << scene.disk.surface_optical_depth << '\n'
           << "disk.max_crossings="
           << scene.disk.max_crossings << '\n'
           << "disk.display_exposure="
           << scene.disk.display_exposure << '\n';
    return output.str();
}

std::string serialize_ray_csv(const ReferenceFrame& frame) {
    std::ostringstream csv;
    csv.imbue(std::locale::classic());
    csv << std::setprecision(17)
        << "pixel_x,pixel_y,classification,termination_reason,"
        << "final_affine_M,final_radius_M,min_radius_M,winding,"
        << "max_constraint_error,max_energy_rel_error,max_lz_rel_error,"
        << "max_carter_rel_error,accepted_steps,rejected_steps,"
        << "disk_radius_M,redshift_g,observed_temperature,"
        << "observed_specific_intensity,observed_bolometric_intensity,"
        << "disk_crossings\n";
    for (std::size_t index = 0; index < frame.rays.size(); ++index) {
        const ReferenceRayResult& ray = frame.rays[index];
        csv << index % frame.scene.width << ','
            << index / frame.scene.width << ','
            << ray_classification_name(ray.classification) << ','
            << csv_field(ray.termination_reason) << ','
            << ray.final_affine_M << ','
            << ray.final_radius_M << ','
            << ray.min_radius_M << ','
            << ray.winding << ','
            << ray.max_constraint_error << ','
            << ray.max_energy_rel_error << ','
            << ray.max_lz_rel_error << ','
            << ray.max_carter_rel_error << ','
            << ray.accepted_steps << ','
            << ray.rejected_steps << ','
            << ray.disk_radius_M << ','
            << ray.redshift_g << ','
            << ray.observed_temperature << ','
            << ray.observed_specific_intensity << ','
            << ray.observed_bolometric_intensity << ','
            << ray.disk_crossings << '\n';
    }
    return csv.str();
}

} // namespace

SerializedReferenceGeneration serialize_reference_generation(
    const ReferenceFrame& frame) {
    if (!frame_shape_is_consistent(frame)) {
        SerializedReferenceGeneration failure;
        failure.message =
            "reference frame shape or summary is inconsistent";
        return failure;
    }

    EncodedReferenceImages images =
        encode_reference_images(frame);
    const std::string csv = serialize_ray_csv(frame);
    const std::uint64_t beauty_checksum = fnv1a64(
        images.beauty_ppm.data(), images.beauty_ppm.size());
    const std::uint64_t classification_checksum = fnv1a64(
        images.classification_ppm.data(),
        images.classification_ppm.size());
    const std::uint64_t csv_checksum = fnv1a64(csv);
    const std::uint64_t scene_hash =
        fnv1a64(canonical_scene(frame.scene));
    std::string manifest = serialize_reference_manifest(
        frame,
        scene_hash,
        beauty_checksum,
        images.beauty_ppm.size(),
        classification_checksum,
        images.classification_ppm.size(),
        csv_checksum,
        csv.size());
    return SerializedReferenceGeneration{
        true,
        {},
        std::move(images.beauty_ppm),
        std::move(images.classification_ppm),
        csv,
        std::move(manifest),
        beauty_checksum,
        classification_checksum,
        csv_checksum,
    };
}

} // namespace gargantua::reference::detail
