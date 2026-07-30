#pragma once

namespace gargantua::reference {

inline constexpr double reference_hamiltonian_error_gate = 1.0e-10;
inline constexpr double reference_stationary_invariant_error_gate =
    1.0e-12;
inline constexpr double reference_carter_relative_error_gate = 1.0e-9;
inline constexpr double reference_capture_margin_fraction = 1.0e-3;
inline constexpr double reference_separated_relative_tolerance = 2.0e-13;
inline constexpr double reference_separated_potential_tolerance = 1.0e-8;
inline constexpr double reference_separated_root_tolerance = 1.0e-10;

} // namespace gargantua::reference
