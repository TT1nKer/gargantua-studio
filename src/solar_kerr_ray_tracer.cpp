#include "gargantua/reference/reference_ray_tracer.h"

#include "gargantua/reference/reference_numerics.h"
#include "reference_ray_evidence.h"
#include "solar_kerr_path.h"
#include "solar/relativity/kerr_bl_metric.h"
#include "solar/relativity/observer.h"
#include "solar/version.h"

#include <stdexcept>

namespace gargantua::reference {
namespace {

using namespace solar::relativity;

ObserverFrame make_render_observer(
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
        throw std::invalid_argument(
            "cannot construct render ZAMO: " + observer.message);
    }
    return *observer.frame;
}

class SolarKerrRayTracer final : public ReferenceRayTracer {
public:
    explicit SolarKerrRayTracer(const ReferenceScene& scene)
        : scene_(scene),
          metric_(scene.mass_M, scene.spin_chi),
          observer_(make_render_observer(metric_, scene)),
          info_{
              std::string(solar::version),
              std::string(solar::physics_contract),
              metric_.outer_horizon_radius() +
                  reference_capture_margin_fraction *
                      scene.mass_M} {
        detail::validate_solar_kerr_path_scene(scene);
    }

    const ReferenceTracerInfo& info() const noexcept override {
        return info_;
    }

    ReferenceRayResult trace(
        const CameraRay& ray) const override {
        try {
            return detail::trace_solar_kerr_path(
                       metric_, observer_, scene_, ray)
                .ray;
        } catch (const std::exception&) {
            return detail::unavailable_reference_ray(
                RayClassification::Unconverged,
                "integration_exception");
        }
    }

private:
    ReferenceScene scene_;
    KerrBoyerLindquistMetric metric_;
    ObserverFrame observer_;
    ReferenceTracerInfo info_;
};

} // namespace

ReferenceTracerBuild make_solar_kerr_ray_tracer(
    const ReferenceScene& scene) {
    const SceneValidation validation =
        validate_reference_scene(scene);
    if (!validation) {
        return ReferenceTracerBuild{nullptr, validation.message};
    }
    try {
        return ReferenceTracerBuild{
            std::make_unique<SolarKerrRayTracer>(scene),
            {},
        };
    } catch (const std::exception& error) {
        return ReferenceTracerBuild{nullptr, error.what()};
    }
}

} // namespace gargantua::reference
