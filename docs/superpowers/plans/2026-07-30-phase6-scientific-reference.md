# Phase 6 Scientific Reference Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a deterministic Solar Phase 5-backed CPU Kerr thin-disk reference frame with correct observer-to-past propagation, raw per-ray physics evidence, and an auditable grayscale beauty image.

**Architecture:** Gargantua keeps its existing `ReferenceRayTracer` boundary, but splits the Solar adapter into construction, separated-path orchestration, and evidence validation. Solar owns every metric/geodesic/redshift/disk calculation; Gargantua owns scene validation, event continuation policy, frame traversal, display encoding, and atomic artifacts.

**Tech Stack:** C++17, CMake 3.20+, Solar `Solar::Relativity` at locked merge commit `635d99f47fa50be892416986f2723d035ee2acc1`, PPM/CSV/JSON zero-runtime-dependency artifacts, CTest, AppleClang/GCC, ASan/UBSan.

## Global Constraints

- CPU C++17 `double` is the reference physics path.
- Signature is `(-,+,+,+)` and geometric units are `G=c=1`.
- Camera photons remain future-directed and integrate observer-to-past with negative Mino parameter.
- Capture remains the explicit Boyer-Lindquist cutoff `r_+ + 1e-3 M`; it is not a horizon-crossing claim.
- Disk fluid, redshift, and surface transfer must come from Solar Phase 5 public APIs.
- Raw redshift and linear intensity must never be replaced by display-mapped values.
- Failed, non-finite, near-critical-refused, or step-exhausted rays must never be classified as escape.
- No CUDA, WebGL, WASM, OpenEXR, ACES, film look, UI, or volume transfer in this plan.
- Preserve existing CLI spellings as aliases; reject ambiguous alias combinations.
- Every filesystem generation is all-or-nothing.
- Do not modify or remove the untracked root `.DS_Store`.

---

## File map and dependency direction

### L0 contracts

- Modify `include/gargantua/reference/reference_scene.h`: scientific disk scene values and opacity enum.
- Modify `include/gargantua/reference/reference_ray.h`: physical classification and per-ray evidence.
- Modify `include/gargantua/reference/reference_frame.h`: disk/transfer summary fields.
- Modify `include/gargantua/reference/reference_output.h`: three artifact checksums.
- Modify `include/gargantua/reference/reference_numerics.h`: crossing-direction and display constants only.

### L1 building blocks

- Modify `src/perspective_camera.cpp`: future-arriving observer photon direction.
- Create `src/reference_ray_evidence.h`.
- Create `src/reference_ray_evidence.cpp`: evidence constructors and structural validation.
- Create `src/solar_kerr_path.h`.
- Create `src/solar_kerr_path.cpp`: negative-Mino segmented path and disk event continuation.
- Modify `src/solar_kerr_ray_tracer.cpp`: object construction and delegation only.

### L2 flow

- Modify `src/reference_frame_summary.h`: exhaustive evidence aggregation.
- Modify `src/reference_frame.cpp`: enum/status names.
- Modify `src/reference_renderer.cpp`: retain row-major orchestration and call evidence validation.

### L3 diplomacy

- Modify `cli/reference_render_options.h`.
- Modify `cli/reference_render_options.cpp`: disk and canonical Mino options plus legacy aliases.
- Modify `cli/reference_render_main.cpp`: report all artifact checksums and disk counts.
- Modify `src/reference_serialization.h`.
- Modify `src/reference_serialization.cpp`: beauty/classification PPM and expanded CSV.
- Modify `src/reference_manifest.h`.
- Modify `src/reference_manifest.cpp`: v2 schema and capability/provenance evidence.
- Modify `src/reference_output.cpp`: atomic four-file generation with cleanup.
- Modify `CMakeLists.txt`: new sources and focused test.
- Modify `cmake/solar-lock.cmake`: published Solar Phase 5 merge commit.

Dependency direction remains `CLI/output L3 -> renderer/path L2 -> focused L1 -> contracts L0`. Solar is an external L1 capability dependency and never depends on Gargantua.

---

### Task 1: Prove and lock the Solar Phase 5 consumer

**Files:**
- Modify: `src/probe_main.cpp`
- Modify: `cmake/solar-lock.cmake`
- Modify: `docs/validation/00_solar_consumer.md`

**Interfaces:**
- Consumes: `Solar::Relativity`, `AnalyticCircularDiskFluid`, `AnalyticOpticallyThinTorus`, `ThinDiskCrossingRecorder`, and `advance_backward_transfer`.
- Produces: a CTest probe JSON object containing `transfer_intensity`, `disk_temperature`, `torus_density`, and `surface_crossings`.

- [x] **Step 1: Extend the consumer probe before changing the lock**

Add the Phase 5 public headers and a compact execution path to
`src/probe_main.cpp`:

```cpp
#include "solar/relativity/emission_model.h"
#include "solar/relativity/fluid_model.h"
#include "solar/relativity/radiative_transfer.h"
#include "solar/relativity/thin_disk.h"

const TransferAdvanceResult transfer = advance_backward_transfer(
    BackwardTransferState{},
    TransferCoefficients{2.0, 0.5},
    3.0);
if (!transfer) {
    throw std::runtime_error(transfer.message);
}
```

Construct one valid disk sample, one torus sample, and one disk crossing using
the same finite fixtures already accepted by Solar's installed consumer.
Require every result to succeed before printing JSON.

- [x] **Step 2: Build to verify the old lock is red**

Run:

```sh
cmake --build build --parallel 4
```

Expected: compilation fails because the locked pre-Phase 5 Solar commit does
not contain one or more new public headers.

- [x] **Step 3: Move the exact dependency lock**

Set:

```cmake
set(GARGANTUA_SOLAR_COMMIT "635d99f47fa50be892416986f2723d035ee2acc1")
set(GARGANTUA_SOLAR_VERSION "0.2.0-alpha.1")
set(GARGANTUA_SOLAR_PHYSICS_CONTRACT "relativity-v3-phase2")
```

Delete only the worktree-local `build` directory through CMake's clean
reconfigure path or configure a fresh `build-phase5` directory. Do not alter a
Solar checkout to make it match the lock.

- [x] **Step 4: Configure, build, and run the focused probe**

Run:

```sh
cmake -S . -B build-phase5 -DCMAKE_BUILD_TYPE=Release
cmake --build build-phase5 --parallel 4
ctest --test-dir build-phase5 -R gargantua.solar_probe --output-on-failure
./build-phase5/gargantua-probe
```

Expected: CTest passes and JSON contains finite Phase 5 fields with
`surface_crossings:1`.

- [x] **Step 5: Record the consumer evidence**

Update `docs/validation/00_solar_consumer.md` with the exact commit, compiler,
commands, JSON, model boundary, and the fact that the unchanged
`relativity-v3-phase2` contract string makes the Git commit mandatory
provenance.

- [x] **Step 6: Commit the dependency gate**

```sh
git add cmake/solar-lock.cmake src/probe_main.cpp \
  docs/validation/00_solar_consumer.md
git commit -m "build: lock Solar Phase 5 transfer API"
```

---

### Task 2: Correct observer-to-past camera and evidence contracts

**Files:**
- Modify: `include/gargantua/reference/reference_scene.h`
- Modify: `include/gargantua/reference/reference_ray.h`
- Modify: `include/gargantua/reference/reference_frame.h`
- Modify: `include/gargantua/reference/reference_numerics.h`
- Modify: `src/reference_scene.cpp`
- Modify: `src/perspective_camera.cpp`
- Modify: `src/reference_frame.cpp`
- Modify: `src/reference_ray.cpp`
- Modify: `src/reference_serialization.cpp`
- Modify: `src/solar_kerr_ray_tracer.cpp`
- Modify: `tests/test_scene_camera.cpp`
- Modify: `tests/test_solar_kerr_ray_tracer.cpp`
- Modify: `tests/test_reference_renderer.cpp`
- Modify: `tests/test_reference_output.cpp`

**Interfaces:**
- Produces:

```cpp
enum class ReferenceDiskOpacity {
    Opaque,
    SemiTransparent,
};

struct ScientificDiskScene {
    double outer_radius_M = 20.0;
    double density_scale = 1.0;
    double temperature_scale = 1.0;
    double density_power = 0.75;
    double specific_intensity_scale = 1.0;
    double bolometric_intensity_scale = 1.0;
    ReferenceDiskOpacity opacity = ReferenceDiskOpacity::Opaque;
    double surface_optical_depth = 1.0;
    std::size_t max_crossings = 8;
    double display_exposure = 1.0;
};
```

`ReferenceScene` appends `ScientificDiskScene disk{}` after the existing step
fields.

`ReferenceRayResult` becomes:

```cpp
struct ReferenceRayResult {
    RayClassification classification;
    std::string termination_reason;
    double final_affine_M;
    double final_radius_M;
    double min_radius_M;
    double winding;
    double max_constraint_error;
    double max_energy_rel_error;
    double max_lz_rel_error;
    double max_carter_rel_error;
    std::size_t accepted_steps;
    std::size_t rejected_steps;
    double disk_radius_M;
    double redshift_g;
    double observed_temperature;
    double observed_specific_intensity;
    double observed_bolometric_intensity;
    std::size_t disk_crossings;
};
```

`ReferenceFrameSummary` appends:

```cpp
std::size_t disk_surface_hits = 0;
std::size_t transfer_failures = 0;
std::size_t disk_crossings = 0;
double max_redshift_g = 0.0;
double max_observed_specific_intensity = 0.0;
double max_observed_bolometric_intensity = 0.0;
```

- [x] **Step 1: Add the camera direction regression**

In `tests/test_scene_camera.cpp`, assert the center ray and one offset:

```cpp
const CameraRay center = perspective_camera_ray(scene, 0, 0);
check("future photon points outward at observer",
      center.local_direction[0] == 1.0);

check("future photon is the negative observer-to-scene direction",
      offset.local_direction[0] == 1.0 &&
      offset.local_direction[1] == -old_scene_direction_y &&
      offset.local_direction[2] == -old_scene_direction_z);
```

Use exact expected values computed from the existing pixel-center formula, not
a self-referential call to production code.

- [x] **Step 2: Run the camera test to verify red**

```sh
cmake --build build-phase5 --target test-scene-camera --parallel 4
./build-phase5/test-scene-camera
```

Expected: the future-outward direction assertion fails.

- [x] **Step 3: Reverse all local spatial camera components**

Change `perspective_camera_ray` to:

```cpp
return CameraRay{
    pixel_x,
    pixel_y,
    std::array<double, 3>{{
        1.0,
        normalized_y * tangent_half_fov,
        -normalized_x * aspect * tangent_half_fov,
    }},
};
```

Until Task 4 replaces the generic solver, pass
`-scene.initial_step_M` to its CPU-reference configuration. Add center and
corner assertions that every advanced final affine value is negative. This
keeps the vertical slice physically consistent between commits.

- [x] **Step 4: Add disk/result contracts and exhaustive names**

Add `DiskSurfaceHit` and `TransferFailure` to `RayClassification`. Update
`ray_classification_name` and `is_failed_classification` so disk hits succeed
and transfer failures fail. Add disk and physical maxima/counts to
`ReferenceFrameSummary`.

Update all aggregate fixtures with explicit finite/NaN evidence consistent
with their classification. Use a local fixture factory rather than repeating
18 positional values in every test.

- [x] **Step 5: Add scene boundary tests**

Cover:

- non-finite disk scale;
- disk outer radius at or below the horizon;
- zero/oversized crossing count;
- non-positive exposure;
- negative or non-finite optical depth;
- unknown opacity enum.

Run:

```sh
cmake --build build-phase5 --target test-scene-camera \
  test-reference-renderer test-reference-output --parallel 4
./build-phase5/test-scene-camera
./build-phase5/test-reference-renderer
./build-phase5/test-reference-output
```

Expected: all focused tests pass after fixture updates.

- [x] **Step 6: Commit L0 contracts and direction**

```sh
git add include/gargantua/reference/reference_scene.h \
  include/gargantua/reference/reference_ray.h \
  include/gargantua/reference/reference_frame.h \
  include/gargantua/reference/reference_numerics.h \
  src/reference_scene.cpp src/perspective_camera.cpp src/reference_ray.cpp \
  src/solar_kerr_ray_tracer.cpp \
  tests/test_scene_camera.cpp tests/test_reference_renderer.cpp \
  tests/test_reference_output.cpp tests/test_solar_kerr_ray_tracer.cpp
git commit -m "fix: define observer-to-past reference rays"
```

---

### Task 3: Add structural ray-evidence validation

**Files:**
- Create: `src/reference_ray_evidence.h`
- Create: `src/reference_ray_evidence.cpp`
- Modify: `src/reference_frame_summary.h`
- Modify: `src/reference_renderer.cpp`
- Modify: `CMakeLists.txt`
- Create: `tests/test_reference_ray_evidence.cpp`

**Interfaces:**
- Produces:

```cpp
namespace gargantua::reference::detail {

ReferenceRayResult unavailable_reference_ray(
    RayClassification classification,
    std::string reason);

bool valid_reference_ray_evidence(
    const ReferenceRayResult& ray) noexcept;

} // namespace gargantua::reference::detail
```

- [x] **Step 1: Write classification/evidence truth-table tests**

Create fixtures for:

- escaped no-hit ray with zero intensity;
- BL-captured no-hit ray with negative final affine;
- opaque disk hit with one crossing and finite disk values;
- semi-transparent escape with two crossings and positive intensity;
- transfer failure with unavailable physical fields.

Mutate each accepted fixture one field at a time:

```cpp
ReferenceRayResult invalid = disk_hit;
invalid.redshift_g = std::numeric_limits<double>::quiet_NaN();
check("disk hit requires finite redshift",
      !valid_reference_ray_evidence(invalid));

invalid = escaped;
invalid.observed_bolometric_intensity = 1.0;
check("no-hit ray cannot carry disk intensity",
      !valid_reference_ray_evidence(invalid));
```

- [x] **Step 2: Register and run the test red**

Add `test-reference-ray-evidence` to CMake, give only that test target a
private `${CMAKE_CURRENT_SOURCE_DIR}/src` include directory, and run:

```sh
cmake --build build-phase5 --target test-reference-ray-evidence --parallel 4
```

Expected: link or compile failure because the validation functions do not
exist.

- [x] **Step 3: Implement evidence construction and validation**

Validation rules:

```cpp
const bool advanced =
    std::isfinite(ray.final_affine_M) && ray.final_affine_M < 0.0;
const bool diagnostics_finite =
    std::isfinite(ray.final_radius_M) &&
    std::isfinite(ray.min_radius_M) &&
    std::isfinite(ray.winding) &&
    std::isfinite(ray.max_constraint_error) &&
    std::isfinite(ray.max_energy_rel_error) &&
    std::isfinite(ray.max_lz_rel_error) &&
    std::isfinite(ray.max_carter_rel_error);
```

Successful classifications require `advanced`, finite diagnostics, and all
v3 invariant gates. Disk evidence requires positive finite `g`, finite disk
radius, non-negative finite temperature/intensities, and
`disk_crossings > 0`. No-hit success requires zero intensities/count and NaN
disk-only coordinates. Failure classifications may carry unavailable values
but cannot carry negative finite intensities.

- [x] **Step 4: Make renderer and summary use the validator**

`render_reference_frame` validates every returned ray before retaining it.
`summarize_reference_rays` must reject unknown enums and contradictory
evidence, then aggregate every new classification and maximum.

- [x] **Step 5: Run focused tests**

```sh
cmake --build build-phase5 --target test-reference-ray-evidence \
  test-reference-renderer --parallel 4
./build-phase5/test-reference-ray-evidence
./build-phase5/test-reference-renderer
```

Expected: all assertions pass.

- [x] **Step 6: Commit the evidence boundary**

```sh
git add CMakeLists.txt src/reference_ray_evidence.h \
  src/reference_ray_evidence.cpp src/reference_frame_summary.h \
  src/reference_renderer.cpp tests/test_reference_ray_evidence.cpp \
  tests/test_reference_renderer.cpp
git commit -m "test: enforce physical ray evidence"
```

---

### Task 4: Implement negative-Mino segmented disk paths

**Files:**
- Create: `src/solar_kerr_path.h`
- Create: `src/solar_kerr_path.cpp`
- Create: `src/solar_kerr_path_result.h`
- Create: `src/solar_kerr_path_result.cpp`
- Create: `src/solar_kerr_path_setup.h`
- Create: `src/solar_kerr_path_setup.cpp`
- Create: `src/solar_thin_disk.h`
- Create: `src/solar_thin_disk.cpp`
- Modify: `src/solar_kerr_ray_tracer.cpp`
- Modify: `CMakeLists.txt`
- Create: `tests/test_solar_kerr_path.cpp`
- Modify: `tests/test_solar_kerr_ray_tracer.cpp`

**Interfaces:**
- Consumes: `KerrSeparatedIntegrator`, `ThinDiskCrossingRecorder`,
  `initialize_local_photon`, `evaluate_kerr_constants`.
- Produces:

```cpp
struct SolarKerrPathTrace {
    ReferenceRayResult ray;
    std::size_t ignored_surface_crossings = 0;
};

SolarKerrPathTrace trace_solar_kerr_path(
    const solar::relativity::KerrBoyerLindquistMetric& metric,
    const solar::relativity::ObserverFrame& observer,
    const ReferenceScene& scene,
    const CameraRay& camera_ray);
```

`SolarKerrPathTrace` is private to `src/` and exists so focused tests can
verify continuation through outside-support equatorial roots. The public
`ReferenceRayTracer` returns only its `ray`.

- [x] **Step 1: Write a negative-affine integration test**

For the center camera ray, require:

```cpp
const ReferenceRayResult center =
    trace_solar_kerr_path(metric, observer, scene, center_ray);
check("center ray advances observer-to-past",
      center.final_affine_M < 0.0);
check("center ray does not fabricate escape",
      center.classification != RayClassification::Escaped);
```

Add an ordinary corner ray that escapes with finite min radius and winding.

- [x] **Step 2: Write disk and continuation tests**

Use `spin_chi=0.5`, `observer_radius_M=30`, inclination `85 degrees`,
`disk.outer_radius_M=20`, and the existing camera FOV. Search the fixed
`64 x 36` pixel-center grid in row-major order and require:

- at least one opaque `DiskSurfaceHit`;
- at least one captured ray;
- at least one escaped ray;
- every disk hit has finite positive `g`;
- at least one ray has an equatorial crossing outside disk support before its
  accepted final classification.

Also construct a spin-zero scene with disk outer radius `3M`: it is outside
the `2M` horizon but inside the `6M` ISCO. Require the Solar tracer factory to
reject it, proving Gargantua does not copy the ISCO formula into L0 validation.

Require `SolarKerrPathTrace::ignored_surface_crossings > 0` for at least one
accepted ray, proving continuation without changing the public physical
result.

For semi-transparent mode with optical depth `0.5`, scan the same grid and
require at least one successful ray with `disk_crossings >= 2`. The scan is a
deterministic acceptance search, not random fixture discovery.

- [x] **Step 3: Register the new test and verify red**

Give `test-solar-kerr-path` a private
`${CMAKE_CURRENT_SOURCE_DIR}/src` include directory, then run:

```sh
cmake --build build-phase5 --target test-solar-kerr-path --parallel 4
```

Expected: compile failure because `solar_kerr_path.h` does not exist.

- [x] **Step 4: Construct Solar disk and event objects**

Map `ScientificDiskScene` exactly:

```cpp
AnalyticCircularDiskFluid disk(AnalyticCircularDiskConfig{
    scene.mass_M,
    scene.spin_chi,
    OrbitSense::Prograde,
    std::nullopt,
    scene.disk.outer_radius_M,
    scene.disk.density_scale,
    scene.disk.temperature_scale,
    scene.disk.density_power,
    1.0e-8,
});

ThinDiskCrossingRecorder recorder(
    ThinDiskRecorderConfig{
        scene.disk.opacity == ReferenceDiskOpacity::Opaque
            ? DiskOpacityMode::Opaque
            : DiskOpacityMode::SemiTransparent,
        scene.disk.max_crossings,
    },
    std::move(disk),
    ThinDiskSurfaceEmission{
        scene.disk.specific_intensity_scale,
        scene.disk.bolometric_intensity_scale,
        scene.disk.surface_optical_depth,
    });
```

Build capture, escape, and equatorial events with root tolerance
`1e-10 * mass_M`.

- [x] **Step 5: Implement the segment loop**

Initialize one future-directed observer photon, then use:

```cpp
KerrSeparatedConfig config =
    KerrSeparatedConfig::cpu_reference(
        GeodesicKind::Null,
        scene.mass_M,
        -scene.initial_step_M,
        scene.max_step_M,
        remaining_affine);
```

For each segment:

1. Integrate with the current disk direction.
2. Aggregate diagnostics and subtract
   `abs(final.affine - segment_initial.affine)` from the total budget.
3. Capture/escape ends the path.
4. A disk event records the exact `EventHit::state`.
5. Closed opaque transfer returns `DiskSurfaceHit`.
6. Open or outside-support crossings switch to the opposite direction:

```cpp
const double directed_change =
    std::copysign(1.0, config.initial_mino_step) *
    crossing_state.p.v[2];
next_direction = directed_change > 0.0
    ? EventDirection::Decreasing
    : EventDirection::Increasing;
```

Reject non-finite or near-zero `directed_change`; do not nudge the event state.
Stop explicitly when remaining affine is exhausted or the recorder reports a
transfer/crossing-limit error.

- [x] **Step 6: Recompute final invariant drift**

Evaluate final `E`, `Lz`, and `Q` with Solar. Use v3 denominators
`max(1, abs(initial))`, preserve the maximum Carter diagnostic reported by
segments, and validate all results before returning success.

- [x] **Step 7: Reduce the public tracer to construction/delegation**

`SolarKerrRayTracer::trace` becomes:

```cpp
return trace_solar_kerr_path(
    metric_, observer_, scene_, ray).ray;
```

The object stores a copy of `ReferenceScene`; generic integrator/event members
leave this file. Keep `ReferenceTracerInfo` construction and the BL cutoff
description here.

- [x] **Step 8: Run path and tracer tests**

```sh
cmake --build build-phase5 --target test-solar-kerr-path \
  test-solar-kerr-ray-tracer --parallel 4
./build-phase5/test-solar-kerr-path
./build-phase5/test-solar-kerr-ray-tracer
```

Expected: all assertions pass, with nonzero capture/escape/disk populations and
at least one multi-crossing semi-transparent ray.

- [x] **Step 9: Run two mutation checks**

Mutation A: make the Mino step positive. Rebuild and require the
negative-affine test to fail.

Mutation B: retain the same polar event direction after a crossing. Rebuild
and require the outside-support/multi-crossing test to fail through duplicate
endpoint detection or crossing exhaustion.

Restore both mutations with `apply_patch`, rebuild, and require green.

- [x] **Step 10: Commit the Solar path**

```sh
git add CMakeLists.txt src/solar_kerr_path.h src/solar_kerr_path.cpp \
  src/solar_kerr_ray_tracer.cpp tests/test_solar_kerr_path.cpp \
  tests/test_solar_kerr_ray_tracer.cpp
git commit -m "feat: trace Solar thin-disk reference rays"
```

---

### Task 5: Serialize raw evidence and deterministic beauty

**Files:**
- Modify: `include/gargantua/reference/reference_output.h`
- Modify: `src/reference_serialization.h`
- Modify: `src/reference_serialization.cpp`
- Modify: `src/reference_manifest.h`
- Modify: `src/reference_manifest.cpp`
- Modify: `src/reference_output.cpp`
- Modify: `tests/test_reference_output.cpp`

**Interfaces:**
- Produces:

```cpp
struct SerializedReferenceGeneration {
    bool valid = false;
    std::string message;
    std::vector<unsigned char> beauty_ppm;
    std::vector<unsigned char> classification_ppm;
    std::string csv;
    std::string manifest;
    std::uint64_t beauty_ppm_checksum = 0;
    std::uint64_t classification_ppm_checksum = 0;
    std::uint64_t csv_checksum = 0;
};
```

- [x] **Step 1: Add output tests for v2 artifacts**

Require:

- both PPM files exist and have exact dimensions;
- disk beauty pixels match a hand-computed scientific transform;
- failed beauty pixels retain the diagnostic color;
- CSV contains every new raw field;
- manifest schema is v2 and has three checksummed files;
- disk/redshift/separated/beauty are absent from `missing_capabilities`;
- invalid evidence creates neither a final nor `.part` directory;
- an existing `.part` directory is rejected without modifying its contents.

The transform reference uses:

```cpp
double linear_to_srgb(double value) {
    return value <= 0.0031308
        ? 12.92 * value
        : 1.055 * std::pow(value, 1.0 / 2.4) - 0.055;
}
```

- [x] **Step 2: Run the output test red**

```sh
cmake --build build-phase5 --target test-reference-output --parallel 4
./build-phase5/test-reference-output
```

Expected: assertions fail because `beauty.ppm` and v2 fields are absent.

- [x] **Step 3: Implement separate image encoders**

Keep classification colors unchanged and add:

```cpp
double mapped = linear / (1.0 + linear);
double srgb = linear_to_srgb(std::clamp(mapped, 0.0, 1.0));
unsigned char channel = static_cast<unsigned char>(
    std::lround(255.0 * srgb));
```

Use `scene.disk.display_exposure * observed_bolometric_intensity`. Emit the
same channel for RGB. Successful no-emission rays are black; failed rays use
their classification color. The fixed classification palette adds cyan
`(0,200,255)` for disk hits and red `(255,0,0)` for transfer failures.

- [x] **Step 4: Expand canonical CSV and scene hash**

Append exact columns:

```text
final_affine_M,min_radius_M,winding,disk_radius_M,redshift_g,
observed_temperature,observed_specific_intensity,
observed_bolometric_intensity,disk_crossings
```

Add every disk/display scene value to the canonical scene string so a physics
or display change changes the scene hash.

- [x] **Step 5: Advance the manifest to v2**

Record:

- solver `kerr-separated-mino`;
- direction `observer-to-past`;
- photon orientation `future-directed`;
- surface model and opacity;
- summary disk/transfer counts and maxima;
- display transform formula identifier `reinhard-srgb-v1`;
- all three file byte counts/checksums;
- remaining limitations only.

- [x] **Step 6: Make output cleanup reliable**

Use a scoped partial-generation guard:

```cpp
class PartialGenerationGuard {
public:
    explicit PartialGenerationGuard(std::filesystem::path path)
        : path_(std::move(path)) {}
    ~PartialGenerationGuard() {
        if (!committed_) {
            std::error_code ignored;
            std::filesystem::remove_all(path_, ignored);
        }
    }
    void commit() noexcept { committed_ = true; }
private:
    std::filesystem::path path_;
    bool committed_ = false;
};
```

Write beauty, classification, CSV, then manifest. Rename only after every
close succeeds, then mark the guard committed.

Mid-write filesystem failures are guarded by the scoped cleanup but are not
fault-injected in this slice; the validation report must retain this as an
unverified recovery branch.

- [x] **Step 7: Run output and renderer tests**

```sh
cmake --build build-phase5 --target test-reference-output \
  test-reference-renderer --parallel 4
./build-phase5/test-reference-output
./build-phase5/test-reference-renderer
```

Expected: all assertions pass.

- [x] **Step 8: Commit artifact schema v2**

```sh
git add include/gargantua/reference/reference_output.h \
  src/reference_serialization.h src/reference_serialization.cpp \
  src/reference_manifest.h src/reference_manifest.cpp \
  src/reference_output.cpp tests/test_reference_output.cpp
git commit -m "feat: write scientific reference beauty"
```

---

### Task 6: Expose disk and Mino controls through the CLI

**Files:**
- Modify: `cli/reference_render_options.h`
- Modify: `cli/reference_render_options.cpp`
- Modify: `cli/reference_render_main.cpp`
- Modify: `tests/test_reference_render_options.cpp`
- Modify: `tests/test_reference_render_cli.sh`

**Interfaces:**
- Consumes: validated `ReferenceScene`.
- Produces: canonical options listed in the design plus legacy step aliases.

- [x] **Step 1: Add parser red tests**

Add successful parsing for all disk values and each opacity. Add failures for:

- unknown opacity;
- repeated canonical option;
- canonical/legacy step alias conflict;
- zero crossing count;
- negative/infinite semi-transparent depth;
- non-positive exposure;
- syntactically valid disk radii are deferred to the Solar adapter, which
  rejects Kerr-domain violations before rendering.

Example alias conflict:

```cpp
const auto parsed = parse_reference_render_options({
    "renderer", "--output", "frame",
    "--initial-mino-step", "0.02",
    "--initial-step-M", "0.02",
});
check("canonical and legacy step aliases conflict", !parsed);
```

- [x] **Step 2: Run parser tests red**

```sh
cmake --build build-phase5 --target test-reference-render-options --parallel 4
./build-phase5/test-reference-render-options
```

Expected: new option assertions fail.

- [x] **Step 3: Parse options into scene values**

Canonical `--initial-mino-step` and `--max-mino-step` map to the existing
positive magnitude scene fields. Legacy `--initial-step-M` and
`--max-step-M` map identically but conflict with their canonical counterpart.
Framework-light validation remains in `validate_reference_scene`; Kerr-domain
validation remains in the Solar adapter so Gargantua does not copy GR
formulae.

- [x] **Step 4: Update help and CLI JSON**

Help must state:

```text
CPU reference uses future-directed photons integrated observer-to-past.
The renderer is a scientific thin-disk model, not GRMHD or film look.
```

CLI JSON adds disk hits, crossings, beauty checksum, classification checksum,
and CSV checksum.

- [x] **Step 5: Extend the shell integration**

The fixed `10 x 8` scene must write four files, contain nonzero capture,
escape, and disk populations, contain zero failures, and expose 80 CSV rows.
Retain duplicate-output exit code 5 and diagnostic-frame exit code 4.

- [x] **Step 6: Run CLI tests**

```sh
cmake --build build-phase5 --target test-reference-render-options \
  gargantua-render-reference --parallel 4
ctest --test-dir build-phase5 -R \
  'gargantua.reference_render_(options|cli)' --output-on-failure
```

Expected: both tests pass.

- [x] **Step 7: Commit CLI contract**

```sh
git add cli/reference_render_options.h cli/reference_render_options.cpp \
  cli/reference_render_main.cpp tests/test_reference_render_options.cpp \
  tests/test_reference_render_cli.sh
git commit -m "feat: configure scientific disk references"
```

---

### Task 7: Produce and independently check the Phase 6 reference artifact

**Files:**
- Modify: `README.md`
- Create: `docs/validation/02_scientific_disk_reference.md`
- Create: `tests/fixtures/scientific_reference_v2.json`

**Interfaces:**
- Produces: a pinned compact fixture containing scene parameters, counts,
  numerical maxima, and checksums; it does not contain a copied binary image.

- [x] **Step 1: Render two clean generations**

From a clean committed worktree:

```sh
./build-phase5/gargantua-render-reference \
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

./build-phase5/gargantua-render-reference \
  --output artifacts/scientific-reference-v2-repeat \
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

- [x] **Step 2: Require byte identity**

```sh
cmp artifacts/scientific-reference-v2/beauty.ppm \
  artifacts/scientific-reference-v2-repeat/beauty.ppm
cmp artifacts/scientific-reference-v2/classification.ppm \
  artifacts/scientific-reference-v2-repeat/classification.ppm
cmp artifacts/scientific-reference-v2/rays.csv \
  artifacts/scientific-reference-v2-repeat/rays.csv
cmp artifacts/scientific-reference-v2/manifest.json \
  artifacts/scientific-reference-v2-repeat/manifest.json
```

Expected: every comparison exits 0.

- [x] **Step 3: Run physical accounting checks**

Use a small read-only shell/AWK check:

- population sum equals `2304`;
- capture, escape, and disk hit are each nonzero;
- failed count is zero;
- every disk hit has `g > 0`, non-negative intensity, and at least one
  crossing;
- at least one disk-bearing ray has strictly positive intensity;
- every accepted max invariant is below its declared gate;
- every advanced final affine value is negative.

- [x] **Step 4: Retain the Schwarzschild boundary check**

Render the existing `64 x 64`, spin-zero, inclination-90 fixture with a disk
outer radius set below the analytic shadow-screen comparison region. Derive
the captured bounding box from classification, and require the analytic local
shadow radius to remain inside the one-pixel bracket.

- [x] **Step 5: Inspect the generated beauty**

Convert PPM to a temporary PNG only if a locally available zero-loss tool can
do so, then inspect it. Verify:

- failed-pixel colors are absent;
- the disk is visible and lensed;
- the classification image contains the expected shadow;
- no display claim exceeds raw evidence.

The PNG is inspection-only and is not the scientific artifact.

- [x] **Step 6: Write fixture and validation report**

`tests/fixtures/scientific_reference_v2.json` records exact inputs, Solar and
Gargantua commits, population counts, maxima, and checksums from the accepted
generation.

`docs/validation/02_scientific_disk_reference.md` follows the v3 validation
template: claim, model boundary, references, commands, inputs, expected,
actual, error, result, limitations, and fastest falsification.

- [x] **Step 7: Update README without cinema claims**

Document the new four-file generation, scientific grayscale transform,
observer-to-past convention, Phase 5 lock, and explicit missing film
capabilities.

- [x] **Step 8: Commit accepted evidence**

```sh
git add README.md docs/validation/02_scientific_disk_reference.md \
  tests/fixtures/scientific_reference_v2.json
git commit -m "docs: validate scientific disk reference"
```

---

### Task 8: Full verification, sanitizers, audit, and publication

**Files:**
- Create: `.github/workflows/ci.yml`
- Modify: `README.md` to append the final supported command and CI platform.
- Modify: `docs/validation/02_scientific_disk_reference.md` to append exact
  clean Release, sanitizer, and Linux CI evidence.

**Interfaces:**
- Produces: a reviewable public branch and PR with local and Linux evidence.

- [x] **Step 1: Add the repository Linux gate**

Create `.github/workflows/ci.yml`:

```yaml
name: C++ CI

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

permissions:
  contents: read

jobs:
  build-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v6
      - name: Configure
        run: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
      - name: Build
        run: cmake --build build --parallel 2
      - name: Test
        run: ctest --test-dir build --output-on-failure
      - name: Render small scientific reference
        run: |
          ./build/gargantua-render-reference \
            --output /tmp/gargantua-ci-reference \
            --mass-M 1 --spin 0 --observer-r-M 30 \
            --inclination-deg 90 --fov-y-deg 40 \
            --width 10 --height 8 --escape-r-M 60 \
            --max-affine-M 200 \
            --initial-mino-step 0.02 --max-mino-step 0.25 \
            --disk-outer-r-M 20 --disk-opacity opaque
          test -s /tmp/gargantua-ci-reference/beauty.ppm
          grep -q '"failed":0' \
            /tmp/gargantua-ci-reference/manifest.json
```

Commit the workflow only after the same commands pass locally:

```sh
git add .github/workflows/ci.yml
git commit -m "ci: verify scientific reference renderer"
```

- [x] **Step 2: Clean Release verification**

Run:

```sh
cmake -S . -B build-release \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel 4
ctest --test-dir build-release --output-on-failure
git diff --check
```

Expected: all Gargantua tests pass and no new compiler warning appears.

- [x] **Step 3: Combined sanitizer verification**

Configure:

```sh
cmake -S . -B build-sanitize \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build-sanitize --parallel 4
ctest --test-dir build-sanitize -R \
  'gargantua.(scene_camera|reference_ray_evidence|solar_kerr_path|solar_kerr_ray_tracer|reference_renderer|reference_output)' \
  --output-on-failure
```

Expected: every focused test passes without ASan/UBSan diagnostics.

- [x] **Step 4: Requirements and architecture audit**

Check:

- no positive-Mino production path;
- no copied Kerr, disk, or redshift formula in Gargantua;
- no raw value overwritten by display encoding;
- no failed classification treated as success;
- no L3 dependency imported into L0/L1;
- `solar_kerr_ray_tracer.cpp` is below the review signal after splitting;
- any remaining file over 200 lines has one cohesive responsibility;
- no CUDA/WebGL/OpenEXR/UI placeholder was added;
- no untracked root `.DS_Store` was staged.

- [x] **Step 5: Fresh completion evidence**

Run:

```sh
git status -sb
git log --oneline origin/main..HEAD
git diff --stat origin/main...HEAD
git diff --check origin/main...HEAD
ctest --test-dir build-release --output-on-failure
```

Record exact pass counts and the accepted reference maxima/checksums.

- [x] **Step 6: Push and create the public PR**

```sh
git push -u origin codex/phase6-scientific-reference
gh pr create \
  --repo TT1nKer/gargantua-studio \
  --base main \
  --head codex/phase6-scientific-reference \
  --title "feat: render Solar scientific disk references" \
  --body-file /tmp/gargantua-phase6-pr.md
```

The PR body must state the direction defect, solution, model boundary, test
commands, measured evidence, remaining risks, and fastest falsification.

- [ ] **Step 7: Require Linux CI and merge**

Wait for the exact PR head SHA's C++ CI. Require configure, warning-enabled
build, all tests, and a small CLI artifact to pass on Linux. If green, merge
the PR using the already approved public integration route, then require the
automatic `main` push CI for the merge commit to pass.

- [ ] **Step 8: Record the next permitted slice**

After merge, the next plan may begin offline HDR/cinematic infrastructure:
linear color policy, OpenEXR, camera sequences, sampling, and CPU/CUDA parity
fixtures. Web UI and WebGL remain deferred.
