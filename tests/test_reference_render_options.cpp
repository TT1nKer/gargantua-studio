#include "reference_render_options.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace gargantua::cli;

namespace {

int passed = 0;
int failed = 0;

void check(const std::string& name, bool condition) {
    std::cout << (condition ? "  PASS: " : "  FAIL: ") << name << '\n';
    condition ? ++passed : ++failed;
}

ReferenceRenderParse parse(
    std::initializer_list<const char*> values) {
    std::vector<std::string> arguments;
    for (const char* value : values) {
        arguments.emplace_back(value);
    }
    return parse_reference_render_options(arguments);
}

} // namespace

int main() {
    const ReferenceRenderParse help =
        parse({"gargantua-render-reference", "--help"});
    check("help is accepted without output", bool(help));
    check("help is explicit", help.show_help);
    check("help does not create render options",
          !help.options.has_value());

    check("missing output is rejected",
          !parse({"gargantua-render-reference"}));
    check("unknown option is rejected",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--unknown", "1"}));
    check("missing value is rejected",
          !parse({
              "gargantua-render-reference",
              "--output"}));
    check("repeated option is rejected",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--width", "3",
              "--width", "4"}));
    check("NaN is rejected",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--spin", "nan"}));
    check("infinite numeric value is rejected",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--spin", "inf"}));
    check("numeric value with surrounding whitespace is rejected",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--spin", " 0.5"}));
    check("trailing numeric text is rejected",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--spin", "0.5x"}));
    check("negative size is rejected",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--width", "-1"}));
    check("empty output path is rejected",
          !parse({
              "gargantua-render-reference",
              "--output", ""}));
    check("zero size is rejected by scene contract",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--height", "0"}));
    check("help mixed with render options is rejected",
          !parse({
              "gargantua-render-reference",
              "--help",
              "--output", "frame"}));
    check("unknown disk opacity is rejected",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--disk-opacity", "transparent"}));
    check("repeated canonical Mino option is rejected",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--initial-mino-step", "0.01",
              "--initial-mino-step", "0.02"}));
    check("canonical and legacy initial step aliases conflict",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--initial-mino-step", "0.02",
              "--initial-step-M", "0.02"}));
    check("canonical and legacy maximum step aliases conflict",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--max-mino-step", "0.25",
              "--max-step-M", "0.25"}));
    check("zero disk crossing count is rejected",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--disk-max-crossings", "0"}));
    check("negative semi-transparent depth is rejected",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--disk-opacity", "semi-transparent",
              "--disk-surface-optical-depth", "-0.1"}));
    check("infinite semi-transparent depth is rejected",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--disk-opacity", "semi-transparent",
              "--disk-surface-optical-depth", "inf"}));
    check("non-positive exposure is rejected",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--exposure", "0"}));
    check("disk outer radius inside the horizon is rejected",
          !parse({
              "gargantua-render-reference",
              "--output", "frame",
              "--mass-M", "1",
              "--spin", "0",
              "--disk-outer-r-M", "1.5"}));

    const ReferenceRenderParse valid = parse({
        "gargantua-render-reference",
        "--output", "frame",
        "--mass-M", "2",
        "--spin", "-0.25",
        "--observer-r-M", "40",
        "--inclination-deg", "60",
        "--fov-y-deg", "30",
        "--width", "9",
        "--height", "7",
        "--escape-r-M", "80",
        "--max-affine-M", "250",
        "--initial-step-M", "0.01",
        "--max-step-M", "0.2",
    });
    check("valid full override is accepted", bool(valid));
    if (valid.options) {
        check("output path is retained",
              valid.options->output_directory == "frame");
        check("mass override is retained",
              valid.options->scene.mass_M == 2.0);
        check("signed spin override is retained",
              valid.options->scene.spin_chi == -0.25);
        check("width override is retained",
              valid.options->scene.width == 9);
        check("height override is retained",
              valid.options->scene.height == 7);
        check("inclination is converted to radians",
              std::fabs(
                  valid.options->scene.inclination_radians -
                  1.0471975511965976) < 1.0e-15);
        check("field of view is converted to radians",
              std::fabs(
                  valid.options->scene.vertical_fov_radians -
                  0.5235987755982988) < 1.0e-15);
    }

    const ReferenceRenderParse scientific_disk = parse({
        "gargantua-render-reference",
        "--output", "scientific-frame",
        "--mass-M", "2",
        "--observer-r-M", "40",
        "--escape-r-M", "80",
        "--initial-mino-step", "0.015",
        "--max-mino-step", "0.3",
        "--disk-outer-r-M", "25",
        "--disk-temperature-scale", "2.5",
        "--disk-density-scale", "1.25",
        "--disk-density-power", "0.6",
        "--disk-specific-scale", "3",
        "--disk-bolometric-scale", "4",
        "--disk-opacity", "semi-transparent",
        "--disk-surface-optical-depth", "0.4",
        "--disk-max-crossings", "12",
        "--exposure", "2",
    });
    check("scientific disk overrides are accepted",
          bool(scientific_disk));
    if (scientific_disk.options) {
        const auto& scene = scientific_disk.options->scene;
        check("canonical Mino steps are retained",
              scene.initial_step_M == 0.015 &&
                  scene.max_step_M == 0.3);
        check("disk geometry and material are retained",
              scene.disk.outer_radius_M == 25.0 &&
                  scene.disk.temperature_scale == 2.5 &&
                  scene.disk.density_scale == 1.25 &&
                  scene.disk.density_power == 0.6);
        check("disk emission scales are retained",
              scene.disk.specific_intensity_scale == 3.0 &&
                  scene.disk.bolometric_intensity_scale == 4.0);
        check("semi-transparent transfer controls are retained",
              scene.disk.opacity ==
                      gargantua::reference::
                          ReferenceDiskOpacity::SemiTransparent &&
                  scene.disk.surface_optical_depth == 0.4 &&
                  scene.disk.max_crossings == 12);
        check("display exposure is retained",
              scene.disk.display_exposure == 2.0);
    }

    const ReferenceRenderParse opaque_disk = parse({
        "gargantua-render-reference",
        "--output", "opaque-frame",
        "--disk-opacity", "opaque",
    });
    check("opaque disk mode is accepted", bool(opaque_disk));
    if (opaque_disk.options) {
        check("opaque disk mode is retained",
              opaque_disk.options->scene.disk.opacity ==
                  gargantua::reference::
                      ReferenceDiskOpacity::Opaque);
    }
    check("usage names required output option",
          reference_render_usage().find("--output") !=
              std::string::npos);
    check("usage states integration direction",
          reference_render_usage().find(
              "observer-to-past") != std::string::npos);
    check("usage states scientific model boundary",
          reference_render_usage().find(
              "not GRMHD or film look") != std::string::npos);

    std::cout << "\n=== Results: " << passed << " passed, "
              << failed << " failed ===\n";
    return failed == 0 ? 0 : 1;
}
