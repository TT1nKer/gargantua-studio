# Validation: Gargantua consumes Solar Phase 3

## Claim

Gargantua Studio can compile and run as an independent CMake project while
calling the locked Solar Phase 3 public separated-Kerr API.

## Model boundary

The probe performs two independent public-library calls in a Kerr spacetime
using Boyer-Lindquist coordinates and geometrized units:

- the Phase 2 Bardeen critical curve at `M=1`, `chi=0.5`, equatorial
  inclination, and 65 requested samples per branch;
- a Phase 3 short null-geodesic integration in Mino time from a ZAMO at
  `r=20M`, including minimum-radius, winding, and constraint diagnostics.

This is CPU double-precision dependency and contract evidence. The production
reference raster remains on Solar's generic Hamiltonian path for its existing
image-validation baseline.

## Solar dependency

- Git commit: `82acb4e6c60e1fa18447cf37370278d5ac9e82f8`
- Package version: `0.2.0-alpha.1`
- Physics contract: `relativity-v3-phase2`
- Public target: `Solar::Relativity`

## Command

```sh
cmake -S . -B /tmp/gargantua-final-build \
  -DGARGANTUA_SOLAR_SOURCE_DIR=/path/to/solar-at-82acb4e6c60e1fa18447cf37370278d5ac9e82f8 \
  -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/gargantua-final-build --parallel
ctest --test-dir /tmp/gargantua-final-build --output-on-failure
/tmp/gargantua-final-build/gargantua-probe
```

## Inputs

- Metric: Kerr, Boyer-Lindquist chart
- Mass: `1`
- Spin: `0.5`
- Inclination: `1.5707963267948966`
- Samples per branch: `65`
- Separated initial/max Mino step: `1e-5 / 1e-4`
- Separated maximum affine parameter: `0.1M`

## Expected

- Closed-curve sample count: `128`
- Left edge: `-4.096266658713869`
- Right edge: `6.138155724715452`
- Absolute edge tolerance: `1e-13`
- Separated termination: `MaxAffine`
- At least one separated step and finite minimum radius/winding
- Separated normalized Hamiltonian constraint: `< 1e-10`
- CTest result: all eight Gargantua tests pass

## Actual

Both a clean local-source build and a fresh repository-only `FetchContent`
build passed `8/8` CTest tests. The fresh probe output was:

```json
{"engine":"solar","solar_version":"0.2.0-alpha.1","physics_contract":"relativity-v3-phase2","metric":"kerr-bl","mass":1,"spin":0.5,"samples":128,"left":-4.0962666587138692,"right":6.1381557247154532,"separated_steps":4,"separated_constraint":1.2394276503472693e-16,"separated_min_radius_M":19.905098572994902,"separated_winding":2.1111653880200616e-06}
```

The build emitted no warnings.

## Error

- Left-edge absolute error: `0`
- Right-edge absolute error: `8.8817841970012523e-16`
- Both are below the `1e-13` acceptance threshold.
- The separated constraint used `0.00000124%` of its `1e-10` gate.

## Result

Pass. The separate repository resolved the exact published Solar merge,
linked `Solar::Relativity`, reproduced the Kerr shadow evidence, and executed
the public separated solver with render-relevant diagnostics.

## Limitations

This proves the external CPU library call and dependency lock. It does not
prove that the reference raster has migrated to the separated solver, CUDA
execution, GPU/CPU parity, radiative transfer, accretion-disk appearance,
OpenEXR output, temporal stability, or cinema-quality video. The edge
expectations are integration regression values, not a new independent
derivation of the underlying physics.

## Fastest falsification

Run the commands above after changing Solar or the lock. Configuration must
reject a dirty local checkout or one whose `HEAD` differs from the lock. The
probe must fail on a version/contract mismatch, shadow regression, missing or
zero-step separated integration, non-finite render diagnostic, wrong
termination, or a constraint value at or above `1e-10`.
