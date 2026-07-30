# Gargantua First CPU Image Vertical Slice Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce one deterministic, Solar-backed Kerr CPU classification frame with per-ray diagnostics, atomic output, and reproducible validation evidence.

**Architecture:** A framework-light reference-render library owns validated scene types, camera mapping, a backend interface, the private Solar adapter, row-major rendering, and an atomic output transaction. A thin CLI maps strict arguments to one render flow. Solar remains the only physics implementation; output and scene modules consume backend-neutral results.

**Tech Stack:** C++17, CMake 3.20+, Solar `0.2.0-alpha.1` / `relativity-v3-phase2`, CTest, binary PPM, CSV, JSON, `std::filesystem`.

## Global Constraints

- C++17 `double` Solar CPU physics is authoritative.
- The metric is fixed Kerr Boyer-Lindquist with signature `(-,+,+,+)` and geometrized units `G=c=1`.
- The capture event is a Boyer-Lindquist interior cutoff at `r_+ + 1e-4 M`; it is not a horizon-crossing claim.
- Accepted ordinary rays require Hamiltonian error `< 1e-10` and Carter relative error `< 1e-9`.
- `MaxAffine`, `MaxSteps`, invalid, non-finite, event-root, and step failures may not be relabeled as escape.
- Output mode is `ENGINE_DEBUG`; no disk, radiation, CUDA, OpenEXR, ACES, or beauty-image claim is permitted.
- Render order is deterministic row-major with one pixel-center sample.
- A completed output is an atomically renamed generation directory.
- Production files should remain focused near the 200-line review signal.

---

### Task 1: Validated scene and perspective camera

**Files:**
- Modify: `CMakeLists.txt`
- Create: `include/gargantua/reference/reference_scene.h`
- Create: `include/gargantua/reference/perspective_camera.h`
- Create: `src/reference_scene.cpp`
- Create: `src/perspective_camera.cpp`
- Create: `tests/test_scene_camera.cpp`

**Interfaces:**
- Produces:
  - `ReferenceScene reference_scene_defaults()`
  - `SceneValidation validate_reference_scene(const ReferenceScene&)`
  - `CameraRay perspective_camera_ray(const ReferenceScene&, std::size_t x, std::size_t y)`
- `CameraRay::local_direction` is `std::array<double, 3>` in Solar observer-local spatial coordinates.

- [ ] **Step 1: Register the focused test before production sources exist**

Add a `GARGANTUA_BUILD_TESTS` option and test helper to `CMakeLists.txt`:

```cmake
option(GARGANTUA_BUILD_TESTS "Build Gargantua tests" ON)

add_library(
    gargantua_reference
    src/reference_scene.cpp
    src/perspective_camera.cpp)
target_compile_features(gargantua_reference PUBLIC cxx_std_17)
target_include_directories(
    gargantua_reference
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(GARGANTUA_BUILD_TESTS)
    add_executable(test-scene-camera tests/test_scene_camera.cpp)
    target_compile_features(test-scene-camera PRIVATE cxx_std_17)
    target_link_libraries(test-scene-camera PRIVATE gargantua_reference)
    add_test(NAME gargantua.scene_camera COMMAND test-scene-camera)
endif()
```

Create `tests/test_scene_camera.cpp` with a local `check` counter. Assert:

```cpp
const auto defaults = reference_scene_defaults();
check("default scene valid", validate_reference_scene(defaults));

auto invalid_spin = defaults;
invalid_spin.spin_chi = 1.0;
check("extremal spin rejected", !validate_reference_scene(invalid_spin));

auto invalid_pixels = defaults;
invalid_pixels.width = 4097;
check("oversized width rejected", !validate_reference_scene(invalid_pixels));

const CameraRay center = perspective_camera_ray(defaults, 31, 17);
check_near("near-center ray looks inward",
           center.local_direction[0], -1.0, 0.0);

ReferenceScene odd = defaults;
odd.width = 3;
odd.height = 3;
const CameraRay exact_center = perspective_camera_ray(odd, 1, 1);
check_near("odd center horizontal", exact_center.local_direction[2], 0.0, 0.0);
check_near("odd center vertical", exact_center.local_direction[1], 0.0, 0.0);
check("top ray points toward decreasing theta",
      perspective_camera_ray(odd, 1, 0).local_direction[1] < 0.0);
check("right ray points toward increasing phi",
      perspective_camera_ray(odd, 2, 1).local_direction[2] > 0.0);
```

- [ ] **Step 2: Run RED**

Run:

```sh
cmake -S . -B build -DGARGANTUA_SOLAR_SOURCE_DIR=/Users/hostsjim/project/solar/.worktrees/gargantua-bootstrap
cmake --build build --target test-scene-camera
```

Expected: compilation fails because the scene and camera headers do not exist.

- [ ] **Step 3: Add the scene and camera contracts**

Create `reference_scene.h`:

```cpp
#pragma once

#include <cstddef>
#include <string>

namespace gargantua::reference {

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

struct SceneValidation {
    bool valid;
    std::string message;
    explicit operator bool() const noexcept { return valid; }
};

ReferenceScene reference_scene_defaults() noexcept;
SceneValidation validate_reference_scene(const ReferenceScene& scene);

} // namespace gargantua::reference
```

Create `perspective_camera.h`:

```cpp
#pragma once

#include "gargantua/reference/reference_scene.h"

#include <array>
#include <cstddef>

namespace gargantua::reference {

struct CameraRay {
    std::size_t pixel_x;
    std::size_t pixel_y;
    std::array<double, 3> local_direction;
};

CameraRay perspective_camera_ray(
    const ReferenceScene& scene,
    std::size_t pixel_x,
    std::size_t pixel_y);

} // namespace gargantua::reference
```

Implement defaults as:

```cpp
return ReferenceScene{
    1.0, 0.5, 30.0,
    85.0 * pi / 180.0,
    40.0 * pi / 180.0,
    64, 36, 60.0, 200.0, 0.02, 0.25};
```

Implement validation in the order documented by the design. Use
`std::isfinite`, checked multiplication (`width > max_pixels / height`), and:

```cpp
const double horizon =
    scene.mass_M *
    (1.0 + std::sqrt(1.0 - scene.spin_chi * scene.spin_chi));
```

Implement camera mapping:

```cpp
const double u =
    2.0 * (static_cast<double>(pixel_x) + 0.5) /
        static_cast<double>(scene.width) - 1.0;
const double v =
    1.0 - 2.0 * (static_cast<double>(pixel_y) + 0.5) /
        static_cast<double>(scene.height);
const double tan_half_fov = std::tan(0.5 * scene.vertical_fov_radians);
const double aspect =
    static_cast<double>(scene.width) / static_cast<double>(scene.height);
return CameraRay{
    pixel_x,
    pixel_y,
    {-1.0, -v * tan_half_fov, u * aspect * tan_half_fov}};
```

Throw `std::out_of_range` only when pixel coordinates are outside a previously
validated scene.

- [ ] **Step 4: Run GREEN**

Run:

```sh
cmake --build build --target test-scene-camera
ctest --test-dir build -R gargantua.scene_camera --output-on-failure
```

Expected: the focused test passes.

- [ ] **Step 5: Commit**

```sh
git add CMakeLists.txt include/gargantua/reference src/reference_scene.cpp \
  src/perspective_camera.cpp tests/test_scene_camera.cpp
git commit -m "feat: add reference scene camera contract"
```

---

### Task 2: Solar-backed Kerr ray tracer

**Files:**
- Modify: `CMakeLists.txt`
- Create: `include/gargantua/reference/reference_ray.h`
- Create: `include/gargantua/reference/reference_ray_tracer.h`
- Create: `src/reference_ray.cpp`
- Create: `src/solar_kerr_ray_tracer.cpp`
- Create: `tests/test_solar_kerr_ray_tracer.cpp`

**Interfaces:**
- Consumes: `ReferenceScene`, `CameraRay`, `Solar::Relativity`.
- Produces:
  - `RayClassification`
  - `ReferenceRayResult`
  - `ReferenceTracerInfo`
  - `ReferenceRayTracer::trace(const CameraRay&)`
  - `make_solar_kerr_ray_tracer(const ReferenceScene&)`

- [ ] **Step 1: Write the failing real-Solar test**

Create `tests/test_solar_kerr_ray_tracer.cpp`. Use an odd `9 x 9` scene so
pixel `(4,4)` is exactly radial. Assert:

```cpp
ReferenceScene scene = reference_scene_defaults();
scene.spin_chi = 0.0;
scene.inclination_radians = half_pi;
scene.width = 9;
scene.height = 9;

const auto built = make_solar_kerr_ray_tracer(scene);
check("Solar tracer initializes", bool(built));
check("capture cutoff is outside horizon",
      built.tracer->info().capture_radius_M > 2.0);
check("Solar contract retained",
      built.tracer->info().physics_contract == "relativity-v3-phase2");

const auto center = built.tracer->trace(
    perspective_camera_ray(scene, 4, 4));
check("center ray captured",
      center.classification ==
          RayClassification::CapturedAtBlCutoff);
check("center constraint accepted",
      center.max_constraint_error < 1.0e-10);

const auto corner = built.tracer->trace(
    perspective_camera_ray(scene, 0, 0));
check("corner ray escapes",
      corner.classification == RayClassification::Escaped);
check("corner Carter accepted",
      corner.max_carter_rel_error < 1.0e-9);
```

Also pass a `CameraRay` containing NaN and assert
`InitializationError`, never `Escaped`.

- [ ] **Step 2: Run RED**

Run:

```sh
cmake --build build --target test-solar-kerr-ray-tracer
```

Expected: compilation fails because the ray contracts and Solar tracer factory
do not exist.

- [ ] **Step 3: Add backend-neutral ray contracts**

Create `reference_ray.h`:

```cpp
#pragma once

#include <cstddef>
#include <string>

namespace gargantua::reference {

enum class RayClassification {
    CapturedAtBlCutoff,
    Escaped,
    Unconverged,
    ConstraintViolation,
    InitializationError,
};

const char* ray_classification_name(RayClassification value) noexcept;
bool is_failed_classification(RayClassification value) noexcept;

struct ReferenceRayResult {
    RayClassification classification;
    std::string termination_reason;
    double final_radius_M;
    double max_constraint_error;
    double max_carter_rel_error;
    std::size_t accepted_steps;
    std::size_t rejected_steps;
};

} // namespace gargantua::reference
```

Create `reference_ray_tracer.h`:

```cpp
#pragma once

#include "gargantua/reference/perspective_camera.h"
#include "gargantua/reference/reference_ray.h"

#include <memory>
#include <string>

namespace gargantua::reference {

struct ReferenceTracerInfo {
    std::string solar_version;
    std::string physics_contract;
    double capture_radius_M;
};

class ReferenceRayTracer {
public:
    virtual ~ReferenceRayTracer() = default;
    virtual const ReferenceTracerInfo& info() const noexcept = 0;
    virtual ReferenceRayResult trace(const CameraRay& ray) const = 0;
};

struct ReferenceTracerBuild {
    std::unique_ptr<ReferenceRayTracer> tracer;
    std::string message;
    explicit operator bool() const noexcept {
        return tracer != nullptr;
    }
};

ReferenceTracerBuild make_solar_kerr_ray_tracer(
    const ReferenceScene& scene);

} // namespace gargantua::reference
```

Implement exhaustive enum-to-name mapping. An unknown enum returns
`"unknown"` and counts as failed.

- [ ] **Step 4: Implement the private Solar adapter**

In `solar_kerr_ray_tracer.cpp`, define the implementation class privately.
Its factory must:

1. validate the scene;
2. create `KerrBoyerLindquistMetric(scene.mass_M, scene.spin_chi)`;
3. create a ZAMO at `{0, r, inclination, 0}`;
4. compute `capture_radius_M = metric.outer_horizon_radius() +
   1e-4 * scene.mass_M`; this keeps the validated radial ray below the
   `1e-10` Hamiltonian gate without entering the failing `1e-5 M` margin;
5. return an error result for any exception or failed observer.

For each ray:

```cpp
const InitialStateResult initial = initialize_local_photon(
    metric_,
    observer_,
    Vec3{{direction[0], direction[1], direction[2]}});
```

Create the CPU config with scene steps and affine limit, enable energy and Lz,
and set:

```cpp
config.carter_evaluator =
    [this](const PhaseSpaceState& state) {
        return evaluate_kerr_constants(
            metric_, state, GeodesicKind::Null).Q;
    };
```

Use:

```cpp
GeodesicEvent{
    "bl-capture-cutoff",
    [capture](const PhaseSpaceState& state) {
        return state.x.v[1] - capture;
    },
    EventDirection::Decreasing,
    TerminationReason::InteriorCutoff,
    1.0e-10 * scene.mass_M}
```

and an increasing escape event with `TerminationReason::Escaped`.

Map termination first, then apply finite and invariant gates. A constraint
failure overrides capture/escape. An unsupported successful-looking reason
maps to `Unconverged`.

Append `src/reference_ray.cpp` and `src/solar_kerr_ray_tracer.cpp` to
`gargantua_reference`, link that target privately to `Solar::Relativity`, and
link the focused test to `gargantua_reference`.

- [ ] **Step 5: Run GREEN and the existing Solar probe**

Run:

```sh
cmake --build build --target test-solar-kerr-ray-tracer
ctest --test-dir build -R gargantua.solar_kerr_ray_tracer --output-on-failure
ctest --test-dir build -R gargantua.solar_probe --output-on-failure
```

Expected: both Gargantua tests pass and no ray failure is hidden.

- [ ] **Step 6: Commit**

```sh
git add CMakeLists.txt include/gargantua/reference/reference_ray.h \
  include/gargantua/reference/reference_ray_tracer.h \
  src/reference_ray.cpp src/solar_kerr_ray_tracer.cpp \
  tests/test_solar_kerr_ray_tracer.cpp
git commit -m "feat: trace reference Kerr pixels through Solar"
```

---

### Task 3: Deterministic reference-frame flow

**Files:**
- Modify: `CMakeLists.txt`
- Create: `include/gargantua/reference/reference_frame.h`
- Create: `include/gargantua/reference/reference_renderer.h`
- Create: `src/reference_frame.cpp`
- Create: `src/reference_renderer.cpp`
- Create: `tests/test_reference_renderer.cpp`

**Interfaces:**
- Consumes: a validated scene and any `ReferenceRayTracer`.
- Produces:
  - `ReferenceFrameSummary`
  - `ReferenceFrame`
  - `ReferenceRenderResult render_reference_frame(...)`

- [ ] **Step 1: Write the failing deterministic-order test**

Create a `RecordingTracer final : ReferenceRayTracer` in the test. It records
pixel `(x,y)` pairs and returns captured for `x==0`, escaped for `x!=0`, and
constraint violation at one configured pixel.

Assert:

```cpp
ReferenceScene scene = reference_scene_defaults();
scene.width = 3;
scene.height = 2;
const auto rendered = render_reference_frame(scene, tracer);
check("render succeeds structurally", bool(rendered));
check("one result per pixel", rendered.frame->rays.size() == 6);
check("row-major first", tracer.seen[0] == Pixel{0, 0});
check("row-major row boundary", tracer.seen[3] == Pixel{0, 1});
check("captured count", rendered.frame->summary.captured == 2);
check("escaped count", rendered.frame->summary.escaped == 3);
check("failure count", rendered.frame->summary.failed == 1);
check("failed frame status",
      rendered.frame->status == FrameStatus::DiagnosticFailed);
```

Also make the tracer throw once and assert the renderer returns a structural
error rather than a partial vector.

- [ ] **Step 2: Run RED**

Run:

```sh
cmake --build build --target test-reference-renderer
```

Expected: compilation fails because frame and renderer contracts do not exist.

- [ ] **Step 3: Add frame types and the direct render loop**

Define:

```cpp
enum class FrameStatus { Complete, DiagnosticFailed };

struct ReferenceFrameSummary {
    std::size_t captured = 0;
    std::size_t escaped = 0;
    std::size_t unconverged = 0;
    std::size_t constraint_violations = 0;
    std::size_t initialization_errors = 0;
    std::size_t failed = 0;
    double max_constraint_error = 0.0;
    double max_carter_rel_error = 0.0;
    std::size_t max_accepted_steps = 0;
    std::size_t max_rejected_steps = 0;
};

struct ReferenceFrame {
    ReferenceScene scene;
    ReferenceTracerInfo tracer;
    std::vector<ReferenceRayResult> rays;
    ReferenceFrameSummary summary;
    FrameStatus status;
};

struct ReferenceRenderResult {
    std::optional<ReferenceFrame> frame;
    std::string message;
    explicit operator bool() const noexcept {
        return frame.has_value();
    }
};
```

`render_reference_frame` validates the scene, reserves the exact checked pixel
count, calls `perspective_camera_ray` then `trace` in row-major order, and
updates every count exhaustively. It rejects mismatched vector size and any
exception with a message; it never inserts a fabricated ray.

- [ ] **Step 4: Run GREEN**

Run:

```sh
cmake --build build --target test-reference-renderer
ctest --test-dir build -R gargantua.reference_renderer --output-on-failure
```

Expected: focused renderer tests pass.

- [ ] **Step 5: Commit**

```sh
git add CMakeLists.txt include/gargantua/reference/reference_frame.h \
  include/gargantua/reference/reference_renderer.h \
  src/reference_frame.cpp src/reference_renderer.cpp \
  tests/test_reference_renderer.cpp
git commit -m "feat: render deterministic reference frames"
```

---

### Task 4: Atomic PPM, CSV, and manifest generation

**Files:**
- Modify: `CMakeLists.txt`
- Create: `include/gargantua/version.h`
- Create: `include/gargantua/reference/reference_output.h`
- Create: `src/reference_output.cpp`
- Create: `tests/test_reference_output.cpp`

**Interfaces:**
- Consumes: a complete in-memory `ReferenceFrame` and absent output path.
- Produces:
  - `ReferenceOutputResult write_reference_generation(...)`
  - directory files `classification.ppm`, `rays.csv`, `manifest.json`
  - named FNV-1a 64-bit checksums.

- [ ] **Step 1: Write the failing output-transaction test**

Build a `2 x 2` synthetic frame containing each relevant palette value. Create
a unique directory under `std::filesystem::temp_directory_path()`, then assert:

```cpp
const auto written = write_reference_generation(output, frame);
check("output succeeds", bool(written));
check("final directory exists", is_directory(output));
check("part directory absent", !exists(output.string() + ".part"));
check("PPM exists", exists(output / "classification.ppm"));
check("CSV exists", exists(output / "rays.csv"));
check("manifest exists", exists(output / "manifest.json"));
check("PPM uses P6 header", read_prefix(ppm, 2) == "P6");
check("CSV contains header and four rows", csv_line_count == 5);
check("manifest names checksum algorithm",
      manifest.find("\"checksum_algorithm\":\"fnv1a64\"") != npos);
check("manifest records failed status",
      manifest.find("\"status\":\"diagnostic_failed\"") != npos);
```

Call the writer again and assert it rejects the existing output without
altering its checksum. Precreate `<output>.part` and assert rejection.

- [ ] **Step 2: Run RED**

Run:

```sh
cmake --build build --target test-reference-output
```

Expected: compilation fails because the output contract does not exist.

- [ ] **Step 3: Implement serialization and the directory transaction**

Create:

```cpp
namespace gargantua {
inline constexpr std::string_view version{"0.1.0"};
}
```

Define:

```cpp
struct ReferenceOutputResult {
    bool written;
    std::string message;
    std::uint64_t ppm_checksum;
    std::uint64_t csv_checksum;
    explicit operator bool() const noexcept { return written; }
};

ReferenceOutputResult write_reference_generation(
    const std::filesystem::path& output_directory,
    const ReferenceFrame& frame);
```

Implementation order:

1. reject empty paths, an absent parent, existing final output, or existing
   `<output>.part`;
2. create exactly the part directory;
3. build PPM and CSV byte strings in memory;
4. compute FNV-1a using offset `14695981039346656037` and prime
   `1099511628211`;
5. write and close PPM, CSV, then manifest, checking every stream;
6. rename the part directory to the final directory;
7. on a pre-rename failure, leave the part directory visible and return a
   specific message.

Manifest serialization must use a private JSON string-escape function for
quotes, backslashes, and ASCII control bytes. Use lowercase 16-digit hex for
checksums and `std::setprecision(17)` for doubles.

Build the canonical scene-hash input in this exact key order:

```text
mass_M,spin_chi,observer_radius_M,inclination_radians,
vertical_fov_radians,width,height,escape_radius_M,max_affine_M,
initial_step_M,max_step_M
```

Separate keys with `=` and records with `\n`, format doubles with 17
significant digits, then apply the same FNV-1a function. The manifest field is
named `scene_hash_fnv1a64`.

The fixed missing-capability array is:

```json
[
  "trajectory_min_radius",
  "azimuthal_winding",
  "disk_intersections",
  "redshift_and_radiative_transfer",
  "kerr_schild_horizon_crossing",
  "separated_mino_solver",
  "cuda",
  "openexr_aces",
  "beauty_render"
]
```

- [ ] **Step 4: Run GREEN**

Run:

```sh
cmake --build build --target test-reference-output
ctest --test-dir build -R gargantua.reference_output --output-on-failure
```

Expected: all atomic-output assertions pass.

- [ ] **Step 5: Commit**

```sh
git add CMakeLists.txt include/gargantua/version.h \
  include/gargantua/reference/reference_output.h \
  src/reference_output.cpp tests/test_reference_output.cpp
git commit -m "feat: write atomic reference frame evidence"
```

---

### Task 5: Strict reference-render CLI and end-to-end acceptance

**Files:**
- Modify: `CMakeLists.txt`
- Create: `cli/reference_render_options.h`
- Create: `cli/reference_render_options.cpp`
- Create: `cli/reference_render_main.cpp`
- Create: `tests/test_reference_render_options.cpp`
- Create: `tests/test_reference_render_cli.sh`

**Interfaces:**
- Consumes: CLI arguments and the Task 1–4 library.
- Produces: executable `gargantua-render-reference` and CTest
  `gargantua.reference_render_cli`.

- [ ] **Step 1: Write failing parser and process tests**

The parser test asserts:

```cpp
check("help accepted", parse({"app", "--help"}).show_help);
check("output required", !parse({"app"}));
check("unknown option rejected",
      !parse({"app", "--output", "x", "--unknown", "1"}));
check("repeated width rejected",
      !parse({"app", "--output", "x", "--width", "3", "--width", "4"}));
check("NaN rejected",
      !parse({"app", "--output", "x", "--spin", "nan"}));
check("trailing numeric text rejected",
      !parse({"app", "--output", "x", "--spin", "0.5x"}));
check("valid overrides applied",
      parse(valid_args).options->scene.width == 9);
```

The shell test uses `mktemp -d` and asserts:

- `--help` exits `0`;
- an unknown option exits `2`;
- a `9 x 9` Schwarzschild render exits `0`;
- all three output files exist;
- a second render to the same output exits `5`;
- manifest count totals equal `81`;
- manifest contains both captured and escaped rays;
- manifest contains no failed rays.

- [ ] **Step 2: Run RED**

Run:

```sh
cmake --build build --target test-reference-render-options \
  gargantua-render-reference
```

Expected: targets or sources are missing.

- [ ] **Step 3: Implement strict option parsing**

Define:

```cpp
struct ReferenceRenderOptions {
    ReferenceScene scene;
    std::filesystem::path output_directory;
};

struct ReferenceRenderParse {
    std::optional<ReferenceRenderOptions> options;
    bool show_help = false;
    std::string message;
    explicit operator bool() const noexcept {
        return show_help || options.has_value();
    }
};

ReferenceRenderParse parse_reference_render_options(
    const std::vector<std::string>& arguments);
std::string reference_render_usage();
```

Recognize exactly:

```text
--help
--output
--mass-M
--spin
--observer-r-M
--inclination-deg
--fov-y-deg
--width
--height
--escape-r-M
--max-affine-M
--initial-step-M
--max-step-M
```

Use `std::from_chars` for sizes and `std::strtod` with `errno`, end-pointer,
and finite checks for doubles. Track seen option names in `std::set`. Convert
degrees only after successful parsing, then call scene validation.

- [ ] **Step 4: Implement the thin CLI flow**

`main` performs:

```cpp
const auto parsed = parse_reference_render_options(arguments);
if (!parsed) return 2;
if (parsed.show_help) { std::cout << usage; return 0; }

const auto tracer = make_solar_kerr_ray_tracer(parsed.options->scene);
if (!tracer) return 3;
const auto rendered =
    render_reference_frame(parsed.options->scene, *tracer.tracer);
if (!rendered) return 3;
const auto output = write_reference_generation(
    parsed.options->output_directory, *rendered.frame);
if (!output) return 5;
print one compact JSON summary;
return rendered.frame->status == FrameStatus::Complete ? 0 : 4;
```

No CLI catch may convert an exception to success. A final `catch
(const std::exception&)` prints the message and returns `3`.

- [ ] **Step 5: Run GREEN and full local CTest**

Run:

```sh
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Expected: probe, module tests, parser tests, and CLI process test all pass.

- [ ] **Step 6: Commit**

```sh
git add CMakeLists.txt cli tests/test_reference_render_options.cpp \
  tests/test_reference_render_cli.sh
git commit -m "feat: add reference render command"
```

---

### Task 6: Actual reference artifact and validation report

**Files:**
- Modify: `README.md`
- Create: `docs/validation/01_cpu_reference_frame.md`
- Create generated, Git-ignored evidence under `artifacts/reference-frame-v1/`
- Modify: `.gitignore`

**Interfaces:**
- Consumes: the verified CLI and actual output.
- Produces: reproducible validation evidence, explicit model boundary, and the
  next Solar capability request.

- [ ] **Step 1: Ignore generated render generations**

Add:

```gitignore
/artifacts/
```

The validation report remains tracked; binary and per-ray generated evidence
does not enter Git history.

- [ ] **Step 2: Render two fresh identical generations**

Run:

```sh
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
```

Expected: both renders exit `0`, contain captured and escaped pixels, contain
zero failed rays, and compare byte-for-byte equal.

- [ ] **Step 3: Write the validation report from those outputs**

Use the exact headings:

```markdown
# Validation: Solar-backed CPU reference frame

## Claim
## Model boundary
## Reference
## Command
## Inputs
## Expected
## Actual
## Error
## Result
## Limitations
## Fastest falsification
```

Record the actual Solar commit, package version, physics contract, compiler,
classification counts, maxima, elapsed time, PPM/CSV checksums, repeated-run
comparison, and command exit codes. State that the cutoff proves capture
classification outside the BL horizon, not horizon crossing. State that the
image is classification evidence, not a beauty render.

- [ ] **Step 4: Link the artifact contract and next boundary**

Update README with:

- the exact reference-render command;
- the three generated output files;
- the debug palette;
- the validation-report link;
- the missing-capability statement;
- the next Solar demand: bounded minimum-radius, winding, and event-history
  diagnostics.

- [ ] **Step 5: Run the complete gates**

Run:

```sh
cmake -S . -B build \
  -DGARGANTUA_SOLAR_SOURCE_DIR=/Users/hostsjim/project/solar/.worktrees/gargantua-bootstrap \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
git diff --check
git status --short
```

In Solar, run:

```sh
make test-external-consumer
```

Expected: every Gargantua test and the Solar external consumer pass. Only
documented generated artifacts are ignored.

- [ ] **Step 6: Commit**

```sh
git add .gitignore README.md docs/validation/01_cpu_reference_frame.md
git commit -m "docs: validate Solar-backed reference frame"
```
