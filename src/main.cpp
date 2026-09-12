#include "MiniFB.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>

namespace {

constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 640;
constexpr int kTextureSize = 64;
constexpr int kDisplaySize = 512;
constexpr int kCheckerSize = 8;

using Texture = std::array<std::uint32_t, kTextureSize * kTextureSize>;
using Framebuffer = std::array<std::uint32_t, kWindowWidth * kWindowHeight>;

Texture makeCheckerboard()
{
    Texture texture{};
    for (int y = 0; y < kTextureSize; ++y) {
        for (int x = 0; x < kTextureSize; ++x) {
            const bool light = ((x / kCheckerSize) + (y / kCheckerSize)) % 2 == 0;
            texture[static_cast<std::size_t>(y * kTextureSize + x)] =
                light ? MFB_RGB(235, 235, 235) : MFB_RGB(35, 75, 145);
        }
    }
    return texture;
}

std::uint32_t sampleNearest(const Texture& texture, float u, float v)
{
    u = std::clamp(u, 0.0F, 1.0F);
    v = std::clamp(v, 0.0F, 1.0F);

    const int x = std::min(
        static_cast<int>(u * static_cast<float>(kTextureSize)),
        kTextureSize - 1);
    const int y = std::min(
        static_cast<int>(v * static_cast<float>(kTextureSize)),
        kTextureSize - 1);

    return texture[static_cast<std::size_t>(y * kTextureSize + x)];
}

void renderNearest(const Texture& texture, Framebuffer& framebuffer)
{
    framebuffer.fill(MFB_RGB(45, 49, 58));

    const int left = (kWindowWidth - kDisplaySize) / 2;
    const int top = (kWindowHeight - kDisplaySize) / 2;

    for (int screenY = 0; screenY < kDisplaySize; ++screenY) {
        for (int screenX = 0; screenX < kDisplaySize; ++screenX) {
            const float u = static_cast<float>(screenX) /
                            static_cast<float>(kDisplaySize - 1);
            const float v = static_cast<float>(screenY) /
                            static_cast<float>(kDisplaySize - 1);

            const int framebufferX = left + screenX;
            const int framebufferY = top + screenY;
            framebuffer[static_cast<std::size_t>(
                framebufferY * kWindowWidth + framebufferX)] =
                sampleNearest(texture, u, v);
        }
    }
}

} // namespace

int main()
{
    const Texture texture = makeCheckerboard();
    // A 960 x 640 RGBA framebuffer is about 2.5 MB, larger than the default
    // Windows stack. Static storage prevents a stack-overflow crash at startup.
    static Framebuffer framebuffer{};
    renderNearest(texture, framebuffer);

    mfb_window* window = mfb_open_ex(
        "Texture Filtering Lab - Iteration 1: Nearest Neighbor",
        kWindowWidth,
        kWindowHeight,
        MFB_WF_RESIZABLE);
    if (window == nullptr) {
        std::cerr << "Failed to create the MiniFB window.\n";
        return 1;
    }

    std::cout << "Iteration 1 uses nearest-neighbor texture sampling.\n";
    std::cout << "Press Escape or close the window to exit.\n";

    while (mfb_update_ex(
               window,
               framebuffer.data(),
               kWindowWidth,
               kWindowHeight) >= 0) {
        const std::uint8_t* keys = mfb_get_key_buffer(window);
        if (keys != nullptr && keys[KB_KEY_ESCAPE]) {
            break;
        }
        mfb_wait_sync(window);
    }

    mfb_close(window);
    return 0;
}
