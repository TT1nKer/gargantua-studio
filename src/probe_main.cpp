#include "solar/relativity/kerr_shadow.h"
#include "solar/relativity/kerr_separated.h"
#include "solar/relativity/local_initialization.h"
#include "solar/relativity/observer.h"
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
              << "}\n";
    return 0;
}
