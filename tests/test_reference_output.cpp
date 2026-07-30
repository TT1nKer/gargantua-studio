#include "gargantua/reference/reference_output.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
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

std::string manifest_scene_hash(const std::string& manifest) {
    const std::string prefix =
        "\"scene_hash_fnv1a64\":\"";
    const std::size_t start = manifest.find(prefix);
    if (start == std::string::npos) {
        return {};
    }
    return manifest.substr(start + prefix.size(), 16);
}

ReferenceRayResult ray(RayClassification classification) {
    const char* termination_reason = [&] {
        if (classification ==
            RayClassification::CapturedAtBlCutoff) {
            return "interior_cutoff";
        }
        if (classification ==
            RayClassification::DiskSurfaceHit) {
            return "disk_surface_hit";
        }
        return ray_classification_name(classification);
    }();
    return ReferenceRayResult{
        classification,
        termination_reason,
        -1.0,
        3.0,
        2.5,
        0.25,
        1.0e-12,
        2.0e-14,
        3.0e-14,
        4.0e-12,
        12,
        1,
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        0.0,
        0.0,
        0,
    };
}

ReferenceRayResult disk_ray() {
    ReferenceRayResult result =
        ray(RayClassification::DiskSurfaceHit);
    result.disk_radius_M = 8.0;
    result.redshift_g = 1.5;
    result.observed_temperature = 6.0;
    result.observed_specific_intensity = 0.125;
    result.observed_bolometric_intensity = 0.25;
    result.disk_crossings = 1;
    return result;
}

unsigned char expected_beauty_channel(
    double exposure,
    double intensity) {
    const double linear = exposure * intensity;
    const double mapped = linear / (1.0 + linear);
    const double srgb =
        mapped <= 0.0031308
            ? 12.92 * mapped
            : 1.055 * std::pow(mapped, 1.0 / 2.4) - 0.055;
    return static_cast<unsigned char>(
        std::lround(255.0 * std::clamp(srgb, 0.0, 1.0)));
}

ReferenceFrame diagnostic_frame() {
    ReferenceScene scene = reference_scene_defaults();
    scene.width = 3;
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
            disk_ray(),
            ray(RayClassification::Unconverged),
            ray(RayClassification::ConstraintViolation),
            ray(RayClassification::TransferFailure),
        },
        {},
        FrameStatus::DiagnosticFailed,
    };
    frame.summary.captured = 1;
    frame.summary.escaped = 1;
    frame.summary.disk_surface_hits = 1;
    frame.summary.unconverged = 1;
    frame.summary.constraint_violations = 1;
    frame.summary.transfer_failures = 1;
    frame.summary.failed = 3;
    frame.summary.disk_crossings = 1;
    frame.summary.max_constraint_error = 1.0e-12;
    frame.summary.max_energy_rel_error = 2.0e-14;
    frame.summary.max_lz_rel_error = 3.0e-14;
    frame.summary.max_carter_rel_error = 4.0e-12;
    frame.summary.max_redshift_g = 1.5;
    frame.summary.max_observed_specific_intensity = 0.125;
    frame.summary.max_observed_bolometric_intensity = 0.25;
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
    check("beauty PPM exists",
          fs::is_regular_file(output / "beauty.ppm"));
    check("classification PPM exists",
          fs::is_regular_file(output / "classification.ppm"));
    check("CSV exists",
          fs::is_regular_file(output / "rays.csv"));
    check("manifest exists",
          fs::is_regular_file(output / "manifest.json"));

    const std::vector<unsigned char> classification =
        read_bytes(output / "classification.ppm");
    const std::vector<unsigned char> beauty =
        read_bytes(output / "beauty.ppm");
    const std::string ppm_header = "P6\n3 2\n255\n";
    check("classification PPM uses exact P6 dimensions",
          classification.size() == ppm_header.size() + 18 &&
              std::equal(
                  ppm_header.begin(),
                  ppm_header.end(),
                  classification.begin()));
    check("beauty PPM uses exact P6 dimensions",
          beauty.size() == ppm_header.size() + 18 &&
              std::equal(
                  ppm_header.begin(),
                  ppm_header.end(),
                  beauty.begin()));
    check("captured pixel is black",
          classification[ppm_header.size()] == 0 &&
              classification[ppm_header.size() + 1] == 0 &&
              classification[ppm_header.size() + 2] == 0);
    check("escaped pixel is neutral gray",
          classification[ppm_header.size() + 3] == 235 &&
              classification[ppm_header.size() + 4] == 235 &&
              classification[ppm_header.size() + 5] == 235);
    check("disk classification pixel is cyan",
          classification[ppm_header.size() + 6] == 0 &&
              classification[ppm_header.size() + 7] == 200 &&
              classification[ppm_header.size() + 8] == 255);
    check("unconverged pixel is magenta",
          classification[ppm_header.size() + 9] == 255 &&
              classification[ppm_header.size() + 10] == 0 &&
              classification[ppm_header.size() + 11] == 255);
    check("constraint pixel is orange",
          classification[ppm_header.size() + 12] == 255 &&
              classification[ppm_header.size() + 13] == 64 &&
              classification[ppm_header.size() + 14] == 0);
    const unsigned char disk_channel =
        expected_beauty_channel(
            frame.scene.disk.display_exposure, 0.25);
    check("disk beauty pixel uses scientific transform",
          beauty[ppm_header.size() + 6] == disk_channel &&
              beauty[ppm_header.size() + 7] == disk_channel &&
              beauty[ppm_header.size() + 8] == disk_channel);
    check("failed beauty pixel retains diagnostic color",
          beauty[ppm_header.size() + 9] == 255 &&
              beauty[ppm_header.size() + 10] == 0 &&
              beauty[ppm_header.size() + 11] == 255);
    check("transfer failure beauty pixel is red",
          beauty[ppm_header.size() + 15] == 255 &&
              beauty[ppm_header.size() + 16] == 0 &&
              beauty[ppm_header.size() + 17] == 0);

    const std::string csv = read_text(output / "rays.csv");
    check("CSV contains one header and six rays",
          line_count(csv) == 7);
    check("CSV exposes all invariant diagnostics",
          csv.find("max_energy_rel_error,max_lz_rel_error,"
                   "max_carter_rel_error") != std::string::npos);
    check("CSV exposes raw path and disk evidence",
          csv.find(
              "final_affine_M,final_radius_M,min_radius_M,winding") !=
                  std::string::npos &&
              csv.find(
                  "disk_radius_M,redshift_g,observed_temperature,"
                  "observed_specific_intensity,"
                  "observed_bolometric_intensity,disk_crossings") !=
                  std::string::npos);

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
    check("manifest advances to v2",
          manifest.find(
              "\"schema\":\"gargantua.reference-frame.v2\"") !=
              std::string::npos);
    check("manifest records separated observer-to-past physics",
          manifest.find("\"solver\":\"kerr-separated-mino\"") !=
                  std::string::npos &&
              manifest.find(
                  "\"direction\":\"observer-to-past\"") !=
                  std::string::npos);
    check("manifest records all three output files",
          manifest.find("\"beauty.ppm\"") != std::string::npos &&
              manifest.find("\"classification.ppm\"") !=
                  std::string::npos &&
              manifest.find("\"rays.csv\"") != std::string::npos);
    check("implemented scientific capabilities are not missing",
          manifest.find("\"beauty_render\"") == std::string::npos &&
              manifest.find("\"disk_intersections\"") ==
                  std::string::npos &&
              manifest.find(
                  "\"redshift_and_radiative_transfer\"") ==
                  std::string::npos &&
              manifest.find("\"separated_mino_solver\"") ==
                  std::string::npos);
    check("manifest records BL polar-axis limitation",
          manifest.find("\"bl_polar_axis_crossing\"") !=
              std::string::npos);
    check("writer returns nonzero beauty checksum",
          written.beauty_ppm_checksum != 0);
    check("writer returns nonzero classification checksum",
          written.classification_ppm_checksum != 0);
    check("writer returns nonzero CSV checksum",
          written.csv_checksum != 0);

    ReferenceFrame extreme_display = frame;
    extreme_display.scene.disk.display_exposure =
        std::numeric_limits<double>::max();
    extreme_display.rays[2].observed_bolometric_intensity =
        std::numeric_limits<double>::max();
    extreme_display.summary.max_observed_bolometric_intensity =
        std::numeric_limits<double>::max();
    const fs::path extreme_output =
        test_root / "extreme-display";
    const ReferenceOutputResult extreme_written =
        write_reference_generation(
            extreme_output, extreme_display);
    check("finite extreme display values serialize",
          bool(extreme_written));
    const std::vector<unsigned char> extreme_beauty =
        read_bytes(extreme_output / "beauty.ppm");
    check("overflowing positive display product saturates white",
          extreme_beauty[ppm_header.size() + 6] == 255 &&
              extreme_beauty[ppm_header.size() + 7] == 255 &&
              extreme_beauty[ppm_header.size() + 8] == 255);
    check("display changes alter the canonical scene hash",
          manifest_scene_hash(read_text(
              extreme_output / "manifest.json")) !=
              manifest_scene_hash(manifest));

    const ReferenceOutputResult duplicate =
        write_reference_generation(output, frame);
    check("existing generation is rejected", !duplicate);
    check("existing PPM remains unchanged",
          read_bytes(output / "classification.ppm") ==
              classification);

    const fs::path blocked = test_root / "blocked";
    fs::create_directory(blocked.string() + ".part");
    {
        std::ofstream marker(
            blocked.string() + ".part/owner.txt",
            std::ios::binary);
        marker << "keep";
    }
    check("existing part generation is rejected",
          !write_reference_generation(blocked, frame));
    check("existing part generation remains unchanged",
          read_text(
              blocked.string() + ".part/owner.txt") == "keep");

    ReferenceFrame incomplete = frame;
    incomplete.rays.pop_back();
    const fs::path incomplete_output = test_root / "incomplete";
    check("incomplete frame is rejected",
          !write_reference_generation(
              incomplete_output, incomplete));
    check("rejected frame creates no part directory",
          !fs::exists(incomplete_output.string() + ".part"));

    ReferenceFrame invalid_disk_evidence = frame;
    invalid_disk_evidence.rays[2].redshift_g =
        std::numeric_limits<double>::quiet_NaN();
    const fs::path invalid_disk_output =
        test_root / "invalid-disk-evidence";
    check("invalid disk evidence is rejected",
          !write_reference_generation(
              invalid_disk_output, invalid_disk_evidence));
    check("invalid disk evidence creates no output or part",
          !fs::exists(invalid_disk_output) &&
              !fs::exists(
                  invalid_disk_output.string() + ".part"));

    ReferenceFrame mismatched_summary = frame;
    mismatched_summary.rays[0] =
        ray(RayClassification::Escaped);
    check(
        "summary that disagrees with rays is rejected",
        !write_reference_generation(
            test_root / "mismatched-summary",
            mismatched_summary));

    ReferenceFrame unknown_classification = frame;
    unknown_classification.rays[0].classification =
        static_cast<RayClassification>(99);
    check(
        "unknown ray classification is rejected",
        !write_reference_generation(
            test_root / "unknown-classification",
            unknown_classification));

    ReferenceFrame unknown_status = frame;
    unknown_status.status = static_cast<FrameStatus>(99);
    check(
        "unknown frame status is rejected",
        !write_reference_generation(
            test_root / "unknown-status",
            unknown_status));

    ReferenceFrame invalid_tracer = frame;
    invalid_tracer.tracer.capture_radius_M =
        std::numeric_limits<double>::quiet_NaN();
    check(
        "non-finite tracer metadata is rejected",
        !write_reference_generation(
            test_root / "invalid-tracer",
            invalid_tracer));

    ReferenceFrame invalid_accepted_diagnostics = frame;
    invalid_accepted_diagnostics.rays[0].max_constraint_error =
        std::numeric_limits<double>::quiet_NaN();
    check(
        "non-finite accepted-ray diagnostics are rejected",
        !write_reference_generation(
            test_root / "invalid-accepted-diagnostics",
            invalid_accepted_diagnostics));

    ReferenceFrame contradictory_termination = frame;
    contradictory_termination.rays[0].termination_reason =
        "escaped";
    check(
        "contradictory successful termination is rejected",
        !write_reference_generation(
            test_root / "contradictory-termination",
            contradictory_termination));

    ReferenceFrame accepted_above_gate = frame;
    accepted_above_gate.rays[0].max_constraint_error =
        2.0e-10;
    accepted_above_gate.summary.max_constraint_error =
        2.0e-10;
    check(
        "accepted ray above invariant gate is rejected",
        !write_reference_generation(
            test_root / "accepted-above-gate",
            accepted_above_gate));

    fs::remove_all(test_root);
    std::cout << "\n=== Results: " << passed << " passed, "
              << failed << " failed ===\n";
    return failed == 0 ? 0 : 1;
}
