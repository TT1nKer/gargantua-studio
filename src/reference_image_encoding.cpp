#include "reference_image_encoding.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>

namespace gargantua::reference::detail {
namespace {

std::array<unsigned char, 3> classification_color(
    RayClassification classification) noexcept {
    switch (classification) {
    case RayClassification::CapturedAtBlCutoff:
        return {{0, 0, 0}};
    case RayClassification::Escaped:
        return {{235, 235, 235}};
    case RayClassification::DiskSurfaceHit:
        return {{0, 200, 255}};
    case RayClassification::Unconverged:
        return {{255, 0, 255}};
    case RayClassification::ConstraintViolation:
        return {{255, 64, 0}};
    case RayClassification::InitializationError:
        return {{255, 255, 0}};
    case RayClassification::TransferFailure:
        return {{255, 0, 0}};
    }
    return {{255, 255, 0}};
}

double linear_to_srgb(double value) noexcept {
    return value <= 0.0031308
               ? 12.92 * value
               : 1.055 * std::pow(value, 1.0 / 2.4) - 0.055;
}

std::array<unsigned char, 3> beauty_color(
    const ReferenceRayResult& ray,
    double exposure) noexcept {
    if (is_failed_classification(ray.classification)) {
        return classification_color(ray.classification);
    }
    const double linear =
        exposure * ray.observed_bolometric_intensity;
    const double mapped =
        std::isinf(linear)
            ? 1.0
            : linear / (1.0 + linear);
    const double srgb =
        linear_to_srgb(std::clamp(mapped, 0.0, 1.0));
    const unsigned char channel =
        static_cast<unsigned char>(
            std::lround(255.0 * srgb));
    return {{channel, channel, channel}};
}

std::vector<unsigned char> ppm_with_header(
    const ReferenceScene& scene) {
    std::ostringstream header;
    header << "P6\n" << scene.width << ' '
           << scene.height << "\n255\n";
    const std::string text = header.str();
    std::vector<unsigned char> ppm(text.begin(), text.end());
    ppm.reserve(text.size() + 3 * scene.width * scene.height);
    return ppm;
}

} // namespace

EncodedReferenceImages encode_reference_images(
    const ReferenceFrame& frame) {
    EncodedReferenceImages encoded{
        ppm_with_header(frame.scene),
        ppm_with_header(frame.scene),
    };
    for (const ReferenceRayResult& ray : frame.rays) {
        const auto beauty = beauty_color(
            ray, frame.scene.disk.display_exposure);
        encoded.beauty_ppm.insert(
            encoded.beauty_ppm.end(),
            beauty.begin(),
            beauty.end());

        const auto classification =
            classification_color(ray.classification);
        encoded.classification_ppm.insert(
            encoded.classification_ppm.end(),
            classification.begin(),
            classification.end());
    }
    return encoded;
}

} // namespace gargantua::reference::detail
