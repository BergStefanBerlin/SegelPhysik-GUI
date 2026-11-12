# SegelPhysik-GUI

Native desktop front end for the SegelPhysik C++ Core. The GUI directly links
the Core library; model validation and simulation are started by the GUI and
run off the interface thread. Qt 6 Widgets provides the desktop controls and
VTK renders the model, water surface, waterline and force vectors.

User acceptance of the delivered hydrostatics, hydrodynamics and interaction
milestone was explicitly completed (`done`) on 2026-11-04.
The [implementation plan](Umsetzungsplaene/Umsetzungsplan_G-0.11.md)
records the current regression evidence separately from historical results.

## Requirements

- Windows x64, Visual Studio 2022 C++ build tools and CMake 3.25 or newer.
- vcpkg with the baseline and dependencies in `vcpkg.json`.
- A separate SegelPhysik-CORE checkout containing its `Code` CMake project.
- Qt 6 and VTK as declared dependencies. Review the Qt and VTK licenses for
  the actual distribution before packaging the application.

See the [license review](Dokumentation/Lizenzdokumentation.md) for the observed
dependency versions and outstanding distribution obligations. This review is
not a distribution approval.

The intended terms for the project's own code are noncommercial, with an
education/nonprofit exception and free redistribution of derivatives with
source. The [proposed terms](Dokumentation/Lizenzbedingungen_Entwurf.md) are
an **inactive draft**, not an open-source license or a grant of rights.
Third-party licenses remain unchanged; legal compatibility is still open.

## Configure and build

Building Qt through vcpkg fails when the vcpkg install or build-tree path
contains spaces (qtsvg cannot find the qtbase SBOM document). Therefore the
dependencies and the build tree are placed in a path without spaces, here
`C:\spdeps`. `VCPKG_MAX_CONCURRENCY` limits the parallel compiler jobs; the
first Qt/VTK build takes several hours.

Two upstream download hosts failed during the dependency build (libaec:
gitlab.dkrz.de returned HTTP 429; gl2ps: gitlab.onelab.info returned an HTML
page). Overlay ports in `C:\spdeps\overlay` point these two ports to verified
mirrors (`MathisRosenhauer/libaec` on GitHub, `geuz.org/gl2ps/src/gl2ps-1.4.2.tgz`)
with checked SHA512 hashes; pass them with `--overlay-ports`.

`cmake` and `ctest` are not on `PATH` with the Build Tools; use
`...\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`
or a Developer PowerShell.

From this repository root, in PowerShell:

```powershell
$vcpkg = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\vcpkg"
$core = "C:\_DATEN\Code\Python\Organisierter Code\Segelphysik\CORE"
$env:VCPKG_MAX_CONCURRENCY = "6"
& "$vcpkg\vcpkg.exe" install --triplet x64-windows --vcpkg-root $vcpkg `
  --overlay-ports C:\spdeps\overlay `
  --x-install-root C:\spdeps\installed `
  --x-buildtrees-root C:\spdeps\buildtrees `
  --x-packages-root C:\spdeps\packages
cmake -S . -B C:\spdeps\guibuild -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$vcpkg\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_INSTALLED_DIR=C:/spdeps/installed -DVCPKG_MANIFEST_INSTALL=OFF `
  -DSEGELPHYSIK_CORE_SOURCE_DIR="$core"
cmake --build C:\spdeps\guibuild --config Release --parallel 6
$env:PATH = "C:\spdeps\installed\x64-windows\bin;$env:PATH"
ctest --test-dir C:\spdeps\guibuild -C Release --output-on-failure
```

Set `SEGELPHYSIK_CORE_SOURCE_DIR` to the local Core checkout if it is stored
elsewhere. The GUI links the Core target directly; the Core CLI is not required
at runtime.

Model data directories are named `modells` (formerly `meshes`). The Core
resolves file references from each `modells_information.json`. Core stores each
component's `setting` directly beside its existing `id` and `Darstellung`,
without additional object IDs or a second component list. Shared revision
metadata remains in `konfiguration`. Source/lookup references occur once
at the sail element; the Fock offset is derived from its pose. Unrelated
model fields and other components are preserved. Mounting poses remain separate from operating
angles. Saving is atomic and uses `modells_information.lock`; revisions and source
checks still guard against conflicts and changed geometry. The former
`segelkonfiguration.json` is migrated without resetting sail placement.

The stored boat base pose places sails and boom amidships, with clews aft;
0 degrees is this physical opening reference, not an identity quaternion.
Mast/forestay inclination and sail camber are preserved.

The GUI has no separate model-directory parser or component configuration
file. Renaming data directories does not rename API mesh
fields or alter the geometry and lookup tables.

The performance implementation requires Core C-0.10's
`Modellaufbereitung` API. Each live run owns separate persistent worker and
display contexts: stepping and historical JSON generation reuse exact prepared
data without sharing mutable state between the GUI and worker threads.
Replacing the model creates new contexts. Batch simulation uses the Core's
internal context; no solver settings or integration steps are reduced.
See the [performance plan and evidence](Umsetzungsplaene/Umsetzungsplan_G-0.11.md).

The dynamic camera reference is located in the right-hand display panel.
In axis-fixed boat-selection views, dragging the background pans the image
in the mouse direction, both horizontally and vertically.
The auxiliary side/rear/top views are labelled "Analyseansichten".
Without saved mode-specific settings, the static workspace starts with
compass/attitude instruments off and analysis views on; the dynamic workspace
starts with instruments on and analysis views off. Saved choices are preserved.

Static display settings omit motion/wake/track controls. Their own technical
default matches the static choices saved on 2026-11-04: opaque material surfaces
with color/ORM/normal textures, SSAO, grid, axes, waterline, sky and haze;
calm water at 18% opacity, without sun lighting, sun shadows or reflections.
Water animation is selected but has no effect while water normals are disabled.
"Auf Default setzen" immediately applies
and persists this static default; later custom choices also persist. Existing
saved static settings are retained until explicitly reset. Dynamic defaults
and saved dynamic settings are independent and unchanged.

Dynamic motion visualization switches are in the display sidebar, between
"Darstellung" and the simple force arrows. The dynamic default uses the full
optical profile at maximum quality; only the bow/stern wave motion effect is
enabled by default (no technical tracks, distance marks or course trail).
The dynamic settings page also offers "Auf Default setzen"; saved user choices
remain independent of the static workspace.

Loading or replacing a model applies the current graphics profile immediately,
including material opacity and textures. Toggling course trails therefore does
not incidentally change the model's material appearance.

During a dynamic run, foreign-force activation, XYZ force, its application
point and rudder/sword values remain editable. Confirmed numeric edits take
effect at the next uncomputed integration step, never retrospectively.
Movement edits during startup are queued even before the first published
snapshot; the original initial state is retained.
Historical frames retain their original force contexts and mesh transforms.
The existing movement-event path updates physical preparation and display
geometry consistently; no model files are rewritten. The status area reports
applied changes or changes that could no longer be applied before the run ended.

## Use

### Workspace tabs and colors

The tabs above the workspace select its color scheme: **Bootsauswahl**
(bordeaux red), **Statisches Modell** (dark blue), and **Dynamisches Modell**
(dark green). Each tab retains its own color even while inactive; the active
tab has a brighter fill and underline.
Checked checkboxes retain the workspace color and also show a white checkmark,
so their state is not conveyed by color alone (including child setup dialogs).
The GUI build deploys Qt's SVG image plugin into `imageformats` beside the
executable. This is required for both checkmarks and navigation instrument
images when starting without a development `QT_PLUGIN_PATH`.

The active model name appears outside the side panels, right-aligned on the
workspace-tab row, in the same font size as **SEGELPHYSIK**. In **Statisches
Modell**, the left model list is replaced by the complete **DARSTELLUNG** block
(vector visibility, camera, scales, profiles, force/moment selections and
legend). Simulation/run controls remain on the right. The existing display
widgets move rather than being duplicated: selections, signal connections and
camera behavior are preserved. **Dynamisches Modell** retains the model list
and right-side display controls; **Bootsauswahl** retains its inspection layout.

The static/dynamic choice follows the workspace rather than a separate
dropdown. Switching workspaces cancels and resets the previous simulation.
The static right sidebar places **Fremdkrafteinwirkung** below the start/stop
buttons and fixed **Ruder- und Schwertstellung** settings below the force.
The single force is grouped as one X/Y/Z force vector and one shared X/Y/Z
application point, not three separate forces. Static and dynamic viewports use
the same rounded clipping contour as boat inspection, including after resizing.
The external-force application point is model-fixed; force components may be
world- or model-fixed. Fixed element inputs use Core movement metadata and are
locked during a static solve. Changing inputs after a solve marks the displayed
result as belonging to the previous input case.
Reference settings and units come from the Core inspection metadata as well;
missing movement metadata blocks a stationary run with an explicit diagnostic.
Absent physical limits are identified in the tooltip rather than invented.

Stationary runs use the typed Core stationary configuration. The explicit
**Stationäre Kurvenfahrt zulassen** switch enables the extended Core solve:
constant body-relative water velocity, roll/pitch and signed heading rate.
Disable it to retain the previous straight-line solve with zero angular rate.
Neither switch position optimizes the fixed rudder/centreboard settings.
Result contract V6 carries the new turning and angular-rate semantics;
older V1–V5 results remain readable without reinterpretation. A V5 result
without its `stationaer` context is rejected explicitly.
The values tree retains that entire Core context, including its frozen inputs,
convergence status, tolerances, force/moment residuals, relative velocity, drift
and yaw-reference information. Position, quaternion, world velocity and
orientation are also shown alongside it. The GUI does not re-solve physics or
replace a failed equilibrium with a speed cap. X/Y position and, where Core
marks it as undetermined, absolute yaw are references, not additional solved
physical constraints. Numerical convergence does not assert dynamic stability
or uniqueness; fixed rudder settings need not admit a straight-line solution.
The balance card scrolls when the values dock is short, so convergence and
reference-course notices remain accessible. Stationary iteration counts are
not labelled as physical simulation time steps.
For a non-converged stationary result, the warning now includes all three
world-force and model-moment residual components and their tolerances. The
balance card explicitly labels both speed and pose as approximations.
Fixed rudder inputs are never silently optimized to obtain convergence.

The navigation speed card shows the world-CG speed magnitude in m/s, then
the signed **Kursdrehrate** in degrees/s, then the existing world-X/Y/Z
velocities. Core supplies the angular rate in rad/s for both stationary and
dynamic results; the GUI only converts units. Positive rate is right-handed
about world +Z (counterclockwise viewed from above), unlike the clockwise
compass-angle convention. It is not the norm of angular velocity or its
body-Z component. Missing legacy values and singular course states display
`-- °/s` with their status; numerical zero displays `0.00 °/s`.
Nonconverged rates are marked as approximate; tooltips retain full precision.
With navigation enabled, a mint-colored direction arrow floats just above
the water ahead of the bow in both simulation workspaces. It follows the
horizontal boat heading: zero course rate is straight, positive Core rate
bends to port and negative rate to starboard. Bend increases continuously
with rate magnitude (four-second symbolic angular scale, capped at 80 degrees);
this is an indication of the current rate, not a predicted trajectory or
a force/moment arrow. Missing rates or a singular horizontal heading hide
the arrow. It uses the displayed Core state, including approximate stationary
results, without deriving a second angular rate.

Switching between **Bootsauswahl**, **Statisches Modell** and **Dynamisches
Modell** cancels the running calculation and clears playback, motion history,
displayed results and the direction arrow. Late results from a cancelled
stationary calculation are discarded. A new run starts from the model's
initial state, not the previous workspace's final state; configured force
and fixed stationary movement inputs remain available. Starting another run
waits until the old worker has actually finished.

During turning, external force/moment sums need not vanish: all six Core
**motion residuals**, including acceleration and gyroscopic terms, determine
convergence. The GUI displays these separately and does not infer equilibrium
from rounded speed readings.
Cold turning solves also initialize candidate heading rates from the
Core yaw-motion balance in both directions. This avoids a wrong initial
turning branch at large fixed rudder angles; it does not change the
requested force, application point or element settings. Success still
requires every force and moment motion residual to meet its tolerance.

**Bootsauswahl** provides read-only model inspection. The model-folder button
sits below the model list and is absent from the other workspaces. Core-supplied
The single object tree lists each element once in its assembly; there are no
separate GUI, hydrostatics or hydrodynamics tabs. Ctrl/Shift selection includes
descendants and populates the complete property table without duplicate values
or automatic sums. The adjustable **Bereich** column identifies each property's
domain, including movement, mass/material and computed values.
All six columns can be resized by dragging the visible boundaries between their
header titles. Clicking a header
toggles ascending/descending sorting; numeric values sort by their original
values rather than their rounded labels. The six fields within the table header
filter their respective columns (case-insensitive, combined with AND). Each
filter follows its column's width and horizontal scrolling; an empty filter
shows only `*` as a placeholder, meaning no restriction. Colors appear
as rounded square swatches. Numbers use at most two decimal places, or two
significant digits for very small/large values. Lengths and masses can display
in scaled units (km/mm/µm and t/g/mg); tooltips retain the exact original values
and units. These are display changes only.

The model-catalog tests accept both legacy and migrated schema-3 reference
folders. Existing schema-3 sources retain their explicit derivations rather
than being downgraded to schema 2; legacy fixtures still exercise conversion
and explicit conflict handling.
The right object tree contains exactly one reference per existing object.
Only its current display geometry is rendered; a confirmed or temporary
mounting pose replaces that object's geometry instead of adding another
source instance. Physical source roles and derivation metadata remain in
the Core and property table, not as duplicate objects or selectable overlays.
Ambiguous current display references produce an explicit diagnostic.
Unselected parts remain context. Movement definitions are properties of the
object, not a separate movement tab.
Selected elements are opaque, the remaining context uses opacity 0.15.
Without selection the whole model uses its normal materials. The rounded
inspection viewport has a dark bordeaux background and no water surface.
Model-origin axes (X red, Y green, Z blue) and an open spatial reference grid
use equal world-space spacing along all axes. Mouse camera controls remain;
navigation instruments are hidden. Whole-meter grid lines are continuous;
half-meter intermediate lines consist of dots, with fixed world-space spacing
independent of zoom and DPI. Simulation workspaces retain their water.
The lower X/Y grid edges and the left vertical Z edge carry whole-meter labels
in model coordinates. Both side panels share frame styling and padding, with
their outer top edges aligned to the viewport. The object caption is above
the inner tree, not inside it; the former source-color legend is omitted.
Label density follows zoom and viewport size to avoid overlapping numbers;
the underlying 1 m / 0.5 m grid spacing never changes.

A model selection captures one immutable Core inspection snapshot. Subsequent
file changes may update the catalogue but never replace or invalidate that
opened snapshot. No GUI model/mesh reader or second geometry converter is used.
The simulation retains its own model validation and lifecycle.

Moving elements expose a checkable **Bewegungsvorschau** below the element.
It draws the Core-transformed normal pose in cyan, available minimum in orange
and maximum in violet, plus a yellow rotational axis. Undefined limits are
explicitly marked in the tooltip and are not invented. Preview geometry is
temporary, read-only and does not change the simulation or inspection camera.
Selecting a pivot property highlights its model-fixed point and axis in yellow.
Selecting a normal/minimum/maximum table row shows that particular Core pose,
including lifting motions. A table selection takes precedence over the same
element's all-poses checkbox until the table selection is cleared. Filtering out
a selected row removes its table-driven preview. Undefined bounds stay explicit
and cannot generate a pose.

The unified Core definition supports one OBJ containing named source meshes and
a JSON describing their physical uses. Stable element IDs and explicit
hydrodynamic roles are independent; the GUI never infers physics from names.
Existing model formats remain supported through the same Core pipeline.
Real source variants needed for different physical uses are retained rather
than silently replaced. This establishes a read-only foundation for a later
editor; model editing, repair, automatic mesh generation and saving are not
implemented in this step.

Schemas `"2"` and `"3"` are defined in the Core
[model specification](../CORE/Spezifikationen/Spezifikation_Modellbeschreibung.md).
The Core target `segelphysik_konvertieren` converts an existing model into
`modells_information.json` and `boot.obj` in a **new** destination folder:

```powershell
& "C:\spdeps\gui-workspace-tabs\segelphysik-core\Release\segelphysik_konvertieren.exe" `
  "C:\path\to\source\modells_information.json" "C:\path\to\new-model"
```

The destination's parent must exist. Existing destinations are rejected;
source models are never overwritten. Choose the converted model folder in
the GUI to inspect or simulate it. The converter preserves actual geometry
variants; it does not repair or merge physically different meshes.

For the strict **one source mesh per geometric element** contract, pass
`--schema3` before the two paths. This merges identical per-element geometry
and rejects actual conflicts as `CORE-MOD-0009` before creating the destination.
Eight reference models can be migrated without changing geometry. For the
boats `420`, `folkeboot` and `optimist`, explicitly use
`--schema3-ableitungen` instead. Following the user's approval, this selects
one existing source per element and geometrically verifies reproducible
surface selections / plane cuts against the old physical uses. These boats
do not require separate editable mass and displacement meshes.
`ableitung.art = "ohne_deckflaeche"` excludes existing horizontal lid faces
for an open shell; `"z_schnitt"` takes the part below the declared plane
`z = z_m + steigung_x*x + steigung_y*y`, closing only that declared cut.
The 420 centreboard needs an inclined cut, not an arbitrary horizontal one.
Unsupported or materially different geometry is still rejected.
No mesh simplification or change to mass/support/flooding algorithms is
performed. Open mass rims remain intact. The small 420 hull display
difference is bounded and documented in the Core specification.

All eleven converted reference models are available in
`C:\spdeps\gui-workspace-tabs\single-source-models`; select this folder in
the GUI to use them. Originals remain readable and unchanged. Legacy
multiple sources remain visibly marked; derived meshes are listed under
**Abgeleitete Daten**, with the actual Core definition in their tooltip.
The property table exposes derivation parameters without GUI schema parsing.
Migration bounds and physical/dynamic regression criteria are specified in
the [Core model contract](../CORE/Spezifikationen/Spezifikation_Modellbeschreibung.md).

Switching workspaces preserves the loaded model and configured stationary
inputs, but cancels and resets the simulation. The simulation camera returns
with the model to its initial pose, preserving its boat-relative viewing angle.
Inspection has its own viewport. The workspace selects the simulation mode;
switching never automatically starts a new run. The initial tab is Bootsauswahl;
the title block is above the tabs.

### Navigation and camera views

The viewport contains a draggable **Navigation** overlay with compass,
roll and pitch instruments based on the original project's SVGs.
Drag its header to move it and its lower-right grip to enlarge or shrink it.
It stays inside the viewport; temporary viewport shrinkage does not discard
the chosen size. Toggle **Kompass und Lageanzeigen** under **Darstellung**
to hide/show it (saved for the next launch).
The instruments follow the selected simulation snapshot, not an independent
simulation. Their yellow camera markers follow the actual viewport camera.
No snapshot or a singular angle displays **FEHLER**, not a made-up zero.

**Darstellung → Kamera** offers all 14 legacy camera presets plus free view.
Presets fit the current model using the yacht's own axes. Camera position,
target and up direction then stay constant in the yacht's coordinate system:
translation, roll, pitch and yaw move the camera with the body, so the world
moves around the yacht in the view. Mouse orbit/pan/zoom remain available
and establish the new body-relative view, including during playback/rewind.
The dropdown shows the last selected preset. Loading another model resets
the view; completing a recording no longer refits the world trajectory.
**Backbord, Steuerbord, Bug and Heck** start on the water plane: camera and
target are at world Z=0, giving a horizontal 0° view rather than a view aimed
at the mast-height geometry centre. The yacht axis is projected horizontally
when selecting one of these views; subsequent motion remains body-fixed.

Body axes match the original project: +X bow, +Y port, +Z up. By explicit
user choice, the GUI interprets world +X as **local North** and +Y as West.
This does not change CORE physics or prescribe geographic world axes there.
Current quaternions remain **xyzw**, unlike the legacy **wxyz** arrays.
See [navigation documentation](Dokumentation/Navigation.md) for signs,
singularities, camera semantics, source references and original asset hashes.

### Temporary test propulsion and motion through water

In **Dynamischer Verlauf**, the simulation panel contains
**Testantrieb (temporär)**: initially off, 200 N and exactly 10 kn internally.
Set force and speed limit before starting; the enable flag remains live.
Turning it off removes both test thrust and the artificial horizontal speed
limiter, not the existing motion. CORE owns the force, lever-arm torque and
limiter; historical snapshots retain their applied configuration/events.

The previously saved dynamic graphics remain unchanged. The new
**Darstellung** configuration edits only the static technique view.
The existing world grid, unlabelled
distance marks, spatial sampling, age and point budget make motion visible
with the body-fixed camera. No new numerical motion HUD or old 3D trail is
enabled. See [operation, limits and focused evidence](Dokumentation/Testantrieb_und_Fahrtdarstellung.md).
Technical implementation is verified; personal visual/functional acceptance
and a separate performance acceptance remain open.

### Pressure points

The **Druckpunkte** controls independently show **LDP** (magenta) and
**SDP (Ersatz)** (orange), regardless of force-arrow or navigation visibility.
Both are labeled points at the exact Core model coordinates transformed
with the current boat pose; the former vertical LDP line is removed.
The LDP uses corrected signed panel-sideforce moments, not the magnitude-
weighted component application point. Opposing sideforces can legitimately
place it outside the hull; it is never clamped. Geometric substitutes,
indeterminate and unversioned legacy LDP data are not drawn as corrected LDP.

The replacement SDP is the frozen external-force application point from the
same result, not live editor input and not actual sail/wind physics. An
inactive/zero force or missing external-force context gives no SDP marker.
The detail status explains missing or legacy points. Aligning the two points
is a simplified sideforce illustration, not a complete yaw equilibrium test.
Both visibility choices persist; workspace/model reset clears the markers.
Use the Technik profile to see the underwater LDP through the hull.

### Graphics profiles

The **Darstellung** button is available exclusively in **Statisches Modell**,
also when vectors are hidden. It opens the **Technik** configuration in the
boat viewport, not a modal window. There is no Optik/profile selector for
now; existing optical and dynamic settings are preserved, not deleted.
The former graphics entries under Setup remain removed.

The scrollable table has four columns: an unnamed checkbox/control column,
**Beschreibung**, **CPU Last**, **GPU Last**. The descriptions explain
features and prerequisites; CPU/GPU load classes are qualitative estimates,
not measured percentages or guaranteed frame rates. Numeric controls use
the same table, including opacity, normal strength, water period, quality,
vector occlusion and track parameters.

Each feature description also reports the current renderer setting, refreshed
immediately after a change. Checked effects apply to all supporting parts;
parts without matching explicit material data skip the effect without an
error, keeping the setting saved for suitable parts/models. The analysis
transparency row reports its actual opacity: checking more effects does not
override a low body opacity. Water opacity and the stationary-time limitation
of water animation are stated explicitly. User values are never silently
changed to make an all-checked scene opaque.

Every valid change is immediately saved and applied to the static scene.
There is no Apply/Cancel transaction. **Zurück zum Boot** returns to the
same boat without changing physics, playback or camera. Switching workspace
or model keeps all successfully saved settings. A save failure is reported
and restores the last active control value. Settings survive a program
restart, including a real subprocess restart covered by tests.

The existing per-user QSettings store is reused: static values are under
`statisch/grafik/profile_v1`, initially copied from the old Technik profile.
The old `grafik/profile_v1` remains unchanged and continues to control the
dynamic view. On Windows the standard QSettings backend is the existing
registry store `HKEY_CURRENT_USER\Software\SegelPhysik\SegelPhysik`, not a
new configuration file. Existing vector groups, scales and model-folder
preferences are retained. The static view can select a Radiance HDR
environment; leave the environment
path empty for the built-in procedural sky. Asset references are saved with
the profile. External files are not copied into the project or distributed
automatically; their rights remain the user's responsibility.
Unavailable dependent switches explain the required parent features;
their checked state remains the saved wish when a parent is off.

The course trail (**Kursspur**) is temporarily off by default in both profiles.
Its existing 3D mesh-centre trajectory can form bright loops at sail height
during rocking motion. The feature and its configuration switch are retained for
later reconsideration; it can still be explicitly enabled and saved.

Previously saved dynamic live reduction is retained; no dynamic control
is exposed on the static settings page. Rendering diagnostics distinguish unavailable or
inapplicable features from the desired profile settings.

The former temporary material-preview entry is not offered on this static
page while Optik configuration is postponed. Model materials and saved
optical values remain intact.

Tests for settings, immediate saving and failure rollback run with CTest. The GPU test is
opt-in and needs an interactive OpenGL desktop:

```powershell
cmake -S . -B C:\spdeps\guibuild -DSEGELPHYSIK_GRAPHICS_GPU_TEST=ON
cmake --build C:\spdeps\guibuild --config Release
ctest --test-dir C:\spdeps\guibuild -C Release --output-on-failure
```

The GPU test loads the Folkeboot through the Core catalogue, captures both
profiles, toggles every feature off individually and writes images plus
render timings to `C:\spdeps\guibuild\graphics-evidence`. Timings measure
VTK Render wall time; screenshot readback is recorded separately. They do
not constitute a completed visual/performance acceptance. VTK rendering
errors fail the test even if framebuffer capture returned an image.
The three GPU cases run serially at 100/150/200% Qt scaling, writing separate
evidence subfolders. They also exercise immediate static changes, unchanged force-table
values/camera, workspace isolation and persistence. User settings are
redirected to a temporary directory.
Material test images use explicitly synthetic preview assignments, not
an inferred or persistent Folkeboot material classification.

The focused `gui_workspace_tabs_100`, `gui_workspace_tabs_150` and
`gui_workspace_tabs_200` cases check the three palettes, fixed inactive-tab
colors and the intentional isolation of inspection from simulation controls.
They also check corrected Core pressure points with independent persistent
visibility, exact pose-transformed XYZ (including an outside LDP), legacy/
geometric/missing-state suppression, static-only table changes and persistence,
and cancellation/reset on tab switching, rejection of late static
results, fresh dynamic restart and the Core-driven direction arrow. Actual window
screenshots are written to `graphics-evidence/workspace-tabs-<scale>`.
Run only these cases with
`ctest --test-dir C:\spdeps\guibuild -C Release -R "^gui_workspace_tabs_" --output-on-failure`.

The `gui_boat_inspection_100`, `_150` and `_200` cases exercise the actual
Core snapshot, all advertised tree/mesh types, selection/property deduplication,
point topology and exact context opacity. They compare model-list/viewport
bounds with a maximum one-logical-pixel tolerance, preserve camera state and
corrupt a temporary model file to verify watcher/snapshot isolation. Evidence
is written to `graphics-evidence/boat-inspection-<scale>`.

The Core display contract also accepts explicit `stoff`, `holz` and
`metall_gebuerstet` surfaces in addition to existing surfaces such as `lack`.
The Folkeboot assigns lacquer to its hull, cloth to its sails, wood to its
rudder and brushed metal to its mast in its own display metadata. These are
model-authored presentation choices, not GUI element-name heuristics.
Procedural details use model-local projections; external texture assets
and a general imported UV/seam format are not required for this model.
Hydrostatic mass, volume meshes and physical parameters remain unchanged.

Launch `C:\spdeps\guibuild\Release\segelphysik_gui.exe` (with
`C:\spdeps\installed\x64-windows\bin` on `PATH`). The build copies the Qt
platform and style plugins next to the executable.

**Model folder.** Format-0.3 catalog entries, structure, diagnostics and
fingerprint status come from the Core; the GUI does not parse model JSON or
schemas. The Core resolves the active model and schema roots from
`%APPDATA%\SegelPhysik\defaults.json`. An explicit command-line argument or
folder chosen in the GUI selects a folder; the last explicit folder is
remembered. Model directories are watched and catalog/status changes are
reloaded through the Core.

The legacy `modells_information.json` fixtures are kept under the sibling
folder `Segelphysik\Modelle_alt` for compatibility tests. CMake variable
`SEGELPHYSIK_MODELLE_DIR` points to those legacy fixtures in test builds;
Format-0.3 tests use `SEGELPHYSIK_MODELFORMAT03_TEST_ROOT` and temporary
copies where they write data.

**Static mode** computes the equilibrium and shows it.

**Dynamic mode** runs live without precomputation: the Core advances one
step at a time in a worker thread and only stores the raw states. The view
fetches the newest state about 30 times per second, so rendering never blocks
the Core. With „Echtzeit“ enabled a step is computed only when the wall clock
reaches its simulation time; disabled, the Core runs unthrottled and the view
follows as fast as useful. Duration „bis Abbruch“ (0 s) runs until
„Lauf stoppen“. All states are kept: drag the playback slider back to review
earlier states while the run continues; moving it to the end follows the live
state again. The time line shows simulation time, elapsed real time and the
accumulated Core computation time with its share of real time.
The water surface is drawn at world `Z = 0`; the waterline is generated by
cutting the display mesh for visualization. It does not change Core geometry
or hydrostatics. Force arrows use relative visual scaling; their SI magnitudes
are shown in the contribution table.
Force arrows, moment arcs and lever helpers are rendered separately from
scene shadow/SSAO passes. In spatial mode, opaque model surfaces hide helpers
behind them; through-body mode deliberately shows them through the model.
Analytical helpers do not cast shadows onto sails or other model surfaces.
