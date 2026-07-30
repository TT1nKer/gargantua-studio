# Validation: Gargantua consumes Solar Phase 2

## Claim

Gargantua Studio can compile and run as an independent CMake project while
calling the locked Solar Phase 2 public relativity API.

## Model boundary

The probe evaluates the Bardeen critical curve for a Kerr spacetime in
Boyer-Lindquist coordinates. It uses geometrized units with mass `M=1`,
dimensionless spin `chi=0.5`, equatorial inclination `pi/2`, and 65 requested
samples per branch. This is a CPU double-precision boundary calculation.

## Solar dependency

- Git commit: `919e082b5e2473aac66ec364f22fd6838afd73b2`
- Package version: `0.2.0-alpha.1`
- Physics contract: `relativity-v3-phase2`
- Public target: `Solar::Relativity`

## Command

```sh
cmake -S . -B /tmp/gargantua-final-build \
  -DGARGANTUA_SOLAR_SOURCE_DIR=/path/to/solar-at-919e082b5e2473aac66ec364f22fd6838afd73b2 \
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

## Expected

- Closed-curve sample count: `128`
- Left edge: `-4.096266658713869`
- Right edge: `6.138155724715452`
- Absolute edge tolerance: `1e-13`
- CTest result: one passing test containing the locked physics contract

## Actual

CTest passed `1/1`. The fresh probe output was:

```json
{"engine":"solar","solar_version":"0.2.0-alpha.1","physics_contract":"relativity-v3-phase2","metric":"kerr-bl","mass":1,"spin":0.5,"samples":128,"left":-4.0962666587138692,"right":6.1381557247154532}
```

The build emitted no warnings.

## Error

- Left-edge absolute error: `0`
- Right-edge absolute error: `8.8817841970012523e-16`
- Both are below the `1e-13` acceptance threshold.

## Result

Pass. The separate repository resolved the exact Solar commit, linked its
public relativity target, and reproduced the locked Kerr shadow evidence.

## Limitations

This proves the external CPU library call and dependency lock. It does not
prove CUDA execution, GPU/CPU parity, radiative transfer, accretion-disk
appearance, image or OpenEXR output, temporal stability, a renderer, or
cinema-quality video. The edge expectations are integration regression values,
not a new independent derivation of the underlying physics.

## Fastest falsification

Run the commands above after changing Solar or the lock. Configuration must
reject a local checkout whose `HEAD` differs from the lock; the probe must fail
if the version, physics contract, sample count, or either edge exceeds the
declared tolerance.
