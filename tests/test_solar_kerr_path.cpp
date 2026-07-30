#include "solar_kerr_path.h"

#include "gargantua/reference/perspective_camera.h"
#include "gargantua/reference/reference_ray_tracer.h"
#include "solar/relativity/kerr_bl_metric.h"
#include "solar/relativity/observer.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

using namespace gargantua::reference;
using namespace gargantua::reference::detail;
using namespace solar::relativity;

namespace {

int passed = 0;
int failed = 0;

void check(const std::string& name, bool condition) {
    std::cout << (condition ? "  PASS: " : "  FAIL: ")
              << name << '\n';
    condition ? ++passed : ++failed;
}

ObserverFrame make_observer(
    const KerrBoyerLindquistMetric& metric,
    const ReferenceScene& scene) {
    const ObserverResult observer = make_zamo_observer(
        metric,
        Contravariant4{Vec4{{
            0.0,
            scene.observer_radius_M,
            scene.inclination_radians,
            0.0,
        }}});
    if (!observer) {
        throw std::runtime_error(observer.message);
    }
    return *observer.frame;
}

struct ScanSummary {
    std::size_t captured = 0;
    std::size_t escaped = 0;
    std::size_t disk_hits = 0;
    std::size_t failed = 0;
    std::size_t ignored_crossings = 0;
    std::size_t max_crossings = 0;
    bool all_advanced_to_past = true;
    bool all_disk_samples_finite = true;
    std::map<std::string, std::size_t> reasons;
};

ScanSummary scan_scene(const ReferenceScene& scene) {
    const KerrBoyerLindquistMetric metric(
        scene.mass_M, scene.spin_chi);
    const ObserverFrame observer =
        make_observer(metric, scene);
    ScanSummary summary;
    for (std::size_t y = 0; y < scene.height; ++y) {
        for (std::size_t x = 0; x < scene.width; ++x) {
            const SolarKerrPathTrace traced =
                trace_solar_kerr_path(
                    metric,
                    observer,
                    scene,
                    perspective_camera_ray(scene, x, y));
            const ReferenceRayResult& ray = traced.ray;
            ++summary.reasons[ray.termination_reason];
            summary.ignored_crossings +=
                traced.ignored_surface_crossings;
            summary.max_crossings = std::max(
                summary.max_crossings,
                ray.disk_crossings);
            if (ray.classification ==
                RayClassification::CapturedAtBlCutoff) {
                ++summary.captured;
            } else if (
                ray.classification ==
                RayClassification::Escaped) {
                ++summary.escaped;
            } else if (
                ray.classification ==
                RayClassification::DiskSurfaceHit) {
                ++summary.disk_hits;
            }
            if (is_failed_classification(
                    ray.classification)) {
                ++summary.failed;
            }
            if (std::isfinite(ray.final_affine_M)) {
                summary.all_advanced_to_past &=
                    ray.final_affine_M < 0.0;
            }
            if (ray.disk_crossings > 0) {
                summary.all_disk_samples_finite &=
                    std::isfinite(ray.disk_radius_M) &&
                    std::isfinite(ray.redshift_g) &&
                    ray.redshift_g > 0.0 &&
                    std::isfinite(
                        ray.observed_bolometric_intensity) &&
                    ray.observed_bolometric_intensity >= 0.0;
            }
        }
    }
    return summary;
}

} // namespace

int main() {
    ReferenceScene scene = reference_scene_defaults();
    scene.width = 64;
    scene.height = 36;

    const KerrBoyerLindquistMetric metric(
        scene.mass_M, scene.spin_chi);
    const ObserverFrame observer =
        make_observer(metric, scene);
    const SolarKerrPathTrace center =
        trace_solar_kerr_path(
            metric,
            observer,
            scene,
            perspective_camera_ray(scene, 31, 17));
    check("center-area ray advances observer-to-past",
          center.ray.final_affine_M < 0.0);
    check("center-area ray has finite minimum radius",
          std::isfinite(center.ray.min_radius_M) &&
              center.ray.min_radius_M > 0.0);
    check("center-area ray has finite winding",
          std::isfinite(center.ray.winding));
    check("center-area ray is never fabricated escape",
          center.ray.classification !=
              RayClassification::Escaped);

    const ScanSummary opaque = scan_scene(scene);
    std::cout << "  opaque captured=" << opaque.captured
              << " escaped=" << opaque.escaped
              << " disk=" << opaque.disk_hits
              << " failed=" << opaque.failed
              << " ignored=" << opaque.ignored_crossings
              << " max_crossings=" << opaque.max_crossings
              << '\n';
    for (const auto& reason : opaque.reasons) {
        std::cout << "    " << reason.first << '='
                  << reason.second << '\n';
    }
    check("opaque scan contains capture",
          opaque.captured > 0);
    check("opaque scan contains escape",
          opaque.escaped > 0);
    check("opaque scan contains disk hits",
          opaque.disk_hits > 0);
    check("opaque scan continues outside-support crossings",
          opaque.ignored_crossings > 0);
    check("opaque scan advances every finite ray to past",
          opaque.all_advanced_to_past);
    check("opaque disk samples are finite",
          opaque.all_disk_samples_finite);
    check("opaque scan has no failed rays",
          opaque.failed == 0);

    ReferenceScene translucent = scene;
    translucent.disk.opacity =
        ReferenceDiskOpacity::SemiTransparent;
    translucent.disk.surface_optical_depth = 0.5;
    const ScanSummary semi = scan_scene(translucent);
    std::cout << "  semi captured=" << semi.captured
              << " escaped=" << semi.escaped
              << " disk=" << semi.disk_hits
              << " failed=" << semi.failed
              << " ignored=" << semi.ignored_crossings
              << " max_crossings=" << semi.max_crossings
              << '\n';
    for (const auto& reason : semi.reasons) {
        std::cout << "    " << reason.first << '='
                  << reason.second << '\n';
    }
    check("semi-transparent scan composes repeated crossings",
          semi.max_crossings >= 2);
    check("semi-transparent disk samples are finite",
          semi.all_disk_samples_finite);
    check("semi-transparent scan has no failed rays",
          semi.failed == 0);

    ReferenceScene inside_isco = reference_scene_defaults();
    inside_isco.spin_chi = 0.0;
    inside_isco.disk.outer_radius_M = 3.0;
    const ReferenceTracerBuild rejected =
        make_solar_kerr_ray_tracer(inside_isco);
    check("Solar rejects disk outer edge inside ISCO",
          !rejected);

    std::cout << "\n=== Results: " << passed << " passed, "
              << failed << " failed ===\n";
    return failed == 0 ? 0 : 1;
}
