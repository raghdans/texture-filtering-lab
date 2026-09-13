# Texture Filtering Lab

An interactive computer-graphics project that implements and compares texture
filtering and mipmapping algorithms.

## Final result

The program procedurally creates checkerboard textures and implements:

- nearest-neighbor sampling;
- bilinear sampling by interpolating four neighboring texels;
- texture-minification and aliasing demonstrations;
- a complete mipmap chain from 64 x 64 to 1 x 1;
- automatic mip-level selection based on output resolution;
- interactive output resizing with live mip-level selection.

MiniFB presents the completed CPU framebuffer in a Windows window. It does not
perform the filtering: all sampling, color interpolation, 2 x 2 averaging, and
mipmap selection are implemented in `src/main.cpp`.

## Controls

| Key | View |
| --- | --- |
| `1` or `N` | Nearest-neighbor sampling |
| `2` or `B` | Bilinear sampling |
| `3` or `S` | Nearest and bilinear side by side |
| `4` or `M` | Minification aliasing experiment |
| `5` or `P` | Direct bilinear minification versus a mipmap |
| `6` or `L` | Complete mipmap chain |
| `7` or `A` | Automatic selection for four output sizes |
| `8` or `I` | Interactive automatic selection |
| Up / Down | Resize in interactive mode |
| Escape | Exit |

The title and console report the active mode. Interactive mode also reports the
output size and selected mip resolution.

## Build on Windows

Requirements:

- CMake 3.20 or newer;
- a C++17 compiler, such as Visual Studio 2026 with Desktop development with
  C++ installed.

From the project directory:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\texture_filtering_lab.exe
```

MiniFB is included under `external/minifb` with its MIT license. No downloaded
runtime dependencies or image files are required.

## Concepts demonstrated

1. A texel is a pixel stored in a texture; a screen pixel belongs to the output.
2. Nearest-neighbor chooses one texel and produces sharp, block-like edges.
3. Bilinear filtering blends four surrounding texels.
4. Direct sampling can create false patterns during strong minification.
5. Each mipmap level averages 2 x 2 blocks from the preceding level.
6. Choosing a mip resolution close to the output reduces aliasing.

## Development evidence

The project was developed and visually verified in eight Git iterations. The
prompts, failed experiments, bugs, corrections, and test results are recorded
in `docs/development-log.md`. A short Hebrew explanation is available in
`docs/project-summary-he.md`.
