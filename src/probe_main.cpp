#include "solar/relativity/fluid_model.h"
#include "solar/relativity/kerr_shadow.h"
#include "solar/relativity/kerr_separated.h"
#include "solar/relativity/local_initialization.h"
#include "solar/relativity/observer.h"
#include "solar/relativity/radiative_transfer.h"
#include "solar/relativity/thin_disk.h"
#include "solar/version.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string_view>

#ifndef GARGANTUA_REQUIRED_SOLAR_VERSION
#error "Gargantua requires the locked Solar version at compile time"
#endif

#ifndef GARGANTUA_REQUIRED_SOLAR_CONTRACT
#error "Gargantua requires the locked Solar physics contract at compile time"
#endif

int main() {
    constexpr std::string_view required_version{
        GARGANTUA_REQUIRED_SOLAR_VERSION};
    constexpr std::string_view required_contract{
        GARGANTUA_REQUIRED_SOLAR_CONTRACT};
    constexpr double half_pi = 1.5707963267948966;
    constexpr std::size_t samples_per_branch = 65;
    constexpr double expected_left_edge = -4.096266658713869;
    constexpr double expected_right_edge = 6.138155724715452;
    constexpr double edge_tolerance = 1.0e-13;

    if (solar::version != required_version ||
        solar::physics_contract != required_contract) {
        std::cerr << "Solar package contract mismatch\n";
        return 1;
    }

    const solar::relativity::KerrBoyerLindquistMetric metric(1.0, 0.5);
    const auto curve = solar::relativity::bardeen_shadow_curve(
        metric, half_pi, samples_per_branch);
    if (curve.size() != 2 * samples_per_branch - 2) {
        std::cerr << "Solar shadow sample count mismatch\n";
        return 2;
    }

    const auto edges = std::minmax_element(
        curve.begin(),
        curve.end(),
        [](const auto& left, const auto& right) {
            return left.alpha < right.alpha;
        });
    const double left_edge = edges.first->alpha;
    const double right_edge = edges.second->alpha;
    if (!std::isfinite(left_edge) ||
        !std::isfinite(right_edge) ||
        std::abs(left_edge - expected_left_edge) > edge_tolerance ||
        std::abs(right_edge - expected_right_edge) > edge_tolerance) {
        std::cerr << "Solar shadow edge mismatch\n";
        return 3;
    }

    const solar::relativity::Contravariant4 observer_position{
        solar::relativity::Vec4{{
            0.0,
            20.0,
            half_pi,
            0.0,
        }}};
    const auto observer =
        solar::relativity::make_zamo_observer(
            metric, observer_position);
    if (!observer) {
        std::cerr << "Solar ZAMO construction failed\n";
        return 4;
    }
    const auto photon =
        solar::relativity::initialize_local_photon(
            metric,
            *observer.frame,
            solar::relativity::Vec3{{-1.0, 0.0, 0.0}});
    if (!photon) {
        std::cerr << "Solar photon initialization failed\n";
        return 5;
    }

    const auto separated_config =
        solar::relativity::KerrSeparatedConfig::cpu_reference(
            solar::relativity::GeodesicKind::Null,
            1.0,
            1.0e-5,
            1.0e-4,
            0.1);
    const auto separated =
        solar::relativity::KerrSeparatedIntegrator(metric)
            .integrate(*photon.state, separated_config);
    if (separated.diagnostics.reason !=
            solar::relativity::TerminationReason::MaxAffine ||
        separated.diagnostics.accepted_steps == 0 ||
        !std::isfinite(separated.diagnostics.min_radius_M) ||
        !std::isfinite(separated.diagnostics.winding) ||
        separated.diagnostics.max_constraint_error >= 1.0e-10) {
        std::cerr << "Solar separated Kerr integration failed\n";
        return 6;
    }

    const auto transfer =
        solar::relativity::advance_backward_transfer(
            {},
            solar::relativity::TransferCoefficients{2.0, 0.5},
            3.0);
    constexpr double expected_transfer_intensity =
        3.1074793594062806;
    constexpr double expected_transfer_transmission =
        0.22313016014842982;

    const solar::relativity::Contravariant4 disk_point{
        solar::relativity::Vec4{{
            0.0, 8.0, half_pi, 0.2}}};
    const solar::relativity::AnalyticCircularDiskConfig
        disk_config{
            1.0,
            0.5,
            solar::relativity::OrbitSense::Prograde,
            6.0,
            20.0,
            1.0,
            16.407349347422414,
            0.0,
            1.0e-8,
        };
    const solar::relativity::AnalyticCircularDiskFluid
        disk(disk_config);
    const auto disk_sample = disk.sample(metric, disk_point);
    const solar::relativity::AnalyticOpticallyThinTorus torus(
        solar::relativity::AnalyticOpticallyThinTorusConfig{
            1.0,
            0.5,
            solar::relativity::OrbitSense::Prograde,
            8.0,
            2.0,
            0.2,
            3.0,
            4.0,
            0.0,
            1.0e-4,
        });
    const auto torus_sample = torus.sample(metric, disk_point);
    const auto disk_emitter =
        solar::relativity::make_equatorial_circular_observer(
            metric,
            disk_point,
            solar::relativity::OrbitSense::Prograde);
    if (!disk_emitter) {
        std::cerr << "Solar disk emitter construction failed\n";
        return 7;
    }
    const auto disk_photon =
        solar::relativity::initialize_local_photon(
            metric,
            *disk_emitter.frame,
            solar::relativity::Vec3{{0.0, -1.0, 0.0}});
    if (!disk_photon) {
        std::cerr << "Solar disk photon construction failed\n";
        return 8;
    }
    solar::relativity::ThinDiskCrossingRecorder surface(
        solar::relativity::ThinDiskRecorderConfig{
            solar::relativity::DiskOpacityMode::Opaque, 8},
        disk,
        solar::relativity::ThinDiskSurfaceEmission(
            0.75, 10.0 / 4096.0, 0.7));
    const auto surface_result =
        surface.record(metric, *disk_photon.state, 1.0);

    const double transfer_error = std::max(
        std::abs(
            transfer.state.invariant_intensity -
            expected_transfer_intensity),
        std::abs(
            transfer.state.transmission -
            expected_transfer_transmission));
    if (!transfer ||
        !std::isfinite(transfer.state.invariant_intensity) ||
        !std::isfinite(transfer.state.transmission) ||
        transfer_error >= 5.0e-14 ||
        !disk_sample.valid ||
        !torus_sample.valid ||
        !surface_result ||
        !surface_result.recorded ||
        surface.crossings().size() != 1 ||
        !std::isfinite(disk_sample.temperature) ||
        !std::isfinite(torus_sample.density) ||
        !std::isfinite(
            surface.observed().specific_intensity)) {
        std::cerr << "Solar Phase 5 transfer API failed\n";
        return 9;
    }

    std::cout << std::setprecision(17)
              << "{\"engine\":\"solar\""
              << ",\"solar_version\":\"" << solar::version
              << "\",\"physics_contract\":\"" << solar::physics_contract
              << "\",\"metric\":\"kerr-bl\""
              << ",\"mass\":1,\"spin\":0.5"
              << ",\"samples\":" << curve.size()
              << ",\"left\":" << left_edge
              << ",\"right\":" << right_edge
              << ",\"separated_steps\":"
              << separated.diagnostics.accepted_steps
              << ",\"separated_constraint\":"
              << separated.diagnostics.max_constraint_error
              << ",\"separated_min_radius_M\":"
              << separated.diagnostics.min_radius_M
              << ",\"separated_winding\":"
              << separated.diagnostics.winding
              << ",\"transfer_intensity\":"
              << transfer.state.invariant_intensity
              << ",\"transfer_transmission\":"
              << transfer.state.transmission
              << ",\"disk_temperature\":"
              << disk_sample.temperature
              << ",\"torus_density\":"
              << torus_sample.density
              << ",\"surface_specific_intensity\":"
              << surface.observed().specific_intensity
              << ",\"surface_crossings\":"
              << surface.crossings().size()
              << "}\n";
    return 0;
}
