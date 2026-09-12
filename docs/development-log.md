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
This visually verifies iteration 1. A Git commit is intentionally deferred
until the student has reviewed and can explain the implementation.

### Student review checklist

- [ ] I can explain why the square requires two triangles.
- [ ] I can identify the four values stored for each vertex.
- [ ] I can explain what the vertex shader sends to the fragment shader.
- [ ] I can explain what `texture(checkerTexture, uv)` returns.
- [ ] I built the project successfully.
- [ ] I saw the checkerboard and saved a screenshot.
- [ ] I recorded any error and the change that fixed it.
