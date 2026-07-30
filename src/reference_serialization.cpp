#include "reference_serialization.h"

#include "reference_manifest.h"

#include <array>
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

std::array<unsigned char, 3> classification_color(
    RayClassification classification) noexcept {
    switch (classification) {
    case RayClassification::CapturedAtBlCutoff:
        return {{0, 0, 0}};
    case RayClassification::Escaped:
        return {{235, 235, 235}};
    case RayClassification::Unconverged:
        return {{255, 0, 255}};
    case RayClassification::ConstraintViolation:
        return {{255, 64, 0}};
    case RayClassification::InitializationError:
        return {{255, 255, 0}};
    }
    return {{255, 255, 0}};
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
    const std::size_t pixel_count =
        frame.scene.width * frame.scene.height;
    const ReferenceFrameSummary& summary = frame.summary;
    const std::size_t classified =
        summary.captured +
        summary.escaped +
        summary.unconverged +
        summary.constraint_violations +
        summary.initialization_errors;
    const std::size_t failed =
        summary.unconverged +
        summary.constraint_violations +
        summary.initialization_errors;
    return frame.rays.size() == pixel_count &&
           classified == pixel_count &&
           summary.failed == failed &&
           ((failed == 0) ==
            (frame.status == FrameStatus::Complete));
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
           << "max_step_M=" << scene.max_step_M << '\n';
    return output.str();
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

    std::ostringstream ppm_header;
    ppm_header << "P6\n" << frame.scene.width << ' '
               << frame.scene.height << "\n255\n";
    const std::string header = ppm_header.str();
    std::vector<unsigned char> ppm(
        header.begin(), header.end());
    ppm.reserve(header.size() + 3 * frame.rays.size());
    for (const ReferenceRayResult& ray : frame.rays) {
        const auto color =
            classification_color(ray.classification);
        ppm.insert(ppm.end(), color.begin(), color.end());
    }

    std::ostringstream csv;
    csv.imbue(std::locale::classic());
    csv << std::setprecision(17)
        << "pixel_x,pixel_y,classification,termination_reason,"
        << "final_radius_M,max_constraint_error,"
        << "max_energy_rel_error,max_lz_rel_error,"
        << "max_carter_rel_error,accepted_steps,rejected_steps\n";
    for (std::size_t index = 0; index < frame.rays.size(); ++index) {
        const ReferenceRayResult& ray = frame.rays[index];
        csv << index % frame.scene.width << ','
            << index / frame.scene.width << ','
            << ray_classification_name(ray.classification) << ','
            << csv_field(ray.termination_reason) << ','
            << ray.final_radius_M << ','
            << ray.max_constraint_error << ','
            << ray.max_energy_rel_error << ','
            << ray.max_lz_rel_error << ','
            << ray.max_carter_rel_error << ','
            << ray.accepted_steps << ','
            << ray.rejected_steps << '\n';
    }

    const std::string csv_text = csv.str();
    const std::uint64_t ppm_checksum =
        fnv1a64(ppm.data(), ppm.size());
    const std::uint64_t csv_checksum = fnv1a64(csv_text);
    const std::uint64_t scene_hash =
        fnv1a64(canonical_scene(frame.scene));
    return SerializedReferenceGeneration{
        true,
        {},
        std::move(ppm),
        csv_text,
        serialize_reference_manifest(
            frame,
            scene_hash,
            ppm_checksum,
            header.size() + 3 * frame.rays.size(),
            csv_checksum,
            csv_text.size()),
        ppm_checksum,
        csv_checksum,
    };
}

} // namespace gargantua::reference::detail
