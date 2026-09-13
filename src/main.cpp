#include "MiniFB.h"

#include <algorithm>
#include <array>
#include <cmath>
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

int gRequestedFilter = 0; // 0 = nearest, 1 = bilinear
bool gQuitRequested = false;

void keyboardCallback(mfb_window*, mfb_key key, mfb_key_mod, bool isPressed)
{
    if (!isPressed) {
        return;
    }

    if (key == KB_KEY_1 || key == KB_KEY_N) {
        gRequestedFilter = 0;
    } else if (key == KB_KEY_2 || key == KB_KEY_B) {
        gRequestedFilter = 1;
    } else if (key == KB_KEY_ESCAPE) {
        gQuitRequested = true;
    }
}

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

std::uint8_t colorChannel(std::uint32_t color, int shift)
{
    return static_cast<std::uint8_t>((color >> shift) & 0xffU);
}

std::uint8_t interpolateChannel(
    std::uint8_t c00,
    std::uint8_t c10,
    std::uint8_t c01,
    std::uint8_t c11,
    float tx,
    float ty)
{
    const float top = static_cast<float>(c00) * (1.0F - tx) +
                      static_cast<float>(c10) * tx;
    const float bottom = static_cast<float>(c01) * (1.0F - tx) +
                         static_cast<float>(c11) * tx;
    return static_cast<std::uint8_t>(
        std::lround(top * (1.0F - ty) + bottom * ty));
}

std::uint32_t sampleBilinear(const Texture& texture, float u, float v)
{
    u = std::clamp(u, 0.0F, 1.0F);
    v = std::clamp(v, 0.0F, 1.0F);

    const float textureX = u * static_cast<float>(kTextureSize - 1);
    const float textureY = v * static_cast<float>(kTextureSize - 1);
    const int x0 = static_cast<int>(std::floor(textureX));
    const int y0 = static_cast<int>(std::floor(textureY));
    const int x1 = std::min(x0 + 1, kTextureSize - 1);
    const int y1 = std::min(y0 + 1, kTextureSize - 1);
    const float tx = textureX - static_cast<float>(x0);
    const float ty = textureY - static_cast<float>(y0);

    const auto texel = [&texture](int x, int y) {
        return texture[static_cast<std::size_t>(y * kTextureSize + x)];
    };
    const std::uint32_t c00 = texel(x0, y0);
    const std::uint32_t c10 = texel(x1, y0);
    const std::uint32_t c01 = texel(x0, y1);
    const std::uint32_t c11 = texel(x1, y1);

    const std::uint8_t red = interpolateChannel(
        colorChannel(c00, 16), colorChannel(c10, 16),
        colorChannel(c01, 16), colorChannel(c11, 16), tx, ty);
    const std::uint8_t green = interpolateChannel(
        colorChannel(c00, 8), colorChannel(c10, 8),
        colorChannel(c01, 8), colorChannel(c11, 8), tx, ty);
    const std::uint8_t blue = interpolateChannel(
        colorChannel(c00, 0), colorChannel(c10, 0),
        colorChannel(c01, 0), colorChannel(c11, 0), tx, ty);

    return MFB_RGB(red, green, blue);
}

void renderTexture(
    const Texture& texture,
    Framebuffer& framebuffer,
    bool useBilinear)
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
                framebufferY * kWindowWidth + framebufferX)] = useBilinear
                    ? sampleBilinear(texture, u, v)
                    : sampleNearest(texture, u, v);
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
    bool useBilinear = false;
    renderTexture(texture, framebuffer, useBilinear);

    mfb_window* window = mfb_open_ex(
        "Texture Filtering Lab - Iteration 1: Nearest Neighbor",
        kWindowWidth,
        kWindowHeight,
        MFB_WF_RESIZABLE);
    if (window == nullptr) {
        std::cerr << "Failed to create the MiniFB window.\n";
        return 1;
    }

    std::cout << "Iteration 2 compares nearest-neighbor and bilinear sampling.\n";
    std::cout << "Press 1/N for Nearest, 2/B for Bilinear, or Escape to exit.\n";
    mfb_set_keyboard_callback(window, keyboardCallback);

    while (mfb_update_events(window) != MFB_STATE_EXIT) {
        if (gQuitRequested) {
            break;
        }

        if (gRequestedFilter == 0 && useBilinear) {
            useBilinear = false;
            renderTexture(texture, framebuffer, useBilinear);
            mfb_set_title(window, "Texture Filtering Lab - Nearest Neighbor [1/N]");
            std::cout << "Filter: Nearest Neighbor\n";
        }
        if (gRequestedFilter == 1 && !useBilinear) {
            useBilinear = true;
            renderTexture(texture, framebuffer, useBilinear);
            mfb_set_title(window, "Texture Filtering Lab - Bilinear [2/B]");
            std::cout << "Filter: Bilinear\n";
        }

        if (mfb_update_ex(
                window,
                framebuffer.data(),
                kWindowWidth,
                kWindowHeight) < 0) {
            break;
        }
        mfb_wait_sync(window);
    }

    mfb_close(window);
    return 0;
}
