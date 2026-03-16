# Atoms Editor

An OpenGL-based molecular structure viewer and editor using Dear ImGui and Open Babel.

## Prerequisites

On Debian/Ubuntu (or derivatives):

```bash
sudo apt update
sudo apt install build-essential cmake libglfw3-dev libglew-dev libglm-dev \
                 libopenbabel-dev libopenbabel3
```

> **Note:** This project uses OpenGL 3.0+ (GLSL 130) and GLFW.

## Build

From the `src/` directory:

```bash
make
```

Then run:

```bash
./atoms_editor
```

## Usage

### Opening files

Use **File → Open…** (`Ctrl+O`) to browse and load a structure file.  
Supported formats: `.cif`, `.mol`, `.pdb`, `.xyz`, `.sdf`.

### Camera

| Action | Input |
|---|---|
| Rotate | Left drag |
| Pan | Right drag |
| Zoom | Scroll wheel |

### Atom selection

| Action | Input |
|---|---|
| Select atom | Left click |
| Add / remove from selection | Ctrl + click |
| Select all | Ctrl+A |
| Deselect all | Ctrl+D or Escape |
| Delete selected | Delete |

### Right-click context menu

- **Substitute Atom…** — replace all selected atoms with a chosen element (periodic table picker)
- **Insert Atom at Midpoint…** — insert a new atom at the centroid of ≥ 2 selected atoms
- **Delete** / **Deselect**

### Edit menu

- **Atomic Sizes…** — adjust per-element covalent radii (literature defaults: Cordero et al., *Dalton Trans.* 2008)
- **Element Colors…** — override CPK colours per element
- **Transform Atoms…** — apply a 3×3 matrix transformation to all atom positions (only available when the structure has a unit cell)

## Project layout

```
src/
  main.cpp                   — application entry point, render loop
  ElementData.cpp/.h         — element symbols, radii, colours
  camera/                    — camera orbit controller
  graphics/                  — OpenGL renderer, scene buffers, picking, shaders
  math/                      — math utilities
  ui/                        — Dear ImGui panels and dialogs
  io/                        — structure loading (Open Babel)
imgui/                       — bundled Dear ImGui sources
backends/                    — ImGui GLFW / OpenGL backends
```
