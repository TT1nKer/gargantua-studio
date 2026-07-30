#include "gargantua/reference/reference_renderer.h"
#include "gargantua/reference/reference_scene.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace gargantua::reference;

namespace {

int passed = 0;
int failed = 0;

void check(const std::string& name, bool condition) {
    std::cout << (condition ? "  PASS: " : "  FAIL: ") << name << '\n';
    condition ? ++passed : ++failed;
}

struct Pixel {
    std::size_t x;
    std::size_t y;
};

class RecordingTracer final : public ReferenceRayTracer {
public:
    const ReferenceTracerInfo& info() const noexcept override {
        return info_;
    }

    ReferenceRayResult trace(
        const CameraRay& ray) const override {
        seen.push_back(Pixel{ray.pixel_x, ray.pixel_y});
        if (throw_at_last && ray.pixel_x == 2 && ray.pixel_y == 1) {
            throw std::runtime_error("scripted tracer failure");
        }

        RayClassification classification =
            ray.pixel_x == 0
                ? RayClassification::CapturedAtBlCutoff
                : RayClassification::Escaped;
        if (ray.pixel_x == 2 && ray.pixel_y == 1) {
            classification =
                RayClassification::ConstraintViolation;
        }
        return ReferenceRayResult{
            classification,
            ray_classification_name(classification),
            2.0 + static_cast<double>(ray.pixel_x),
            1.0e-12 * static_cast<double>(ray.pixel_x + 1),
            3.0e-14 * static_cast<double>(ray.pixel_x + 1),
            4.0e-14 * static_cast<double>(ray.pixel_y + 1),
            2.0e-12 * static_cast<double>(ray.pixel_y + 1),
            10 + ray.pixel_x + 3 * ray.pixel_y,
            ray.pixel_y,
        };
    }

    mutable std::vector<Pixel> seen;
    bool throw_at_last = false;

private:
    ReferenceTracerInfo info_{
        "test-solar", "test-contract", 2.1};
};

} // namespace

int main() {
    ReferenceScene scene = reference_scene_defaults();
    scene.width = 3;
    scene.height = 2;

    RecordingTracer tracer;
    const ReferenceRenderResult rendered =
        render_reference_frame(scene, tracer);
    check("reference frame renders", bool(rendered));
    if (!rendered) {
        std::cerr << rendered.message << '\n';
        return 1;
    }

    check("one result is retained per pixel",
          rendered.frame->rays.size() == 6);
    check("tracer sees one call per pixel",
          tracer.seen.size() == 6);
    check("first pixel is row-major origin",
          tracer.seen[0].x == 0 && tracer.seen[0].y == 0);
    check("row boundary advances y after width pixels",
          tracer.seen[3].x == 0 && tracer.seen[3].y == 1);
    check("last pixel is bottom right",
          tracer.seen[5].x == 2 && tracer.seen[5].y == 1);
    check("captured count is exhaustive",
          rendered.frame->summary.captured == 2);
    check("escaped count excludes failed pixel",
          rendered.frame->summary.escaped == 3);
    check("constraint count is retained",
          rendered.frame->summary.constraint_violations == 1);
    check("aggregate failed count is retained",
          rendered.frame->summary.failed == 1);
    check("failed frame is marked diagnostic failed",
          rendered.frame->status ==
              FrameStatus::DiagnosticFailed);
    check("maximum accepted steps is retained",
          rendered.frame->summary.max_accepted_steps == 15);
    check("maximum rejected steps is retained",
          rendered.frame->summary.max_rejected_steps == 1);
    check("maximum constraint error is retained",
          std::fabs(
              rendered.frame->summary.max_constraint_error -
              3.0e-12) < 1.0e-27);
    check("maximum energy error is retained",
          std::fabs(
              rendered.frame->summary.max_energy_rel_error -
              9.0e-14) < 1.0e-28);
    check("maximum Lz error is retained",
          std::fabs(
              rendered.frame->summary.max_lz_rel_error -
              8.0e-14) < 1.0e-28);
    check("maximum Carter error is retained",
          std::fabs(
              rendered.frame->summary.max_carter_rel_error -
              4.0e-12) < 1.0e-27);
    check("tracer contract is copied into frame",
          rendered.frame->tracer.physics_contract ==
              "test-contract");

    RecordingTracer throwing;
    throwing.throw_at_last = true;
    const ReferenceRenderResult interrupted =
        render_reference_frame(scene, throwing);
    check("throwing tracer produces structural failure",
          !interrupted);
    check("structural failure exposes no partial frame",
          !interrupted.frame.has_value());

    std::cout << "\n=== Results: " << passed << " passed, "
              << failed << " failed ===\n";
    return failed == 0 ? 0 : 1;
}
