# Development log

This log records the real development process. It should be updated only after
an iteration has actually been built and tested.

## Iteration 1 - textured square with nearest filtering

### Goal

Establish the smallest complete texture pipeline before comparing filtering
methods.

### AI prompt

> Create only the first iteration of a C++17 OpenGL 3.3 texture-filtering lab.
> Display a square with UV coordinates and a procedural checkerboard texture.
> Use nearest-neighbor filtering only. Keep the shaders and data flow explicit,
> and do not implement later comparison features yet.

### Design decisions

- A procedural checkerboard avoids adding image-loading complexity in the
  first iteration.
- A square makes the relationship between its four UV corners and the texture
  easy to inspect.
- Filtering is fixed to `GL_NEAREST` so the next iteration can introduce one
  controlled change.
- Shader compilation and linking errors are printed instead of silently
  failing.

### Verification status

The initial configuration reported that no compiler was available. Inspection
showed that Visual Studio and MSVC were installed; the actual cause was a
duplicate `Path`/`PATH` environment entry in the automated build environment.
After using a clean environment, CMake detected MSVC 19.51 successfully.

The next build compiled GLFW but GLAD generation failed because the Python
interpreter selected by CMake did not contain Jinja2. GLAD was generated with a
working Python environment and its generated C source and headers were then
stored in `external/glad`. This removes the Python/Jinja2 requirement from
future builds.

The Release executable compiled successfully on September 12, 2026. Its first
visual test opened a valid window but displayed only black. The initial code
registered a framebuffer-resize callback but did not set the viewport at
startup. Because the callback is not guaranteed to run during window creation,
the drawable viewport could remain empty. The code now queries the framebuffer
size and calls `glViewport` once immediately after loading OpenGL. A second
visual test still produced a black window, so the viewport hypothesis was not
sufficient. A diagnostic build now uses a clearly gray background, a distinct
`Iteration 1.1 Diagnostic` title, and prints the OpenGL version and renderer.
This separates a render-loop problem from a geometry/texture problem. The next
visual result again appeared black even though OpenGL 3.3 and the AMD renderer
were detected correctly. The next diagnostic build reads the center pixel back
from the framebuffer and prints its RGBA value together with the first OpenGL
error code. This will show whether rendering succeeds internally but is not
presented, or whether drawing fails before presentation.

The diagnostic result reported a white center pixel (`235, 235, 235, 255`) and
no OpenGL error (`0x0`) while the visible window remained black. This proves
that the checkerboard is rendered correctly into the back buffer. The next
diagnostic reads the same pixel from the front buffer after `glfwSwapBuffers`
to confirm whether buffer presentation is the failing operation.

The front-buffer test returned (`0, 0, 0, 0`) after the swap. This confirms a
presentation problem in the GLFW/AMD double-buffer path on the test machine.
Iteration 1.2 requests a single-buffered GLFW window and calls `glFlush` after
drawing. This is a temporary, explicit compatibility workaround for validating
the basic texture pipeline; double buffering should be revisited before later
camera animation is finalized.

The single-buffered GLFW test also remained visually black. As a control test,
the course's existing HW1 application was run on the same computer and its
MiniFB framebuffer displayed correctly. The project architecture was therefore
revised: MiniFB now presents a CPU pixel buffer, while the texture-sampling
methods are implemented manually in project code. This avoids the machine's
GLFW/AMD presentation issue and makes the comparison algorithmic rather than a
comparison of OpenGL configuration constants. Iteration 1 now implements only
nearest-neighbor sampling; bilinear and mipmap methods remain future work.

The first MiniFB link attempt reported an unresolved `release_cpp_stub`
symbol. Comparing the library sources with HW1 showed that `MiniFB_cpp.cpp`
was missing from the new target. It was added to the project before rebuilding.

The first executable then closed immediately with Windows exit code
`0xC00000FD`, which means stack overflow. The 960 x 640 framebuffer occupied
about 2.5 MB and had been declared as a local variable, exceeding the default
Windows stack. It was moved to static storage; the sampling algorithm was not
changed.

### Successful visual verification

After the storage fix, the application remained open and displayed the 8 x 8
blue-and-white checker pattern centered on a gray background. The enlarged
texels had sharp block boundaries, as expected from nearest-neighbor sampling.
This visually verified iteration 1, which was then saved as Git commit
`7999057`.

## Iteration 2 - nearest versus bilinear filtering

### Goal

Implement bilinear sampling manually and allow a direct visual comparison with
nearest-neighbor sampling using keys `1` and `2`.

### AI prompt

> Starting from the tested nearest-neighbor CPU sampler, add a bilinear sampler
> as a separate function. Interpolate the four surrounding texels per RGB
> channel, clamp boundary coordinates, and add keys 1 and 2 to switch methods.
> Do not add mipmaps or other filtering methods in this iteration.

### Implementation and test results

The first interaction test displayed the nearest image correctly, but pressing
`2` did not change the filter. The initial loop polled MiniFB's key-state buffer
without explicitly processing events first. Input handling was revised to
match the working HW1 structure: `mfb_update_events` runs at the start of each
frame and a keyboard callback records the requested filter. Keys `N/B` were
added alongside `1/2` to make the controls unambiguous.

The next visual test showed that switching worked, but the blue squares became
brown in bilinear mode. This exposed a color-channel ordering bug: on Windows,
MiniFB packs colors as `0x00RRGGBB`, while the sampler had initially read red
from the lowest byte and blue from the highest byte. The shifts were corrected.

The final visual test passed: `1/N` restored sharp nearest-neighbor boundaries,
`2/B` selected bilinear filtering, the boundaries became smoothly blended, and
the checkerboard remained blue and white. The console and window title also
reported each selected mode correctly.

### Student review checklist

- [ ] I can explain how nearest-neighbor chooses one texel.
- [ ] I can identify the four texels used by bilinear filtering.
- [ ] I can explain the roles of `tx` and `ty` in interpolation.
- [x] I built the project successfully.
- [x] I switched between both filters and saved a screenshot.
- [x] I recorded the input and color bugs and the changes that fixed them.

## Iteration 3 - simultaneous comparison

### Goal

Show nearest-neighbor and bilinear results side by side so their visual
difference can be inspected at the same moment.

### AI prompt

> Keep the two tested sampling functions unchanged. Add a third display mode
> that draws nearest-neighbor on the left and bilinear on the right, with a
> visible gap between them. Use `3` or `S` for this mode and retain the existing
> single-filter modes. Do not add mipmaps in this iteration.

### Implementation and test result

The framebuffer layout was generalized into a reusable `renderPanel` function.
Single-filter modes keep the original large image, while split mode draws two
384 x 384 panels separated by a gray gap. The sampling functions themselves
were not changed.

The student's screenshot verified that `3/S` activates split mode, with sharp
nearest-neighbor boundaries on the left and blended bilinear boundaries on the
right. The window title and console also identify the comparison correctly.

### Student review checklist

- [ ] I can explain why both panels use the same source texture.
- [ ] I can explain why only the right panel passes `true` to `renderPanel`.
- [x] I activated split mode and saved a screenshot.
- [x] I verified that the left and right panels look different as expected.

## Iteration 4 - texture minification experiment

### Goal

Demonstrate minification and aliasing before implementing mipmaps. A dense
64 x 64 checkerboard is sampled onto a logical 32 x 32 output. That small result
is enlarged only for inspection, so individual output pixels remain visible.

### AI prompt

> Add a separate dense checkerboard and a minification comparison mode. Sample
> the 64 x 64 texture onto a 32 x 32 logical grid using nearest-neighbor on the
> left and bilinear on the right, then enlarge those results for inspection.
> Keep all earlier modes available. Do not implement mipmaps yet.

### Implementation and test result

The dense texture alternates color at every texel. Each displayed block
represents one pixel of the 32 x 32 logical result, enlarged for inspection.

The student's screenshot verified the expected information loss. Nearest
neighbor collapsed the dense pattern into a few large false-color regions,
which is severe aliasing. Bilinear produced broad blended gradients instead of
the original fine pattern. This demonstrates that bilinear filtering alone
does not correctly average all texels covered by a minified output pixel and
motivates the mipmap iteration.

### Student review checklist

- [ ] I understand that the large blocks visualize a small 32 x 32 result.
- [ ] I understand why fine checker details disappear during minification.
- [x] I ran minification mode and saved a screenshot.
- [x] I observed that neither nearest nor bilinear preserves the dense pattern.
