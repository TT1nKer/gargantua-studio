#include "gargantua/reference/reference_output.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

using namespace gargantua::reference;

namespace {

int passed = 0;
int failed = 0;

void check(const std::string& name, bool condition) {
    std::cout << (condition ? "  PASS: " : "  FAIL: ") << name << '\n';
    condition ? ++passed : ++failed;
}

std::vector<unsigned char> read_bytes(
    const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::vector<unsigned char>(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());
}

std::string read_text(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());
}

std::size_t line_count(const std::string& value) {
    std::size_t count = 0;
    for (const char character : value) {
        if (character == '\n') {
            ++count;
        }
    }
    return count;
}

ReferenceRayResult ray(RayClassification classification) {
    return ReferenceRayResult{
        classification,
        ray_classification_name(classification),
        3.0,
        1.0e-12,
        2.0e-14,
        3.0e-14,
        4.0e-12,
        12,
        1,
    };
}

ReferenceFrame diagnostic_frame() {
    ReferenceScene scene = reference_scene_defaults();
    scene.width = 2;
    scene.height = 2;
    ReferenceFrame frame{
        scene,
        ReferenceTracerInfo{
            "0.2.0-alpha.1",
            "relativity-v3-phase2",
            2.0001,
        },
        {
            ray(RayClassification::CapturedAtBlCutoff),
            ray(RayClassification::Escaped),
            ray(RayClassification::Unconverged),
            ray(RayClassification::ConstraintViolation),
        },
        {},
        FrameStatus::DiagnosticFailed,
    };
    frame.summary.captured = 1;
    frame.summary.escaped = 1;
    frame.summary.unconverged = 1;
    frame.summary.constraint_violations = 1;
    frame.summary.failed = 2;
    frame.summary.max_constraint_error = 1.0e-12;
    frame.summary.max_energy_rel_error = 2.0e-14;
    frame.summary.max_lz_rel_error = 3.0e-14;
    frame.summary.max_carter_rel_error = 4.0e-12;
    frame.summary.max_accepted_steps = 12;
    frame.summary.max_rejected_steps = 1;
    return frame;
}

} // namespace

int main() {
    namespace fs = std::filesystem;
    const auto nonce =
        std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path test_root =
        fs::temp_directory_path() /
        ("gargantua-reference-output-" + std::to_string(nonce));
    fs::create_directory(test_root);

    const ReferenceFrame frame = diagnostic_frame();
    const fs::path output = test_root / "generation";
    const ReferenceOutputResult written =
        write_reference_generation(output, frame);
    check("reference output generation succeeds", bool(written));
    check("final generation directory exists",
          fs::is_directory(output));
    check("part directory is absent after commit",
          !fs::exists(output.string() + ".part"));
    check("PPM exists",
          fs::is_regular_file(output / "classification.ppm"));
    check("CSV exists",
          fs::is_regular_file(output / "rays.csv"));
    check("manifest exists",
          fs::is_regular_file(output / "manifest.json"));

    const std::vector<unsigned char> ppm =
        read_bytes(output / "classification.ppm");
    const std::string ppm_header = "P6\n2 2\n255\n";
    check("PPM uses exact P6 dimensions",
          ppm.size() == ppm_header.size() + 12 &&
              std::equal(
                  ppm_header.begin(),
                  ppm_header.end(),
                  ppm.begin()));
    check("captured pixel is black",
          ppm[ppm_header.size()] == 0 &&
              ppm[ppm_header.size() + 1] == 0 &&
              ppm[ppm_header.size() + 2] == 0);
    check("escaped pixel is neutral gray",
          ppm[ppm_header.size() + 3] == 235 &&
              ppm[ppm_header.size() + 4] == 235 &&
              ppm[ppm_header.size() + 5] == 235);
    check("unconverged pixel is magenta",
          ppm[ppm_header.size() + 6] == 255 &&
              ppm[ppm_header.size() + 7] == 0 &&
              ppm[ppm_header.size() + 8] == 255);
    check("constraint pixel is orange",
          ppm[ppm_header.size() + 9] == 255 &&
              ppm[ppm_header.size() + 10] == 64 &&
              ppm[ppm_header.size() + 11] == 0);

    const std::string csv = read_text(output / "rays.csv");
    check("CSV contains one header and four rays",
          line_count(csv) == 5);
    check("CSV exposes all invariant diagnostics",
          csv.find("max_energy_rel_error,max_lz_rel_error,"
                   "max_carter_rel_error") != std::string::npos);

    const std::string manifest =
        read_text(output / "manifest.json");
    check("manifest records diagnostic failure",
          manifest.find("\"status\":\"diagnostic_failed\"") !=
              std::string::npos);
    check("manifest names checksum algorithm",
          manifest.find("\"checksum_algorithm\":\"fnv1a64\"") !=
              std::string::npos);
    check("manifest records Solar contract",
          manifest.find("\"physics_contract\":"
                        "\"relativity-v3-phase2\"") !=
              std::string::npos);
    check("manifest records missing beauty capability",
          manifest.find("\"beauty_render\"") != std::string::npos);
    check("writer returns nonzero PPM checksum",
          written.ppm_checksum != 0);
    check("writer returns nonzero CSV checksum",
          written.csv_checksum != 0);

    const ReferenceOutputResult duplicate =
        write_reference_generation(output, frame);
    check("existing generation is rejected", !duplicate);
    check("existing PPM remains unchanged",
          read_bytes(output / "classification.ppm") == ppm);

    const fs::path blocked = test_root / "blocked";
    fs::create_directory(blocked.string() + ".part");
    check("existing part generation is rejected",
          !write_reference_generation(blocked, frame));

    ReferenceFrame incomplete = frame;
    incomplete.rays.pop_back();
    const fs::path incomplete_output = test_root / "incomplete";
    check("incomplete frame is rejected",
          !write_reference_generation(
              incomplete_output, incomplete));
    check("rejected frame creates no part directory",
          !fs::exists(incomplete_output.string() + ".part"));

    fs::remove_all(test_root);
    std::cout << "\n=== Results: " << passed << " passed, "
              << failed << " failed ===\n";
    return failed == 0 ? 0 : 1;
}
