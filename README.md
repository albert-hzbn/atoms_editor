# AtomForge

AtomForge is a desktop atomic structure viewer and editor built with OpenGL, Dear ImGui, and Open Babel. It is aimed at interactive inspection and editing of molecules and crystal structures, with integrated builders for bulk crystals, CSL grain boundaries, and nanocrystals.

## What AtomForge does

- Load common atomic and crystallographic file formats.
- View structures in real time with atom, bond, and unit-cell rendering.
- Select, edit, delete, substitute, and transform atoms directly in the UI.
- Inspect structure metadata, coordination, and measurements.
- Run common-neighbour analysis and radial distribution analysis.
- Build periodic crystals, cubic grain boundaries, and carved nanocrystals.

## Platform support

- Linux
- Windows via MSYS2 UCRT64 / MinGW-w64

## Quick start

### Linux

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config libglfw3-dev libglew-dev libglm-dev \
                 libopenbabel-dev libopenbabel3 libsymspg-dev

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/AtomForge
```

### Windows (MSYS2 UCRT64)

Install MSYS2, open the UCRT64 shell, then run:

```bash
pacman -Syu
# close and reopen the UCRT64 shell, then run
pacman -Su

pacman -S --needed mingw-w64-ucrt-x86_64-toolchain \
                  mingw-w64-ucrt-x86_64-cmake \
                  mingw-w64-ucrt-x86_64-glfw \
                  mingw-w64-ucrt-x86_64-glew \
                  mingw-w64-ucrt-x86_64-glm \
                  mingw-w64-ucrt-x86_64-openbabel \
                  mingw-w64-ucrt-x86_64-pkgconf

# optional: enables symmetry features through spglib
pacman -S --needed mingw-w64-ucrt-x86_64-spglib

cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/AtomForge.exe
```

Note: run the Windows build from the MSYS2 UCRT64 shell. PowerShell will often miss the required toolchain and DLL paths.

## Build notes

- The project uses CMake and targets C++11.
- OpenGL 3.x, GLFW, GLEW, GLM, and Open Babel are required.
- spglib or symspg is optional; if found, symmetry-related features are enabled automatically.

To clean the current build tree:

```bash
cmake --build build --target clean
```

To remove all generated artifacts:

```bash
rm -rf build
```

## Supported formats

### Open / import

- `.xyz`
- `.cif`
- `.pdb`
- `.sdf`
- `.mol`
- `.vasp`
- `.mol2`
- `.pwi`
- `.gjf`

### Save / export structure

- `.xyz`
- `.cif`
- `.vasp`
- `.pdb`
- `.sdf`
- `.mol2`
- `.pwi`
- `.gjf`

### Export rendered image

- `.png`
- `.jpg`
- `.svg`

## Core controls

### Scene navigation

| Action | Input |
| --- | --- |
| Rotate scene | Left drag |
| Zoom | Scroll wheel |
| Reset fitted default view | View -> Reset Default View |

### Selection

| Action | Input |
| --- | --- |
| Select one atom | Left click |
| Add or remove from selection | Ctrl + left click |
| Select all | Ctrl+A |
| Clear selection | Ctrl+D or Escape |
| Delete selection | Delete |

Box selection is available through Edit -> Box Select Mode. When enabled, right-drag draws a selection rectangle. Hold Ctrl to add to the current selection.

### File shortcuts

| Action | Shortcut |
| --- | --- |
| Open structure | Ctrl+O |
| Save structure as | Ctrl+S |
| Export rendered image | Ctrl+Shift+S |
| Undo | Ctrl+Z |
| Redo | Ctrl+Y or Ctrl+Shift+Z |

## Main workflows

### Open and inspect a structure

1. Use File -> Open.
2. Rotate and zoom the scene.
3. Toggle View -> Show Bonds and View -> Show Element as needed.
4. Open View -> Structure Info for composition, lattice, positions, and symmetry.

### Edit atoms and structure data

- Right-click a selection to substitute atoms, insert an atom at a midpoint, measure, or delete.
- Use Edit -> Edit Structure to modify lattice vectors and atom positions.
- Use Edit -> Atomic Sizes and Edit -> Element Colors to tune radii, colors, and shininess.
- Use Edit -> Transform Structure to apply a 3x3 matrix to periodic structures.

### Build derived structures

- Build -> Bulk Crystal generates a periodic cell from crystal system, space group, lattice parameters, and asymmetric-unit atoms.
- Build -> CSL Grain Boundary generates cubic bicrystals from ideal sc, bcc, fcc, or diamond templates.
- Build -> Nanocrystal carves a finite particle from a loaded reference structure using sphere, ellipsoid, box, cylinder, octahedron, truncated octahedron, or cuboctahedron geometry.

## Builders and analysis

### Nanocrystal builder

- Uses the currently loaded structure as the reference motif.
- Accepts drag-and-drop of a replacement reference file while the dialog is open.
- Provides an embedded 3D preview of the reference structure.
- Preview controls: left-drag to orbit, scroll to zoom.
- Supports auto-centering, manual carving center, auto-replication for periodic sources, and optional rectangular output cell padding.

### Bulk crystal builder

- Groups space groups by crystal system.
- Applies system-specific lattice constraints.
- Builds the full unit cell from asymmetric atoms and symmetry expansion.

### CSL grain boundary builder

- Limited to cubic source lattices: `sc`, `bcc`, `fcc`, `diamond`.
- Lets you generate Sigma candidates from an axis, then choose one candidate explicitly.
- Supports in-plane replication, overlap removal, rigid translation, and padding.

### Analysis tools

- Common Neighbour Analysis reports Honeycutt-Andersen-style signatures and per-atom environment labels.
- Radial Distribution Function provides species filters, normalization, smoothing, tabulated bins, and first-peak / first-minimum summaries.

## Display and measurements

- Bonds are inferred from covalent radii and rendered as split-color cylinders.
- Periodic image atoms are drawn at cell boundaries for periodic visualization.
- Distance and angle tools draw helper overlays in the scene.
- Atom Info reports element data, Cartesian and direct coordinates, and bond statistics.

## Menus at a glance

### File

- Open structures.
- Save the current structure.
- Export a rendered image.
- Close the current structure.

### Edit

- Undo and redo.
- Box selection mode.
- Structure editing.
- Atomic-size and element-color overrides.
- Supercell transform.

### View

- Element labels and bond visibility.
- Isometric and orthographic camera modes.
- Structure information and measurement dialogs.
- Reset default view.

### Build

- Bulk Crystal.
- CSL Grain Boundary.
- Nanocrystal.

### Analysis

- Common Neighbour Analysis.
- Radial Distribution Function.

### Help

- Manual.
- About.

## Windows troubleshooting

### Open Babel plugin path is missing

If AtomForge reports that Open Babel plugins cannot be found, set the plugin path before launch:

```bash
export BABEL_LIBDIR=/ucrt64/lib/openbabel/3.1.0/
```

### Missing DLLs at startup

Run from the UCRT64 shell, or ensure `C:/msys64/ucrt64/bin` is present in `PATH`.

### CMake cannot find dependencies

Verify that `pkgconf` and the required MSYS2 packages are installed in the same UCRT64 environment used for the build.

## Project layout

```text
CMakeLists.txt              cross-platform build configuration
src/
  main.cpp                  application entry point and frame loop
  ElementData.cpp/.h        element symbols, colors, radii, metadata
  app/                      editor coordination, file handling, interaction glue
  camera/                   orbit camera and callbacks
  graphics/                 renderer, scene buffers, picking, meshes, shaders
  io/                       structure loading through Open Babel
  math/                     structure and geometry helpers
  ui/                       Dear ImGui windows, dialogs, and builders
imgui/                      bundled Dear ImGui sources and backend bindings
```
