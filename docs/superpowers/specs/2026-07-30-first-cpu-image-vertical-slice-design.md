# Gargantua First CPU Image Vertical Slice Design

## 1. Goal

Build the first deterministic image-producing Gargantua executable. Every
pixel must create a physical local photon through a Solar observer tetrad and
trace that photon with Solar's C++17 `double` Hamiltonian geodesic integrator.

The deliverable is an `ENGINE_DEBUG` Kerr shadow classification image with
per-pixel diagnostics and an immutable JSON manifest. It is a CPU reference
and integration proof, not a film frame.

## 2. Authoritative boundary

Solar remains the authority for:

- the Kerr Boyer-Lindquist metric;
- ZAMO observer construction and tetrad validation;
- local photon initialization;
- Hamiltonian null-geodesic integration;
- event roots, termination reasons, and invariant diagnostics;
- package and physics-contract identifiers.

Gargantua owns:

- validated image and camera settings;
- mapping pixel centers to observer-local directions;
- render work ordering and classification policy;
- diagnostic image, CSV, manifest, checksums, and atomic generation output;
- truthful reporting of missing Solar capabilities.

No Newtonian approximation, black sphere, fixed shadow curve, screen-space
mask, hidden non-finite repair, or artistic effect participates in
classification.

## 3. Approaches considered

### 3.1 Analytical Bardeen raster

Rasterizing Solar's analytical shadow boundary would be fast and useful as an
independent comparison later. It would not prove that Gargantua can construct
and trace one physical ray per pixel, so it is not the vertical slice.

### 3.2 Solar Phase 2 Hamiltonian raster

This is the selected approach. It exercises the real public observer,
initialization, integration, event, invariant, and package contracts. It is
slower and lacks trajectory AOVs or radiation, but those limitations are
visible and become concrete requirements for later Solar phases.

### 3.3 Implement Solar Phase 3 through Phase 5 first

Separated/Mino-time tracing, Kerr-Schild horizon crossing, and covariant
radiative transfer are required before a film renderer. Implementing them in
one batch would violate the phase gates and make failures difficult to
attribute. They remain separate demand-driven Solar changes.

## 4. Layering and dependency direction

The new code follows:

```text
L3 gargantua-render-reference CLI
  -> L2 reference render flow
       -> L1 perspective camera
       -> L1 Solar Kerr ray tracer
       -> L1 atomic reference-output writer
            -> L0 scene, ray result, error, and classification types
```

Only the Solar tracer includes Solar headers. Scene and output code do not
know about CMake dependency resolution or CUDA. The CLI parses arguments and
maps failures to exit codes; it contains no physics rules.

Each source or header remains focused near the repository's 200-line review
signal. A split is made only at the camera, Solar adapter, render flow, or
output-transaction boundary.

## 5. Reference scene contract

`ReferenceScene` is an immutable validated value used by the first renderer:

```cpp
struct ReferenceScene {
    double mass_M;
    double spin_chi;
    double observer_radius_M;
    double inclination_radians;
    double vertical_fov_radians;
    std::size_t width;
    std::size_t height;
    double escape_radius_M;
    double max_affine_M;
    double initial_step_M;
    double max_step_M;
};
```

Validation rejects:

- non-finite numeric values;
- `mass_M <= 0`;
- `abs(spin_chi) >= 1`;
- inclination outside `[1e-3, pi - 1e-3]`;
- vertical field of view outside `[1 degree, 120 degrees]`;
- zero dimensions, dimensions above `4096`, or more than `16,777,216` pixels;
- an observer at or inside `r_+ + 1 M`;
- an escape radius not greater than the observer radius;
- non-positive affine or step limits;
- an initial step larger than the maximum step;
- a maximum affine parameter shorter than the escape-radius distance.

The first CLI supplies explicit options and uses a checked default scene. A
versioned JSON scene-file parser is intentionally deferred until the scene
schema contains camera motion, radiation, and output policy worth preserving.
The manifest is the first machine-readable scene contract.

## 6. Camera and ray convention

The observer is a ZAMO at:

```text
(t, r, theta, phi) = (0, observer_radius_M, inclination_radians, 0)
```

Its tetrad spatial basis follows Solar's public convention:

- local `x`: outward radial;
- local `y`: increasing polar angle;
- local `z`: increasing azimuth.

The camera looks radially inward. For pixel-center normalized device
coordinates `(u, v)` in `[-1, 1]`, the unnormalized local direction is:

```text
(-1,
 -v * tan(vertical_fov / 2),
  u * aspect * tan(vertical_fov / 2))
```

Solar normalizes the direction during local-photon initialization. Rows are
rendered top to bottom and columns left to right. There is one center sample
per pixel and no stochastic sampling, so this slice needs no random seed.

Tests lock the center ray, corner directions, horizontal symmetry for a
Schwarzschild scene, and deterministic row-major ordering.

## 7. Solar tracing policy

For every pixel, `SolarKerrRayTracer`:

1. constructs one Kerr Boyer-Lindquist metric and ZAMO observer per frame;
2. initializes one future-directed null state from the local direction;
3. configures Solar's CPU reference DOPRI5 integrator;
4. monitors energy, axial angular momentum, and Carter's constant;
5. integrates against two terminal events:
   - decreasing `r - capture_radius`;
   - increasing `r - escape_radius`;
6. maps diagnostics into a Gargantua `ReferenceRayResult`.

The capture radius is:

```text
r_capture = r_+ + 1e-3 M
```

This event is an `InteriorCutoff`, not a claimed Boyer-Lindquist horizon
crossing. The margin was established by the radial-ray constraint gate:
`1e-5 M` reached normalized Hamiltonian error `1.0225e-10`. Although the
radial ray passed at `1e-4 M`, a 128 by 128 Schwarzschild raster exposed 22
off-axis rays just above the same gate, with a maximum of `1.0502e-10`.
Reducing the integration step did not remove that near-horizon conditioning
failure. Moving the explicit cutoff to `1e-3 M` made the focused regression
ray pass without weakening the required `1e-10` gate. Gargantua classifies
the event as `captured_at_bl_cutoff`. The escape event uses Solar's `Escaped`
termination reason.

An ordinary ray is accepted only when:

- its termination is the capture cutoff or escape event;
- final state and exported diagnostics are finite where required;
- `max_constraint_error < 1e-10`;
- `max_carter_rel_error < 1e-9`.

All other outcomes remain explicit:

```text
captured_at_bl_cutoff
escaped
unconverged
constraint_violation
initialization_error
```

`MaxAffine`, `MaxSteps`, `StepUnderflow`, event-root failure, invalid metric,
and non-finite state are never reclassified as escape.

## 8. Render result and current AOVs

Each ray records:

```cpp
struct ReferenceRayResult {
    RayClassification classification;
    std::string termination_reason;
    double final_radius_M;
    double max_constraint_error;
    double max_carter_rel_error;
    std::size_t accepted_steps;
    std::size_t rejected_steps;
};
```

The frame summary records counts for every classification plus maxima of
constraint error, Carter relative error, accepted steps, and rejected steps.

The first output generation contains:

```text
classification.ppm
rays.csv
manifest.json
```

The debug palette is fixed:

- captured: black `(0, 0, 0)`;
- escaped: neutral gray `(235, 235, 235)`;
- unconverged: magenta `(255, 0, 255)`;
- constraint violation: orange `(255, 64, 0)`;
- initialization error: yellow `(255, 255, 0)`.

The file is named `classification.ppm`, not `beauty.ppm`, so it cannot be
mistaken for a film render.

`rays.csv` stores pixel coordinates, classification, termination, final
radius, constraint error, Carter error, and step counts. Floating-point values
use 17 significant digits.

## 9. Manifest and output transaction

The manifest records:

- Gargantua version `0.1.0`;
- Solar version and physics contract;
- mode `ENGINE_DEBUG`;
- metric `kerr-bl`, units `G=c=1`, and geodesic kind `null`;
- every scene value;
- capture radius and numerical gates;
- classification counts and diagnostic maxima;
- canonical scene hash;
- FNV-1a 64-bit checksums and byte counts for the PPM and CSV;
- explicit missing capabilities;
- status `complete` or `diagnostic_failed`.

Gargantua commit and dirty-state metadata is refreshed before each build and
compiled into the binary. It must not be a configure-time-only snapshot,
because a later commit or source edit would otherwise produce a plausible but
stale manifest.

FNV-1a is named in the manifest and is only an accidental-corruption and
determinism checksum, not a cryptographic authenticity claim.

The output is one generation directory. Rendering writes all files into the
sibling `<output>.part` directory, closes and checks every stream, then
renames that directory to `<output>`. The final directory and part directory
must both be absent at start. The parent must exist. The manifest is written
last inside the part directory. A failed render keeps diagnostic output only
when all files can be finalized; otherwise the part directory remains visible
and the command returns nonzero.

Before serialization, the writer recomputes classification counts, failure
counts, diagnostic maxima, and step maxima from the retained ray vector. It
rejects unknown classifications or status values, non-finite tracer metadata,
and any caller-supplied summary that differs from the derived evidence.

## 10. CLI contract

The executable is:

```sh
gargantua-render-reference \
  --output reference-frame \
  --mass-M 1 \
  --spin 0.5 \
  --observer-r-M 30 \
  --inclination-deg 85 \
  --fov-y-deg 40 \
  --width 64 \
  --height 36 \
  --escape-r-M 60 \
  --max-affine-M 200 \
  --initial-step-M 0.02 \
  --max-step-M 0.25
```

Unknown, repeated, missing, malformed, non-finite, and out-of-range options
return nonzero with one actionable error. `--help` returns zero without
constructing Solar objects or creating output.

Exit codes:

```text
0  complete frame with no failed rays
2  invalid CLI or scene
3  renderer initialization failure
4  frame contains unconverged, constraint, or initialization failures
5  output transaction failure
```

## 11. Verification

Focused unit and integration tests cover:

- all scene validation boundaries;
- camera center/corners, orientation, and finite output;
- center capture and off-axis escape in Schwarzschild and Kerr scenes;
- invariant gates and explicit unknown/failure mappings;
- deterministic repeated rendering;
- exact PPM dimensions and palette;
- CSV row count and header;
- manifest contract fields, counts, missing-capability list, and checksums;
- build provenance across clean, dirty, and advanced Git states;
- output-directory collision and atomic part-directory behavior;
- CLI success, help, malformed option, and failure exit codes.

The numerical validation report compares a small Schwarzschild raster against
the expected circular symmetry and Solar's analytical critical impact
parameter `3 sqrt(3) M`. Finite observer distance and pixelization error are
reported separately. Kerr classification is compared against Solar's
analytical Bardeen boundary only as a coarse far-observer regression.

The first acceptance render is intentionally small enough for routine CPU
verification. A larger image may be generated only after the focused suite is
green.

## 12. Missing capabilities and Solar feedback

This slice explicitly does not provide:

- `min_radius` over the full trajectory;
- azimuthal winding;
- trajectory samples or dense-output callbacks;
- disk crossings or multiple intersections;
- redshift or invariant radiative transfer;
- Kerr-Schild horizon crossing;
- Boyer-Lindquist rays that intersect the polar coordinate axis;
- separated/Mino-time high-throughput tracing;
- CUDA, OpenEXR, ACES, animation, or a beauty image.

The first reusable Solar demand is a bounded trajectory-diagnostics observer
or ray-result API that can report minimum radius, winding, and selected event
history without exposing Gargantua types or retaining every integration step.
That demand will receive its own Solar spec and validation gate after this
image slice proves the current public API end to end.

## 13. Acceptance gate

The slice passes only when:

1. all focused Gargantua tests pass from a clean build;
2. the existing locked Solar external call still passes;
3. a fresh reference render produces all three files atomically;
4. repeated renders have identical PPM and CSV checksums;
5. the accepted reference frame contains captured and escaped pixels;
6. failed rays are zero, or the command returns exit code `4` and the manifest
   says `diagnostic_failed`;
7. the validation report contains actual commands, output, numerical error,
   limitations, and fastest falsification;
8. no document claims disk radiation, horizon crossing, CUDA, or film quality.
