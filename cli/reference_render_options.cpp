#include "reference_render_options.h"

#include <cerrno>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <set>
#include <string>
#include <system_error>
#include <utility>

namespace gargantua::cli {
namespace {

constexpr double pi = 3.141592653589793238462643383279502884;

ReferenceRenderParse failure(std::string message) {
    return ReferenceRenderParse{
        std::nullopt, false, std::move(message)};
}

bool parse_finite_double(
    const std::string& text,
    double& value) {
    if (text.empty() ||
        std::isspace(static_cast<unsigned char>(text.front())) ||
        std::isspace(static_cast<unsigned char>(text.back()))) {
        return false;
    }
    errno = 0;
    char* end = nullptr;
    const double parsed = std::strtod(text.c_str(), &end);
    if (errno == ERANGE ||
        end != text.c_str() + text.size() ||
        !std::isfinite(parsed)) {
        return false;
    }
    value = parsed;
    return true;
}

bool parse_size(
    const std::string& text,
    std::size_t& value) {
    if (text.empty() || text.front() == '-') {
        return false;
    }
    std::size_t parsed = 0;
    const auto result = std::from_chars(
        text.data(), text.data() + text.size(), parsed);
    if (result.ec != std::errc{} ||
        result.ptr != text.data() + text.size()) {
        return false;
    }
    value = parsed;
    return true;
}

} // namespace

ReferenceRenderParse parse_reference_render_options(
    const std::vector<std::string>& arguments) {
    if (arguments.size() == 2 && arguments[1] == "--help") {
        return ReferenceRenderParse{std::nullopt, true, {}};
    }
    if (arguments.empty()) {
        return failure("argument vector must include the executable name");
    }

    ReferenceRenderOptions options{
        reference::reference_scene_defaults(), {}};
    std::set<std::string> seen;
    bool has_output = false;

    for (std::size_t index = 1;
         index < arguments.size();
         index += 2) {
        const std::string& name = arguments[index];
        if (name == "--help") {
            return failure("--help cannot be combined with render options");
        }
        if (index + 1 >= arguments.size()) {
            return failure("missing value for option " + name);
        }
        if (!seen.insert(name).second) {
            return failure("option is repeated: " + name);
        }
        const std::string& text = arguments[index + 1];

        double number = 0.0;
        std::size_t size = 0;
        if (name == "--output") {
            if (text.empty()) {
                return failure("--output must not be empty");
            }
            options.output_directory = text;
            has_output = true;
        } else if (name == "--mass-M") {
            if (!parse_finite_double(text, number)) {
                return failure("--mass-M requires a finite number");
            }
            options.scene.mass_M = number;
        } else if (name == "--spin") {
            if (!parse_finite_double(text, number)) {
                return failure("--spin requires a finite number");
            }
            options.scene.spin_chi = number;
        } else if (name == "--observer-r-M") {
            if (!parse_finite_double(text, number)) {
                return failure("--observer-r-M requires a finite number");
            }
            options.scene.observer_radius_M = number;
        } else if (name == "--inclination-deg") {
            if (!parse_finite_double(text, number)) {
                return failure("--inclination-deg requires a finite number");
            }
            options.scene.inclination_radians = number * pi / 180.0;
        } else if (name == "--fov-y-deg") {
            if (!parse_finite_double(text, number)) {
                return failure("--fov-y-deg requires a finite number");
            }
            options.scene.vertical_fov_radians = number * pi / 180.0;
        } else if (name == "--width") {
            if (!parse_size(text, size)) {
                return failure("--width requires an unsigned integer");
            }
            options.scene.width = size;
        } else if (name == "--height") {
            if (!parse_size(text, size)) {
                return failure("--height requires an unsigned integer");
            }
            options.scene.height = size;
        } else if (name == "--escape-r-M") {
            if (!parse_finite_double(text, number)) {
                return failure("--escape-r-M requires a finite number");
            }
            options.scene.escape_radius_M = number;
        } else if (name == "--max-affine-M") {
            if (!parse_finite_double(text, number)) {
                return failure("--max-affine-M requires a finite number");
            }
            options.scene.max_affine_M = number;
        } else if (
            name == "--initial-mino-step" ||
            name == "--initial-step-M") {
            if (!parse_finite_double(text, number)) {
                return failure(
                    name + " requires a finite number");
            }
            options.scene.initial_step_M = number;
        } else if (
            name == "--max-mino-step" ||
            name == "--max-step-M") {
            if (!parse_finite_double(text, number)) {
                return failure(
                    name + " requires a finite number");
            }
            options.scene.max_step_M = number;
        } else if (name == "--disk-outer-r-M") {
            if (!parse_finite_double(text, number)) {
                return failure(
                    "--disk-outer-r-M requires a finite number");
            }
            options.scene.disk.outer_radius_M = number;
        } else if (name == "--disk-temperature-scale") {
            if (!parse_finite_double(text, number)) {
                return failure(
                    "--disk-temperature-scale requires a finite number");
            }
            options.scene.disk.temperature_scale = number;
        } else if (name == "--disk-density-scale") {
            if (!parse_finite_double(text, number)) {
                return failure(
                    "--disk-density-scale requires a finite number");
            }
            options.scene.disk.density_scale = number;
        } else if (name == "--disk-density-power") {
            if (!parse_finite_double(text, number)) {
                return failure(
                    "--disk-density-power requires a finite number");
            }
            options.scene.disk.density_power = number;
        } else if (name == "--disk-specific-scale") {
            if (!parse_finite_double(text, number)) {
                return failure(
                    "--disk-specific-scale requires a finite number");
            }
            options.scene.disk.specific_intensity_scale = number;
        } else if (name == "--disk-bolometric-scale") {
            if (!parse_finite_double(text, number)) {
                return failure(
                    "--disk-bolometric-scale requires a finite number");
            }
            options.scene.disk.bolometric_intensity_scale = number;
        } else if (name == "--disk-opacity") {
            if (text == "opaque") {
                options.scene.disk.opacity =
                    reference::ReferenceDiskOpacity::Opaque;
            } else if (text == "semi-transparent") {
                options.scene.disk.opacity =
                    reference::ReferenceDiskOpacity::SemiTransparent;
            } else {
                return failure(
                    "--disk-opacity must be opaque or semi-transparent");
            }
        } else if (name == "--disk-surface-optical-depth") {
            if (!parse_finite_double(text, number)) {
                return failure(
                    "--disk-surface-optical-depth requires a finite number");
            }
            options.scene.disk.surface_optical_depth = number;
        } else if (name == "--disk-max-crossings") {
            if (!parse_size(text, size)) {
                return failure(
                    "--disk-max-crossings requires an unsigned integer");
            }
            options.scene.disk.max_crossings = size;
        } else if (name == "--exposure") {
            if (!parse_finite_double(text, number)) {
                return failure("--exposure requires a finite number");
            }
            options.scene.disk.display_exposure = number;
        } else {
            return failure("unknown option: " + name);
        }
    }

    if (!has_output) {
        return failure("--output is required");
    }
    if (seen.count("--initial-mino-step") != 0 &&
        seen.count("--initial-step-M") != 0) {
        return failure(
            "--initial-mino-step conflicts with legacy --initial-step-M");
    }
    if (seen.count("--max-mino-step") != 0 &&
        seen.count("--max-step-M") != 0) {
        return failure(
            "--max-mino-step conflicts with legacy --max-step-M");
    }
    const reference::SceneValidation validation =
        reference::validate_reference_scene(options.scene);
    if (!validation) {
        return failure(validation.message);
    }
    return ReferenceRenderParse{
        std::move(options), false, {}};
}

std::string reference_render_usage() {
    return
        "Usage: gargantua-render-reference --output <directory> [options]\n"
        "  --mass-M <value>          Geometrized black-hole mass\n"
        "  --spin <value>            Dimensionless spin, abs(spin) < 1\n"
        "  --observer-r-M <value>    Boyer-Lindquist observer radius\n"
        "  --inclination-deg <value> Observer polar angle in degrees\n"
        "  --fov-y-deg <value>       Vertical field of view in degrees\n"
        "  --width <pixels>          Image width, maximum 4096\n"
        "  --height <pixels>         Image height, maximum 4096\n"
        "  --escape-r-M <value>      Escape event radius\n"
        "  --max-affine-M <value>    Maximum affine integration length\n"
        "  --initial-mino-step <value> Initial Mino-time DOPRI5 step\n"
        "  --max-mino-step <value>     Maximum Mino-time DOPRI5 step\n"
        "  --initial-step-M <value>    Legacy initial Mino-step alias\n"
        "  --max-step-M <value>        Legacy maximum Mino-step alias\n"
        "  --disk-outer-r-M <value>    Thin-disk outer radius\n"
        "  --disk-temperature-scale <value>\n"
        "  --disk-density-scale <value>\n"
        "  --disk-density-power <value>\n"
        "  --disk-specific-scale <value>\n"
        "  --disk-bolometric-scale <value>\n"
        "  --disk-opacity <mode>       opaque or semi-transparent\n"
        "  --disk-surface-optical-depth <value>\n"
        "  --disk-max-crossings <count>\n"
        "  --exposure <value>          Beauty-preview display exposure\n"
        "  --help                      Show this help alone\n"
        "\n"
        "CPU reference uses future-directed photons integrated "
        "observer-to-past.\n"
        "The renderer is a scientific thin-disk model, "
        "not GRMHD or film look.\n";
}

} // namespace gargantua::cli
