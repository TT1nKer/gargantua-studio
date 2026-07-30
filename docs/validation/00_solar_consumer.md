# Validation: Gargantua consumes Solar Phase 5

## Claim

Gargantua Studio can resolve, compile, link, and execute the published Solar
Phase 5 public API as an independent CMake consumer.

## Model boundary

The probe executes four independent public-library paths in fixed Kerr and
flat backgrounds using `double` and geometrized units:

- the Phase 2 Bardeen critical curve at `M=1`, `chi=0.5`;
- a Phase 3 short separated null geodesic in Mino time;
- the Phase 5 constant-coefficient observer-to-past formal solution;
- Phase 5 analytic disk/torus material and one opaque thin-disk surface
  crossing.

The material temperature, density, and intensity scales are caller-selected
model units. The disk is a controlled circular zero-torque profile and the
torus is kinematic; neither is GRMHD.

## Solar dependency

- Git commit: `635d99f47fa50be892416986f2723d035ee2acc1`
- Package version: `0.2.0-alpha.1`
- Physics contract: `relativity-v3-phase2`
- Public target: `Solar::Relativity`

The physics-contract label predates Phase 5. Generated evidence must therefore
retain the exact Git commit as well as the package identifiers.

This lock includes Solar PR #10's consumer-driven correction: material
support is checked before circular four-velocity construction, so exact
equatorial events inside the Solar-owned ISCO remain vacuum samples instead
of false transfer failures.

## Command

```sh
cmake -S . -B build-phase5 -DCMAKE_BUILD_TYPE=Release
cmake --build build-phase5 --parallel 4
ctest --test-dir build-phase5 -R gargantua.solar_probe --output-on-failure
./build-phase5/gargantua-probe
```

Omitting `GARGANTUA_SOLAR_SOURCE_DIR` exercised the canonical repository-only
`FetchContent` path at the locked commit.

Platform: Darwin arm64, AppleClang
`16.0.0.16000026`.

## Inputs

- Kerr BL mass/spin: `1 / 0.5`
- Shadow inclination: `pi/2`
- Shadow samples per branch: `65`
- Separated initial/max Mino step: `1e-5 / 1e-4`
- Separated maximum affine displacement: `0.1`
- Transfer coefficients: `J=2`, `A=0.5`, `ds=3`
- Disk sample: `r=8`, configured inner/outer radii `6 / 20`
- Torus center/width: `8 / 2`
- Surface mode: opaque, maximum eight crossings

## Expected

- Shadow samples: `128`
- Shadow edges:
  `-4.096266658713869` and `6.138155724715452`,
  absolute tolerance `1e-13`
- Separated termination: `MaxAffine`, with at least one accepted step
- Separated Hamiltonian constraint: `< 1e-10`
- Transfer intensity:
  `3.1074793594062806`, absolute tolerance `5e-14`
- Transfer transmission:
  `0.22313016014842982`, absolute tolerance `5e-14`
- Valid disk and torus samples
- Exactly one successfully composed surface crossing

## Actual

The focused CTest passed `1/1`. The probe printed:

```json
{"engine":"solar","solar_version":"0.2.0-alpha.1","physics_contract":"relativity-v3-phase2","metric":"kerr-bl","mass":1,"spin":0.5,"samples":128,"left":-4.0962666587138692,"right":6.1381557247154532,"separated_steps":4,"separated_constraint":1.2394276503472693e-16,"separated_min_radius_M":19.905098572994902,"separated_winding":2.1111653880200616e-06,"transfer_intensity":3.1074793594062804,"transfer_transmission":0.22313016014842982,"disk_temperature":8,"torus_density":3,"surface_specific_intensity":6.0000000000000036,"surface_crossings":1}
```

Before moving the lock, the same Gargantua source failed to compile at:

```text
fatal error: 'solar/relativity/fluid_model.h' file not found
```

That red step proves this gate distinguishes the pre-Phase 5 dependency from
the published Phase 5 commit.

## Error

- Left-edge absolute error: `0`
- Right-edge absolute error: `8.8817841970012523e-16`
- Transfer-intensity absolute error: `2.2204460492503131e-16`
- Transfer-transmission absolute error: `0`
- Separated normalized Hamiltonian constraint:
  `1.2394276503472693e-16`

Every measured error is below its declared gate.

## Result

Pass. The independent Gargantua build consumes real Solar Phase 5 headers and
symbols and executes transfer, matter, and surface composition rather than
only compiling declarations.

## Limitations

This gate does not prove that Gargantua's raster uses the separated solver or
Phase 5 transfer yet. It does not validate repeated geodesic disk events,
observer-to-past camera direction, a beauty image, absolute spectra, volume
transfer, Kerr-Schild horizon crossing, CUDA, OpenEXR, or animation.

## Fastest falsification

Build the probe against the previous Solar lock
`82acb4e6c60e1fa18447cf37370278d5ac9e82f8`; compilation must fail on a
Phase 5 public header. Against the accepted lock, change the transfer
attenuation exponent sign or pass the disk event state outside material
support; the numerical/output gate must fail rather than print a passing JSON
record.
