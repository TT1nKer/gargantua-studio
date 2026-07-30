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
    check("usage names required output option",
          reference_render_usage().find("--output") !=
              std::string::npos);

    std::cout << "\n=== Results: " << passed << " passed, "
              << failed << " failed ===\n";
    return failed == 0 ? 0 : 1;
}
