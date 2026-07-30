# Gargantua Studio

Gargantua Studio is a long-term offline Kerr black-hole film-rendering
project powered by the independent Solar physics library.

The current validated milestone is a deterministic CPU scientific reference:
future-directed photons are integrated observer-to-past with Solar's
separated Kerr/Mino solver, intersect an analytic circular thin disk, and
record redshift and invariant-intensity evidence. It is not yet a GRMHD
simulation or a cinema-quality renderer.

## Repository boundary

- **Solar** owns metrics, observers, geodesics, event states, fluid models,
  horizon/domain calculations, and radiative-transfer primitives.
- **Gargantua Studio** owns cameras, render flows, artifact schemas,
  diagnostics, and eventually the offline film pipeline.

`cmake/solar-lock.cmake` pins Solar commit
`635d99f47fa50be892416986f2723d035ee2acc1`. Local Solar checkouts must be
clean and match that commit; otherwise configuration fails.

## Build

To fetch the locked public Solar revision:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/gargantua-probe
```

For a matching local checkout:

```sh
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DGARGANTUA_SOLAR_SOURCE_DIR=/path/to/solar-at-the-locked-commit
```

## Render the scientific reference

```sh
mkdir -p artifacts
./build/gargantua-render-reference \
  --output artifacts/scientific-reference-v2 \
  --mass-M 1 --spin 0.5 --observer-r-M 30 \
  --inclination-deg 85 --fov-y-deg 40 \
  --width 64 --height 36 --escape-r-M 60 \
  --max-affine-M 200 \
  --initial-mino-step 0.02 --max-mino-step 0.25 \
  --disk-outer-r-M 20 \
  --disk-temperature-scale 1 \
  --disk-density-scale 1 --disk-density-power 0.75 \
  --disk-specific-scale 1 --disk-bolometric-scale 1 \
  --disk-opacity opaque --disk-max-crossings 8 \
  --exposure 1
```

The command atomically finalizes four files:

- `beauty.ppm`: deterministic 8-bit grayscale preview of raw observed
  bolometric intensity through `Reinhard -> linear-to-sRGB`; this is a
  scientific preview, not a film look;
- `classification.ppm`: capture, escape, disk-hit, and failure diagnostics;
- `rays.csv`: one raw evidence row per pixel, including path extrema,
  winding, invariants, disk crossings, redshift, temperature, and intensity;
- `manifest.json`: scene, dependency provenance, model boundary, numerical
  gates, Solar-computed horizon/capture metadata, counts, maxima, byte sizes,
  and FNV-1a checksums.

Any failed ray makes the frame `diagnostic_failed` and the command exits `4`.
Output is never silently overwritten, and incomplete generations use a
guarded `.part` directory.

## Physics convention

The camera launches a future-directed photon momentum and integrates with
negative Mino parameter from the observer into the past. Capture means reaching
the explicit Boyer-Lindquist exterior cutoff
`r_capture = r_+ + 1e-3 M`; it is not a horizon-crossing claim.

The analytic disk begins at the matching prograde ISCO. Opaque mode terminates
at the first supported surface hit; semi-transparent mode composes repeated
crossings up to the configured bound.

## Validation

- [Solar external-consumer contract](docs/validation/00_solar_consumer.md)
- [Initial CPU classification frame](docs/validation/01_cpu_reference_frame.md)
- [Scientific disk reference v2](docs/validation/02_scientific_disk_reference.md)
- [Pinned scientific fixture](tests/fixtures/scientific_reference_v2.json)

The accepted `64 x 36` frame has nonzero capture, escape, and disk populations,
zero failures, byte-identical repeated outputs, and strict Hamiltonian,
stationary-invariant, and Carter gates. A separate `64 x 64` Schwarzschild
screen test brackets the analytic finite-distance shadow radius within one
pixel.

The accepted local build is macOS arm64 with AppleClang 16 in Release mode.
The repository CI gate independently configures, builds, tests, and renders a
small reference on `ubuntu-latest`. PR validation run
[`30547816453`](https://github.com/TT1nKer/gargantua-studio/actions/runs/30547816453)
passed all `10/10` tests and rendered with zero failed rays.

## Explicitly missing

Volume transfer, GRMHD material, polarization, returning radiation, spectral
calibration, Kerr-Schild horizon crossing, robust BL polar-axis crossing,
CUDA, OpenEXR/ACES, temporal sampling, denoising, compositing, and the film
pipeline remain future work. The current output is a verified physics
foundation for those layers, not evidence that they already exist.
