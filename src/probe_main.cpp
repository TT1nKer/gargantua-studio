#include "solar/relativity/kerr_shadow.h"
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

    std::cout << std::setprecision(17)
              << "{\"engine\":\"solar\""
              << ",\"solar_version\":\"" << solar::version
              << "\",\"physics_contract\":\"" << solar::physics_contract
              << "\",\"metric\":\"kerr-bl\""
              << ",\"mass\":1,\"spin\":0.5"
              << ",\"samples\":" << curve.size()
              << ",\"left\":" << left_edge
              << ",\"right\":" << right_edge << "}\n";
    return 0;
}
