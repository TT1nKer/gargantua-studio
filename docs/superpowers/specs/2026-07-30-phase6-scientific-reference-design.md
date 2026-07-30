# Phase 6 Scientific Reference Design

## Goal

Complete the first scientifically auditable Gargantua CPU beauty frame on top
of the published Solar Phase 5 library. The frame must trace observer-to-past
Kerr rays, localize thin-disk crossings, preserve raw redshift and invariant
intensity, and emit deterministic visual and per-pixel evidence.

This is the next vertical slice toward the offline cinematic renderer. It is
not the cinematic renderer itself.

## Authority and repository boundary

- Solar remains the sole authority for metrics, observer frames, geodesics,
  disk fluid kinematics, redshift, and surface-intensity composition.
- Gargantua owns scene configuration, ray orchestration, image traversal,
  display encoding, artifact serialization, and the future film pipeline.
- Gargantua locks the exact published Solar merge commit and records that
  commit, `solar::version`, and `solar::physics_contract` in every generation.
- No Solar source is copied into Gargantua.
- The CPU `double` path remains the reference. CUDA, WebGL, WASM, and UI work
  cannot redefine its classifications or physical samples.

The v3 prompt placed the first reference command in Solar. The later approved
two-repository architecture intentionally moves that command to Gargantua
without changing its physics or validation obligations.

## Correctness defect to remove first

The existing camera emits a spatially inward, future-directed photon and the
generic solver advances positive affine parameter. That is not an
observer-to-past ray. It can produce a plausible classification image in
simple fixtures but is not the v3 contract and is unsafe for Kerr redshift.

The corrected convention is:

1. A camera pixel constructs the future-directed photon momentum arriving at
   the observer.
2. The central local spatial direction is outward in the ZAMO tetrad.
3. Screen offsets are the sign inverse of the existing observer-to-scene
   direction, preserving the displayed image orientation.
4. The separated integrator advances negative Mino parameter.
5. Observer frequency remains positive and is normalized to one.
6. The final affine parameter for every advanced ray is negative.

Tests must fail if the direction or integration sign is reverted.

## Scope

### Included

- Lock Gargantua to Solar merge commit
  `35aaf231edb847a95c7239c51f63e01b34c2051b`.
- Compile and execute Solar Phase 5 thin-disk public APIs from Gargantua.
- Migrate the reference raster from the generic Hamiltonian solver to Solar's
  separated Kerr solver.
- Preserve capture as the explicit Boyer-Lindquist exterior cutoff
  `r_+ + 1e-3 M`; do not call it horizon crossing.
- Trace an analytic prograde scientific thin disk with an ISCO inner edge.
- Support opaque and finite-optical-depth semi-transparent surface modes.
- Continue through equatorial crossings outside disk support.
- Bound and record repeated valid disk crossings.
- Add disk-hit and transfer-failure classifications.
- Record minimum radius, winding, affine endpoint, redshift, disk radius,
  observed temperature, specific intensity, bolometric intensity, and
  crossing count per pixel.
- Write `beauty.ppm`, `classification.ppm`, `rays.csv`, and `manifest.json`
  atomically.
- Use a documented deterministic scientific display transform while retaining
  unmodified linear values in CSV.
- Produce a repeatable reference generation and validation report.

### Excluded

- GRMHD, polarization, scattering, returning radiation, self-gravity, or
  spectral detector calibration.
- Volumetric torus integration; Solar's torus remains available for a later
  volume-transfer slice.
- Kerr-Schild horizon-crossing imagery.
- Star fields, environment maps, bloom, lens flare, film grain, ACES, OpenEXR,
  denoising, temporal sampling, motion blur, depth of field, or sound.
- CUDA, WebGL, WASM, web UI, and CPU/GPU comparison.
- Automatic exposure and art-directed frequency compression.

## Layered architecture

### L0: scene and result contracts

`ReferenceScene` gains a focused `ScientificDiskScene` value:

- outer radius in `M`;
- density and temperature scales in caller-selected model units;
- density radial power;
- specific and bolometric intensity scales;
- opacity mode and finite surface optical depth;
- maximum valid crossings;
- fixed display exposure.

The inner edge remains Solar's prograde ISCO default. Scene validation rejects
non-finite values, a disk outside its admissible radial order, invalid opacity,
zero crossing limits, and display exposure that is not positive.

`ReferenceRayResult` remains a plain evidence value. It gains:

- `final_affine_M`;
- `min_radius_M`;
- `winding`;
- `disk_radius_M`;
- `redshift_g`;
- `observed_temperature`;
- `observed_specific_intensity`;
- `observed_bolometric_intensity`;
- `disk_crossings`.

Unavailable physical values are quiet NaNs only for failed or physically
inapplicable rays. Successful escaped/captured rays without a disk hit use
zero intensity and crossing count, with disk-specific coordinates and
redshift unavailable.

### L1: Solar path adapter

`solar_kerr_ray_tracer.cpp` is already above the local 200-line review signal
and owns unrelated responsibilities. Split it at real boundaries:

- `solar_kerr_ray_tracer.cpp`: public factory and Solar object construction;
- `solar_kerr_path.cpp`: one-pixel separated propagation and event state
  machine;
- `solar_kerr_path.h`: internal path configuration/result boundary;
- `reference_ray_evidence.cpp`: conversion and validation of Solar diagnostics
  into `ReferenceRayResult`.

The path integrator owns three event families:

- decreasing-radius BL capture cutoff;
- increasing-radius escape;
- equatorial surface crossing.

The first disk event accepts either crossing direction. After each localized
crossing, the next event is restricted to the opposite polar direction. At an
exact restart state this prevents the just-consumed endpoint root from firing
again, while detecting the next physical crossing without changing position,
momentum, or affine parameter.

At every disk event:

1. Pass the exact Solar event state to `ThinDiskCrossingRecorder`.
2. If the crossing is outside radial support, retain no material record and
   continue.
3. If opaque material closes the recorder, terminate as `DiskSurfaceHit`.
4. If semi-transparent material remains open, continue until capture, escape,
   or the configured crossing bound.
5. Treat a Solar transfer error, tangential/indeterminate crossing direction,
   or exhausted crossing bound as an explicit failed ray.

Segment diagnostics are aggregated monotonically:

- sum accepted and rejected steps;
- minimum of segment minimum radii;
- sum signed winding;
- maximum constraint and Carter errors;
- recompute final `E`, `Lz`, and `Q` drift against the initialized constants.

The total remaining affine budget decreases after every segment. A restart
must not reset the scene's maximum path length.

### L2: reference frame flow

`render_reference_frame` remains a row-major, one-call-per-pixel coordinator.
It does not know Solar types or disk equations. It validates all returned
evidence through the existing summary boundary and marks any failed ray as
`diagnostic_failed`.

No partial `ReferenceFrame` escapes if a tracer throws, a classification is
unknown, or evidence is structurally inconsistent.

### L3: CLI and artifact diplomacy

The CLI continues to parse external strings into `ReferenceScene` before
calling the renderer. New explicit disk options are:

- `--disk-outer-r-M`;
- `--disk-temperature-scale`;
- `--disk-density-scale`;
- `--disk-density-power`;
- `--disk-specific-scale`;
- `--disk-bolometric-scale`;
- `--disk-opacity opaque|semi-transparent`;
- `--disk-surface-optical-depth`;
- `--disk-max-crossings`;
- `--exposure`.

The existing `--initial-step-M` and `--max-step-M` spellings remain accepted
for compatibility during this prerelease, but the manifest describes them as
Mino-step controls and the help text marks the legacy names. New canonical
spellings are `--initial-mino-step` and `--max-mino-step`; aliases cannot be
combined in one command.

## Scientific display transform

Raw bolometric intensity is non-negative and remains unchanged in `rays.csv`.
`beauty.ppm` is a deterministic display preview:

```text
linear = exposure * observed_bolometric_intensity
mapped = linear / (1 + linear)
srgb   = linear_to_srgb(mapped)
byte   = round(255 * clamp(srgb, 0, 1))
```

The same scalar is written to R, G, and B. This is a scientific grayscale
preview, not a spectral or film-color claim. Captured and escaped rays with no
emission are black. Failed rays use the existing diagnostic palette in
`beauty.ppm` as well as `classification.ppm`, so post-processing cannot hide a
solver failure.

## Artifact schema

The manifest schema advances from `gargantua.reference-frame.v1` to
`gargantua.reference-frame.v2`.

`files` records byte size and FNV-1a checksum for:

- `beauty.ppm`;
- `classification.ppm`;
- `rays.csv`.

The manifest records:

- exact Gargantua and Solar provenance;
- observer-to-past and negative-Mino semantics;
- BL cutoff semantics;
- disk configuration and model limitations;
- physical numerical gates;
- disk-hit and crossing totals;
- maximum redshift and intensities over valid samples;
- unconverged and transfer-failure totals;
- fixed display transform and exposure;
- remaining missing capabilities.

Atomic generation behavior remains all-or-nothing. If any write fails, no
final generation directory is committed.

## Failure semantics

Add classifications:

- `DiskSurfaceHit`: a valid opaque disk crossing closed the ray's surface
  transfer;
- `TransferFailure`: Solar rejected material, redshift, intensity, or crossing
  composition.

`Unconverged`, `ConstraintViolation`, `InitializationError`, and
`TransferFailure` count as frame failures. Step exhaustion or near-critical
refusal never becomes escape. BL cutoff remains visibly named
`CapturedAtBlCutoff`.

Serialization rejects:

- non-finite diagnostics on successful rays;
- negative intensity, transmission-derived values, or crossing counts that
  contradict classification;
- disk hits without finite disk radius and redshift;
- no-hit rays with positive disk intensity;
- summaries that disagree with ray evidence;
- unknown enum values;
- stale v1 manifest assumptions.

## Tests

### Focused unit and contract tests

- Camera center direction is future-outward and screen offsets are the exact
  sign inverse of the observer-to-scene vector.
- The Solar tracer advances to negative affine, keeps observer frequency
  positive, and classifies center/corner fixtures.
- A disk ray records finite `g`, radius, temperature, and `g^3`/`g^4`
  intensities.
- An outside-support equatorial crossing continues rather than becoming a disk
  hit or escape.
- Semi-transparent mode records more than one crossing on a selected fixture,
  or fails explicitly if the configured bound is reached.
- Reversing the affine sign breaks the direction regression.
- Renderer summary counts disk and transfer classifications exhaustively.
- Serialization writes both images, complete raw CSV columns, checksums, and
  v2 provenance.
- Malformed disk evidence is rejected before filesystem mutation.
- CLI alias conflicts and every disk boundary are rejected with exit code 2.

### Integration and regression gates

- Clean Release configure/build has no new warnings.
- All Gargantua tests pass against the locked Solar Phase 5 commit.
- A fixed small CLI scene has nonzero capture, escape, and disk populations,
  zero failed rays, and one CSV row per pixel.
- Two complete generations are byte-identical for both PPM files, CSV, and
  manifest.
- The existing finite-distance Schwarzschild shadow bracket remains within
  one pixel after the direction and solver migration.
- A fixed disk ray is cross-checked against direct Solar calls at the localized
  event state.
- AddressSanitizer and UndefinedBehaviorSanitizer run the focused camera,
  path, renderer, and output tests without diagnostics.

## Compatibility and risks

- The file schema intentionally changes to v2; v1 artifacts remain historical
  evidence and are not overwritten.
- The C++ result type grows but remains source-compatible for named member
  access. Aggregate fixtures in this repository must be updated explicitly.
- Separated Mino controls replace affine-step interpretation. Legacy CLI
  spellings remain accepted, but results must be revalidated and the manifest
  identifies the solver.
- The Solar public physics-contract string still reads
  `relativity-v3-phase2`; the exact Solar Git commit is therefore mandatory
  provenance for Phase 5 capability.
- BL cannot cross the horizon or robustly cover the polar axis. Those remain
  explicit missing capabilities rather than display fallbacks.
- PPM is display-referred 8-bit evidence. Raw scientific values remain in CSV;
  OpenEXR/ACES belongs to the later offline film-output phase.

## Acceptance boundary

This slice is complete only when a deterministic disk-bearing CPU reference
generation passes all tests and documents measured errors, checksums, model
limits, and fastest falsification inputs. It does not claim cinema quality.

The next allowed slice is offline high-dynamic-range film infrastructure
(linear spectral/RGB policy, OpenEXR, camera sequence, and CPU/CUDA parity
fixtures), not a web frontend.
