# SegelPhysik-GUI

Native desktop front end for the SegelPhysik C++ Core. The GUI directly links
the Core library; model validation and simulation are started by the GUI and
run off the interface thread. Qt 6 Widgets provides the desktop controls and
VTK renders the model, water surface, waterline and force vectors.

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

## Use

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

Under **Setup → Grafik …**, select wake/bow wave and technical water-level
track independently for each profile. The existing world grid, unlabelled
distance marks, spatial sampling, age and point budget make motion visible
with the body-fixed camera. No new numerical motion HUD or old 3D trail is
enabled. See [operation, limits and focused evidence](Dokumentation/Testantrieb_und_Fahrtdarstellung.md).
Technical implementation is verified; personal visual/functional acceptance
and a separate performance acceptance remain open.

### Graphics profiles

The **Darstellung** selector beside the vector controls switches between
**Technik** (transparent analysis) and **Optik** (graphical presentation).
The selector remains available even when vectors are hidden.

Open **Setup → Grafik …** to configure both profiles independently.
Feature switches, opacity, normal strength, world-space water period,
quality and vector occlusion
are saved per user using QSettings. **Apply/OK** saves both profiles;
**Cancel** discards edits made after the last successful Apply.
Profile defaults and **All on/off** affect only the edited profile.
The last active profile is restored on the next launch. Existing vector
groups, scale settings and model-folder preferences are retained.
Each profile can select a Radiance HDR environment; leave the environment
path empty for the built-in procedural sky. Asset references are saved with
the profile. External files are not copied into the project or distributed
automatically; their rights remain the user's responsibility.
Unavailable dependent switches explain the required parent features;
their checked state remains the saved wish when a parent is off.

The course trail (**Kursspur**) is temporarily off by default in both profiles.
Its existing 3D mesh-centre trajectory can form bright loops at sail height
during rocking motion. The feature and its Setup switch are retained for
later reconsideration; it can still be explicitly enabled and saved.

Optional live reduction temporarily disables SSAO, shadows and object
reflection during a running simulation without overwriting saved wishes.
It is off by default. Rendering diagnostics distinguish unavailable or
inapplicable features from the desired profile settings.

**Setup → Materialvorschau …** explicitly assigns a preview material to a
chosen display part (original, lacquer, cloth, wood or brushed metal).
This assignment lasts until the next model load and never changes model
files or physical properties. Use it to compare materials on the Folkeboot
without inferring material roles from element names.

Tests for settings and dialog transactions run with CTest. The GPU test is
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
evidence subfolders. They also exercise real MainWindow profile switching
during playback, unchanged force-table values/camera, and cancellation
while Setup is open. User settings are redirected to a temporary directory.
Material test images use explicitly synthetic preview assignments, not
an inferred or persistent Folkeboot material classification.

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

**Model folder.** The GUI lists every `modell.json` in the model folder and in
its direct subfolders automatically, using the model `name` (fallback `id`,
then folder name). Selecting an entry displays it immediately; the last model
is reopened on the next start. The folder is watched: new, removed or renamed
models update the list, and edits to the files of the shown model redisplay it.
Each listed model is checked by the Core in the background; models the Core
does not release for simulation are marked „nur Ansicht“.

Folder selection order: first command-line argument if it is a folder, then
the folder chosen via „Modellordner wählen“ (remembered), then `modelle` next
to the executable, then the CMake cache value `SEGELPHYSIK_MODELLE_DIR`
(default: the shared folder `Segelphysik\Modelle` next to the `CORE` and
`GUI` checkouts). A model file passed as first argument is
opened directly.

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
