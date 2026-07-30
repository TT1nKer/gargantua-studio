# Validation: Solar-backed CPU reference frame

## Claim

Gargantua Studio can deterministically trace a complete CPU reference image
through Solar's public Kerr null-geodesic API, preserve numerical failures as
visible data, and atomically write reproducible per-pixel evidence.

## Model boundary

The acceptance image uses a Kerr spacetime in Boyer-Lindquist coordinates,
signature `(-,+,+,+)`, geometrized units `G=c=1`, a local ZAMO camera, and
one double-precision pixel-center ray per pixel. Capture means that an inward
ray reached the explicit exterior cutoff
`r_capture = r_+ + 1e-3 M`; it does not mean that a BL trajectory crossed the
event horizon.

The image classifies rays only. It contains no disk, emitting material,
radiative transfer, redshift, lensing intensity, temporal sampling, or
film-look processing.

## Reference

The dependency is locked to:

- Solar commit: `919e082b5e2473aac66ec364f22fd6838afd73b2`
- Solar package version: `0.2.0-alpha.1`
- Solar physics contract: `relativity-v3-phase2`
- Solar target: `Solar::Relativity`

Solar independently tests the Schwarzschild critical impact parameter
`b_c = 3 sqrt(3) M`. For a static Schwarzschild observer at radius `r_o`, the
local shadow angle used here is

```text
sin(alpha) = b_c sqrt(1 - 2M/r_o) / r_o.
```

At zero spin the ZAMO is that static observer. The camera then maps the
analytic angle to the perspective image plane. This supplies an independent
finite-distance screen-radius check in addition to Solar's own Bardeen curve
and backward-ray tests.

## Command

The accepted Kerr generation and its repeat were produced from a clean
Gargantua worktree at
`373be02aa196247177286c63844acd899261b05c`:

```sh
cmake -S . -B build \
  -DGARGANTUA_SOLAR_SOURCE_DIR=/Users/hostsjim/project/solar/.worktrees/gargantua-bootstrap \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

./build/gargantua-render-reference \
  --output artifacts/reference-frame-v1 \
  --mass-M 1 --spin 0.5 --observer-r-M 30 \
  --inclination-deg 85 --fov-y-deg 40 \
  --width 64 --height 36 --escape-r-M 60 \
  --max-affine-M 200 --initial-step-M 0.02 --max-step-M 0.25

./build/gargantua-render-reference \
  --output artifacts/reference-frame-v1-repeat \
  --mass-M 1 --spin 0.5 --observer-r-M 30 \
  --inclination-deg 85 --fov-y-deg 40 \
  --width 64 --height 36 --escape-r-M 60 \
  --max-affine-M 200 --initial-step-M 0.02 --max-step-M 0.25

cmp artifacts/reference-frame-v1/classification.ppm \
    artifacts/reference-frame-v1-repeat/classification.ppm
cmp artifacts/reference-frame-v1/rays.csv \
    artifacts/reference-frame-v1-repeat/rays.csv
cmp artifacts/reference-frame-v1/manifest.json \
    artifacts/reference-frame-v1-repeat/manifest.json
```

The independent circular-boundary check used:

```sh
./build/gargantua-render-reference \
  --output artifacts/reference-schwarzschild-validation-v1 \
  --mass-M 1 --spin 0 --observer-r-M 30 \
  --inclination-deg 90 --fov-y-deg 40 \
  --width 64 --height 64 --escape-r-M 60 \
  --max-affine-M 200 --initial-step-M 0.02 --max-step-M 0.25
```

Compiler: Apple clang `16.0.0 (clang-1600.0.26.6)`, arm64 Darwin.

## Inputs

The accepted Kerr frame contains `64 * 36 = 2304` rays. Its spin is `0.5`,
observer radius is `30 M`, inclination is `85 degrees`, vertical field of
view is `40 degrees`, escape radius is `60 M`, and maximum affine length is
`200 M`. DOPRI5 starts at `0.02 M` and is capped at `0.25 M`.

The numerical gates are:

- normalized Hamiltonian error `< 1e-10`;
- energy relative error `< 1e-12`;
- axial-angular-momentum relative error `< 1e-12`;
- Carter relative error `< 1e-9`.

## Expected

- Both Kerr commands exit `0`.
- Captured and escaped populations are both nonzero and total `2304`.
- No unconverged, constraint-violation, or initialization-error rays occur.
- PPM, CSV, and manifest are byte-identical between repeated runs.
- Every accepted ray remains below every declared invariant gate.
- The Schwarzschild frame exits `0`, is exactly symmetric in row and column
  counts, and brackets the analytic shadow radius within one pixel.
- The manifest records the exact clean Gargantua build, locked Solar commit,
  versions, model boundary, and explicit missing capabilities.

## Actual

Both Kerr commands exited `0`:

```json
{"status":"complete","captured":218,"escaped":2086,"failed":0,"ppm_checksum_fnv1a64":"e572ca151ab8c4ee","csv_checksum_fnv1a64":"94671d7df84e4dc9"}
```

The two runs took `10.25 s` and `9.53 s` wall time (`4.28 s` and `4.31 s`
user CPU time) in the validation environment. All three files compared
byte-for-byte equal. The manifest recorded Gargantua
`373be02aa196247177286c63844acd899261b05c`, `dirty=false`, and the locked
Solar identity above.

Kerr diagnostics:

- capture radius: `1.8670254037844385 M`;
- captured: `218`;
- escaped: `2086`;
- failed: `0`;
- maximum Hamiltonian error: `8.848922300665704e-11`;
- maximum energy relative error: `0`;
- maximum axial-angular-momentum relative error: `0`;
- maximum Carter relative error: `1.702319665582084e-10`;
- maximum accepted/rejected steps: `923 / 2`;
- scene FNV-1a: `f8a76a72343cea69`;
- PPM: `6925` bytes, FNV-1a `e572ca151ab8c4ee`;
- CSV: `220481` bytes, FNV-1a `94671d7df84e4dc9`.

SHA-256 values, provided separately from the manifest's named determinism
checksum, were:

```text
ed1e21cae38163adba4608572468b70923a9f0bc42391f7ae943ae6dff9ead4f  classification.ppm
e161ec02d2d1db382d7528271744172d87c256151b82a2f76350c2c638dcf6e9  rays.csv
d794a0fc72047291ae8886ddbae7d5abc2f24deab4ea45965a8ca4090d41989c  manifest.json
```

The Schwarzschild run also exited `0`: `708` captured, `3388` escaped,
and `0` failed. Its captured bounding box was exactly
`x=17..46, y=17..46`, with exact opposing row and column count symmetry.

## Error

The repeated Kerr outputs had zero differing bytes. Classification accounting
error was zero: `218 + 2086 = 2304`. The maximum Hamiltonian error used
`88.49%` of its gate, leaving `1.1511e-11`; the maximum Carter error used
`17.02%` of its gate. Energy and axial-angular-momentum drift were zero at the
precision exported by Solar.

For the `64 by 64` Schwarzschild frame:

```text
analytic local angle       = 9.632732282697253 degrees
analytic image radius      = 14.922101623505476 pixels
observed boundary bracket  = [14.5, 15.5] pixels
bracket midpoint error     = 0.077898376494524 pixels
relative midpoint error    = 0.5220335476861874%
```

The analytic radius lies inside the one-pixel classification bracket, and
the measured midpoint error is below one tenth of a pixel. Pixelization and
integration error are not conflated: the bracket is imposed by discrete
pixel centers, while invariant maxima report integration quality.

Validation also falsified the original `r_+ + 1e-4 M` cutoff. A `128 by 128`
Schwarzschild stress raster produced `22` strict Hamiltonian failures with a
maximum of `1.0502e-10`; smaller integration steps still produced `16`
failures. The cutoff was moved to `r_+ + 1e-3 M`, while the `1e-10` gate was
left unchanged. The accepted Schwarzschild and Kerr frames then had zero
failed rays.

## Result

Pass for the first CPU classification-image vertical slice. It proves a real
per-pixel Solar integration path, deterministic serialization, explicit
failure semantics, numerical gates, finite-distance Schwarzschild boundary
accuracy, and reproducible build/dependency provenance.

It is not evidence of a cinema-quality render.

## Limitations

- Boyer-Lindquist coordinates do not support horizon crossing or robust polar
  axis crossing; capture remains an exterior cutoff.
- The Kerr acceptance frame is checked against invariant gates and Solar's
  separately validated analytical shadow machinery, but this small finite
  observer raster is not an independent high-resolution Kerr boundary survey.
- There is no minimum trajectory radius, azimuthal winding, selected event
  history, disk-intersection stream, redshift, radiative transfer, adaptive
  image sampling, CUDA path, CPU/GPU parity, OpenEXR, ACES, animation, or
  beauty shading.
- FNV-1a detects accidental changes and determinism regressions; it is not a
  cryptographic authenticity mechanism. SHA-256 values above are external
  evidence, not part of schema v1.
- Wall time is hardware and scheduling dependent and is not a performance
  acceptance gate.

The next Solar-facing requirement is a bounded trajectory-diagnostics
observer or result API that reports minimum radius, winding, and selected
event history without retaining every integration step or importing
Gargantua types.

## Fastest falsification

From a clean checkout at the recorded commits, run the build, CTest, two Kerr
commands, three `cmp` commands, and the Schwarzschild command above. Fail this
claim if any command returns nonzero, a manifest identity differs, a count
does not equal the image dimensions, a failed-ray count is nonzero, a maximum
reaches its declared gate, repeated files differ, opposing Schwarzschild
row/column counts differ, or the analytic radius falls outside the observed
one-pixel bracket.
