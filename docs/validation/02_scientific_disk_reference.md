# Validation: scientific Kerr thin-disk reference v2

## Claim

Gargantua Studio can deterministically trace a complete CPU Kerr frame through
Solar's public separated-Mino and thin-disk APIs, preserve per-ray physical
evidence, and emit a reproducible scientific preview without hiding numerical
failures.

## Model boundary

The accepted frame uses the Kerr metric in Boyer-Lindquist coordinates,
signature `(-,+,+,+)`, geometrized units `G=c=1`, a local ZAMO camera, and one
double-precision pixel-center ray per pixel. Photon momentum is future-directed
while negative Mino parameter integrates observer-to-past.

The surface model is Solar's analytic circular thin disk from prograde ISCO to
`20 M`. The accepted frame is opaque. Redshift and invariant intensity are
evaluated at the exact selected surface event. `beauty.ppm` applies only the
declared deterministic `Reinhard -> linear-to-sRGB` grayscale transform to raw
bolometric intensity.

Capture means reaching `r_+ + 1e-3 M` in the exterior BL chart. It does not
claim event-horizon crossing. This is not a GRMHD, spectral, polarized, or
film-look model.

## References

- Solar package: `Solar::Relativity`
- Solar commit: `635d99f47fa50be892416986f2723d035ee2acc1`
- Solar version/contract: `0.2.0-alpha.1` /
  `relativity-v3-phase2`
- Gargantua source commit:
  `9425c0428169a4a44a292698a8c17f839938ea19`
- Exact machine-readable values:
  [`tests/fixtures/scientific_reference_v2.json`](../../tests/fixtures/scientific_reference_v2.json)
- Previous classification-only baseline:
  [`01_cpu_reference_frame.md`](01_cpu_reference_frame.md)

For the independent Schwarzschild boundary, the finite-distance local shadow
angle is

```text
sin(alpha) = 3 sqrt(3) M sqrt(1 - 2M/r_o) / r_o.
```

The perspective camera maps `tan(alpha)` onto the image plane.

## Commands

The accepted build fetched the locked Solar commit from its public repository:

```sh
cmake -S . -B build-release-final -DCMAKE_BUILD_TYPE=Release
cmake --build build-release-final --parallel 4
ctest --test-dir build-release-final --output-on-failure
```

Two clean generations used identical options and distinct output directories:

```sh
./build-release-final/gargantua-render-reference \
  --output artifacts/scientific-reference-v2-horizon-metadata \
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

The repeat changed only `--output` to
`artifacts/scientific-reference-v2-horizon-metadata-repeat`. `cmp` checked
all four files. The refreshed manifest records Solar's computed outer-horizon
radius; the beauty, classification, and CSV files remained byte-identical to
the pre-refactor accepted generation. A read-only AWK pass independently
checked population accounting, negative final affine values, invariant gates,
and disk evidence. A separate Ruby pass recomputed FNV-1a from raw file bytes
rather than trusting the manifest.

The Schwarzschild check used:

```sh
./build-release-final/gargantua-render-reference \
  --output artifacts/scientific-schwarzschild-boundary-v2-horizon-metadata \
  --mass-M 1 --spin 0 --observer-r-M 30 \
  --inclination-deg 90 --fov-y-deg 40 \
  --width 64 --height 64 --escape-r-M 60 \
  --max-affine-M 200 \
  --initial-mino-step 0.02 --max-mino-step 0.25 \
  --disk-outer-r-M 6.1 \
  --disk-opacity semi-transparent \
  --disk-surface-optical-depth 0 \
  --disk-max-crossings 8 --exposure 1
```

The narrow `6.0..6.1 M` zero-depth surface preserves Solar's valid disk
contract while leaving capture/escape as the boundary classification.

Environment: macOS `14.8.7`, arm64, AppleClang
`16.0.0 (clang-1600.0.26.6)`, Release.

The final warning-enabled Release build completed without a new compiler
warning, and CTest passed `10/10` in `16.77 s`. A Debug build with combined
ASan/UBSan and frame pointers passed all `10/10` tests without a sanitizer
diagnostic: the six physics/render/output tests passed in `624.31 s`
(`solar_kerr_path` used `621.68 s`), and the remaining probe, parser, CLI, and
metadata tests passed in `170.94 s`.

The dependency audit found no positive-Mino production path, no GR or transfer
formula outside the Solar adapter, no display mutation of raw evidence, no
failed classification accepted as success, and no CLI dependency below the
public reference layer. `solar_kerr_ray_tracer.cpp` is `95` lines. The larger
manifest, serialization, option-parser, and path files each retain one
cohesive responsibility; no CUDA, WebGL, OpenEXR, or UI implementation is
present.

## Inputs

The primary scene is `64 x 36`, spin `0.5`, observer radius `30 M`,
inclination `85 degrees`, vertical FOV `40 degrees`, escape radius `60 M`,
and maximum affine magnitude `200 M`. Mino DOPRI5 begins at `0.02` and is
capped at `0.25`.

The strict accepted-ray gates are:

- normalized Hamiltonian error `< 1e-10`;
- energy relative error `< 1e-12`;
- axial-angular-momentum relative error `< 1e-12`;
- Carter relative error `< 1e-9`.

## Expected

- Both primary commands exit `0`, with byte-identical output directories.
- `captured + escaped + disk_surface_hits = 2304`; each population is nonzero.
- No unconverged, constraint, initialization, or transfer failure occurs.
- Every disk hit has `g > 0`, non-negative raw intensities, and at least one
  recorded crossing; at least one emitted intensity is strictly positive.
- Every advanced final affine value is negative.
- Every accepted ray remains strictly below all four numerical gates.
- File byte counts and independently recomputed checksums match the manifest.
- The Schwarzschild frame is row/column symmetric and its analytic shadow
  radius lies inside the observed one-pixel bracket.
- The preview shows the disk and its lensed image without diagnostic colors.

## Actual

Both primary commands returned:

```json
{"status":"complete","captured":120,"escaped":1287,"disk_surface_hits":897,"disk_crossings":897,"failed":0,"beauty_ppm_checksum_fnv1a64":"69366ae1b7ee86b0","classification_ppm_checksum_fnv1a64":"2c632e9865f0e440","csv_checksum_fnv1a64":"2d51d7077551d6ee"}
```

All four repeated files were byte-identical. Independent accounting reported:

```text
rows=2304 captured=120 escaped=1287 disk=897 failed=0
crossings=897 positive_emission=897
nonnegative_affine=0 h_gate_fail=0 e_gate_fail=0
lz_gate_fail=0 carter_gate_fail=0 disk_evidence_fail=0
```

Primary maxima:

- Hamiltonian: `2.0395437490124286e-14`;
- energy drift: `0`;
- axial-angular-momentum drift: `0`;
- Carter drift: `7.677006197971529e-14`;
- redshift `g`: `1.4734490847779178`;
- observed specific intensity: `1.4082970614670725`;
- observed bolometric intensity: `0.22800185228949879`;
- accepted/rejected steps: `2768 / 54`.

Independent file evidence:

| File | Bytes | FNV-1a 64 | SHA-256 |
|---|---:|---|---|
| `beauty.ppm` | 6925 | `69366ae1b7ee86b0` | `fc742cf970fb58d53d1f2c93b5adad814819a87e6724cba96bf9330850ba15f9` |
| `classification.ppm` | 6925 | `2c632e9865f0e440` | `b1de3bc7b01494b9f8439ef2f71dd23b0269291642e28f4b8c6d847adf6adb4f` |
| `rays.csv` | 492203 | `2d51d7077551d6ee` | `586879ef3c64f86cbccf65ad83ba5fdf3473a94d5207949a915c47be77b8c84c` |
| `manifest.json` | — | — | `9506760c866e6d299519a25df599ef257bd985f967b174679ddb0ed1c0906671` |

The classification palette contained exactly `120` black capture pixels,
`897` cyan disk pixels, and `1287` gray escape pixels. No failure color was
present. The beauty image was grayscale, had `895` non-black 8-bit pixels and
peak channel `119`, and visibly contained the main disk, upper lensed image,
and central shadow. The temporary PNG used for inspection is not retained.

The Schwarzschild command returned `708` captured, `3388` escaped, and zero
failures. Its captured box was exactly `x=17..46, y=17..46`; opposing row and
column counts matched exactly. The analytic local angle was
`9.632732282697253 degrees`, giving image radius
`14.922101623505476 pixels` inside the observed `[14.5,15.5]` bracket.

## Error

Repeated-output error was zero bytes. Classification accounting error was
zero: `120 + 1287 + 897 = 2304`.

The maximum Hamiltonian error used `0.020395%` of its gate. The maximum Carter
error used `0.007677%` of its gate. Energy and axial-angular-momentum drift
were zero at exported precision.

The Schwarzschild bracket midpoint error was
`0.0778983764945238 pixels`, or `0.5220335%` of the analytic image radius.
Pixelization error is reported separately from the integration invariants.

During validation, Solar commit `5459a53...` exposed 12 near-cutoff constraint
failures in the `64 x 64` boundary frame. Smaller Mino steps and larger BL
capture margins did not remove them. The cause was that turning-phase event
hits returned phase-velocity momenta while ordinary accepted states returned
separated-potential projections. Solar PR
[`#11`](https://github.com/TT1nKer/solar/pull/11) unified the event-state
contract. The regression reduced the event Hamiltonian error from
`3.1724e-11` at `-O2` (and up to `1.2947e-10` in the Gargantua `-O3` frame)
to `3.0314e-16`; the accepted boundary then had zero failures. The primary
classification and beauty checksums remained unchanged, while the evidence
CSV changed as expected.

The final repository-boundary audit also found Kerr's outer-horizon formula
duplicated in Gargantua's generic scene and output validation. Commit
`9425c04...` removed both copies: Solar now computes the horizon once, the
adapter owns Kerr-domain checks, and serialized tracer provenance carries
`outer_horizon_radius_M = 1.8660254037844386`. Repeated image and CSV bytes
were unchanged.

## Result

Pass for the first Solar-backed scientific thin-disk reference. It proves
deterministic observer-to-past Kerr paths, exact selected surface events,
redshift/intensity evidence, visible failure semantics, strict numerical
gates, atomic four-file output, dependency provenance, and an independent
Schwarzschild screen-radius check.

It is not evidence of a cinema-quality frame.

## Limitations

- The analytic surface is not volume transfer or GRMHD.
- The preview is 8-bit grayscale, not spectral, HDR, OpenEXR, or ACES output.
- BL capture is an exterior cutoff; Kerr-Schild horizon crossing is absent.
- BL polar-axis crossing remains unsupported.
- Polarization, returning radiation, spectral calibration, time evolution,
  adaptive sampling, CUDA, CPU/GPU parity, denoising, compositing, and film
  tooling are absent.
- The `64 x 36` reference is a deterministic regression, not a
  production-resolution accuracy or performance benchmark.
- FNV-1a is a determinism checksum, not an authenticity mechanism; SHA-256 is
  recorded separately.

## Fastest falsification

At the recorded commits, run the Release build, CTest, the two primary
commands, four `cmp` checks, the CSV accounting pass, the independent checksum
pass, and the Schwarzschild command. Fail the claim if a command returns
nonzero; provenance differs; repeated bytes differ; populations do not sum to
the raster; any failed ray appears; any accepted invariant reaches its gate;
disk evidence is invalid; an advanced affine value is non-negative; opposing
Schwarzschild counts differ; or the analytic radius leaves the one-pixel
bracket.
