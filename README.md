# Gargantua Studio

Gargantua Studio is an offline Kerr black-hole film renderer powered and
validated by the independent Solar physics library.

The current vertical slice traces one Solar null geodesic per pixel and writes
a deterministic CPU classification frame. It is physics-engine evidence, not
yet a radiation model, CUDA renderer, or cinema-quality image.

## Build against the locked local Solar checkout

The Solar checkout must be clean and at the exact commit recorded in
`cmake/solar-lock.cmake`:

```sh
cmake -S . -B build \
  -DGARGANTUA_SOLAR_SOURCE_DIR=/path/to/solar-at-the-locked-commit
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/gargantua-probe
```

Omit `GARGANTUA_SOLAR_SOURCE_DIR` to fetch the same commit from the canonical
Solar repository after that commit has been published. Generated artifacts
must record the Solar version and physics contract.

## Render the reference frame

```sh
./build/gargantua-render-reference \
  --output artifacts/reference-frame-v1 \
  --mass-M 1 --spin 0.5 --observer-r-M 30 \
  --inclination-deg 85 --fov-y-deg 40 \
  --width 64 --height 36 --escape-r-M 60 \
  --max-affine-M 200 --initial-step-M 0.02 --max-step-M 0.25
```

The output is one atomically finalized generation directory:

- `classification.ppm`: binary P6 debug image;
- `rays.csv`: one row per pixel with termination and invariant diagnostics;
- `manifest.json`: scene, dependency provenance, numerical gates, counts,
  maxima, byte counts, and FNV-1a determinism checksums.

The fixed debug palette is black for capture at the explicit BL cutoff,
neutral gray `(235,235,235)` for escape, magenta for unconverged rays, orange
for constraint violations, and yellow for initialization failures. Any failed
ray makes the manifest `diagnostic_failed` and the command exit `4`.

## Validation

- [Solar external-consumer contract](docs/validation/00_solar_consumer.md)
- [Solar-backed CPU reference frame](docs/validation/01_cpu_reference_frame.md)

The current capture event is `r_+ + 1e-3 M` in Boyer-Lindquist coordinates.
It is an exterior classification cutoff, not a horizon-crossing claim. The
locked Solar Phase 3 API now provides separated Mino-time evolution, minimum
radius, and azimuthal winding; `gargantua-probe` exercises that contract. The
reference raster deliberately remains on the generic Hamiltonian path until
a dedicated migration can compare complete classifications and invariants.
Selected event history, disk intersections, redshift/radiative transfer,
Kerr-Schild horizon crossing, CUDA, OpenEXR/ACES, temporal validation, and
beauty rendering remain explicit missing capabilities.
