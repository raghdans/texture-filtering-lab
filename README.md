# Texture Filtering Lab

An incremental computer-graphics project for implementing and comparing
texture filtering methods.

## Current state: iteration 7

The program creates a small checkerboard texture in memory and enlarges it with
manually implemented nearest-neighbor and bilinear samplers. It supports each
filter separately and a side-by-side comparison mode. MiniFB,
the same display library used by the course's HW1 project, presents the pixel
buffer in a window. MiniFB is only the presentation layer; the sampling
algorithm is implemented in `src/main.cpp`.

The project does **not** yet contain mipmaps, trilinear filtering, camera
controls, or performance measurements. Those belong to later iterations.

## Build on Windows

Requirements:

- CMake 3.20 or newer
- A C++17 compiler, such as Visual Studio 2026

Commands from the project directory:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\texture_filtering_lab.exe
```

Press `1/N` for nearest-neighbor, `2/B` for bilinear, `3/S` for the split
comparison, `4/M` for the minification experiment, or Escape to close the
program. Press `5/P` to compare direct bilinear minification with a manually
generated 32 x 32 mipmap level.
Press `6/L` to display the complete mipmap chain from 64 x 64 to 1 x 1.
Press `7/A` to demonstrate automatic mip-level selection for four output sizes.

MiniFB is stored inside `external/minifb` under its MIT license. The project
does not need Python or downloaded dependencies.

## What to understand

1. The texture is a 64 x 64 array of packed RGB pixels.
2. The framebuffer is a separate 960 x 640 pixel array.
3. Each output pixel is assigned normalized `(u, v)` coordinates.
4. `sampleNearest` converts `(u, v)` into one integer texel coordinate.
5. Clamping prevents sampling outside the texture array.
6. MiniFB displays the completed framebuffer but does not perform filtering.

7. Split mode uses the same tested samplers and changes only the framebuffer
   layout.
