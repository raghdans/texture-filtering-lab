#include "MiniFB.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 640;
constexpr int kTextureSize = 64;
constexpr int kDisplaySize = 512;
constexpr int kComparisonSize = 384;
constexpr int kComparisonGap = 24;
constexpr int kCheckerSize = 8;
constexpr int kMinifiedSize = 32;
constexpr int kAutoPreviewSize = 240;
constexpr int kAutoPreviewGap = 20;

using Texture = std::array<std::uint32_t, kTextureSize * kTextureSize>;
using Framebuffer = std::array<std::uint32_t, kWindowWidth * kWindowHeight>;

struct MipLevel {
    int size;
    std::vector<std::uint32_t> pixels;
};

using MipChain = std::vector<MipLevel>;

int gRequestedMode = 0; // modes 0-6 are fixed views; mode 7 is interactive
int gRequestedOutputSize = 256;
bool gQuitRequested = false;

void keyboardCallback(mfb_window*, mfb_key key, mfb_key_mod, bool isPressed)
{
    if (!isPressed) {
        return;
    }

    if (key == KB_KEY_1 || key == KB_KEY_N) {
        gRequestedMode = 0;
    } else if (key == KB_KEY_2 || key == KB_KEY_B) {
        gRequestedMode = 1;
    } else if (key == KB_KEY_3 || key == KB_KEY_S) {
        gRequestedMode = 2;
    } else if (key == KB_KEY_4 || key == KB_KEY_M) {
        gRequestedMode = 3;
    } else if (key == KB_KEY_5 || key == KB_KEY_P) {
        gRequestedMode = 4;
    } else if (key == KB_KEY_6 || key == KB_KEY_L) {
        gRequestedMode = 5;
    } else if (key == KB_KEY_7 || key == KB_KEY_A) {
        gRequestedMode = 6;
    } else if (key == KB_KEY_8 || key == KB_KEY_I) {
        gRequestedMode = 7;
    } else if (key == KB_KEY_9 || key == KB_KEY_R) {
        gRequestedMode = 8;
    } else if (key == KB_KEY_UP && gRequestedMode == 7) {
        gRequestedOutputSize = std::min(gRequestedOutputSize * 2, 512);
    } else if (key == KB_KEY_DOWN && gRequestedMode == 7) {
        gRequestedOutputSize = std::max(gRequestedOutputSize / 2, 8);
    } else if (key == KB_KEY_ESCAPE) {
        gQuitRequested = true;
    }
}

Texture makeCheckerboard(int checkerSize)
{
    Texture texture{};
    for (int y = 0; y < kTextureSize; ++y) {
        for (int x = 0; x < kTextureSize; ++x) {
            const bool light = ((x / checkerSize) + (y / checkerSize)) % 2 == 0;
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

std::uint32_t averageFour(const std::uint32_t* colors)
{
    int red = 0;
    int green = 0;
    int blue = 0;
    for (int index = 0; index < 4; ++index) {
        red += colorChannel(colors[index], 16);
        green += colorChannel(colors[index], 8);
        blue += colorChannel(colors[index], 0);
    }
    return MFB_RGB(red / 4, green / 4, blue / 4);
}

MipChain makeMipChain(const Texture& texture)
{
    MipChain chain;
    chain.push_back({kTextureSize, {texture.begin(), texture.end()}});

    while (chain.back().size > 1) {
        const MipLevel& source = chain.back();
        const int destinationSize = source.size / 2;
        MipLevel destination{
            destinationSize,
            std::vector<std::uint32_t>(
                static_cast<std::size_t>(destinationSize * destinationSize))};

        for (int y = 0; y < destinationSize; ++y) {
            for (int x = 0; x < destinationSize; ++x) {
                const int sourceX = x * 2;
                const int sourceY = y * 2;
                const std::uint32_t colors[4] = {
                    source.pixels[static_cast<std::size_t>(
                        sourceY * source.size + sourceX)],
                    source.pixels[static_cast<std::size_t>(
                        sourceY * source.size + sourceX + 1)],
                    source.pixels[static_cast<std::size_t>(
                        (sourceY + 1) * source.size + sourceX)],
                    source.pixels[static_cast<std::size_t>(
                        (sourceY + 1) * source.size + sourceX + 1)]};
                destination.pixels[static_cast<std::size_t>(
                    y * destinationSize + x)] = averageFour(colors);
            }
        }
        chain.push_back(std::move(destination));
    }
    return chain;
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

void renderPanel(
    const Texture& texture,
    Framebuffer& framebuffer,
    int left,
    int top,
    int displaySize,
    int samplingGridSize,
    bool useBilinear)
{
    for (int screenY = 0; screenY < displaySize; ++screenY) {
        for (int screenX = 0; screenX < displaySize; ++screenX) {
            const int sampleX = screenX * samplingGridSize / displaySize;
            const int sampleY = screenY * samplingGridSize / displaySize;
            const float u = static_cast<float>(sampleX) /
                            static_cast<float>(samplingGridSize - 1);
            const float v = static_cast<float>(sampleY) /
                            static_cast<float>(samplingGridSize - 1);

            const int framebufferX = left + screenX;
            const int framebufferY = top + screenY;
            framebuffer[static_cast<std::size_t>(
                framebufferY * kWindowWidth + framebufferX)] = useBilinear
                    ? sampleBilinear(texture, u, v)
                    : sampleNearest(texture, u, v);
        }
    }
}

void renderMipPanel(
    const MipLevel& mip,
    Framebuffer& framebuffer,
    int left,
    int top,
    int displaySize)
{
    for (int screenY = 0; screenY < displaySize; ++screenY) {
        for (int screenX = 0; screenX < displaySize; ++screenX) {
            const int mipX = screenX * mip.size / displaySize;
            const int mipY = screenY * mip.size / displaySize;
            framebuffer[static_cast<std::size_t>(
                (top + screenY) * kWindowWidth + left + screenX)] =
                mip.pixels[static_cast<std::size_t>(mipY * mip.size + mipX)];
        }
    }
}

std::size_t chooseMipLevel(const MipChain& chain, int outputSize)
{
    std::size_t level = 0;
    while (level + 1 < chain.size() &&
           chain[level + 1].size >= outputSize) {
        ++level;
    }
    return level;
}

std::uint32_t sampleMipBilinear(const MipLevel& mip, float u, float v)
{
    u = std::clamp(u, 0.0F, 1.0F);
    v = std::clamp(v, 0.0F, 1.0F);
    const float textureX = u * static_cast<float>(mip.size - 1);
    const float textureY = v * static_cast<float>(mip.size - 1);
    const int x0 = static_cast<int>(std::floor(textureX));
    const int y0 = static_cast<int>(std::floor(textureY));
    const int x1 = std::min(x0 + 1, mip.size - 1);
    const int y1 = std::min(y0 + 1, mip.size - 1);
    const float tx = textureX - static_cast<float>(x0);
    const float ty = textureY - static_cast<float>(y0);
    const auto texel = [&mip](int x, int y) {
        return mip.pixels[static_cast<std::size_t>(y * mip.size + x)];
    };
    const std::uint32_t c00 = texel(x0, y0);
    const std::uint32_t c10 = texel(x1, y0);
    const std::uint32_t c01 = texel(x0, y1);
    const std::uint32_t c11 = texel(x1, y1);
    return MFB_RGB(
        interpolateChannel(
            colorChannel(c00, 16), colorChannel(c10, 16),
            colorChannel(c01, 16), colorChannel(c11, 16), tx, ty),
        interpolateChannel(
            colorChannel(c00, 8), colorChannel(c10, 8),
            colorChannel(c01, 8), colorChannel(c11, 8), tx, ty),
        interpolateChannel(
            colorChannel(c00, 0), colorChannel(c10, 0),
            colorChannel(c01, 0), colorChannel(c11, 0), tx, ty));
}

void renderPerspectiveComparison(
    const Texture& texture,
    const MipChain& chain,
    Framebuffer& framebuffer)
{
    constexpr int panelWidth = 420;
    constexpr int surfaceHeight = 480;
    constexpr int topWidth = 8;
    constexpr int bottomWidth = 360;
    constexpr int gap = 24;
    const int firstPanelLeft = (kWindowWidth - 2 * panelWidth - gap) / 2;
    const int surfaceTop = (kWindowHeight - surfaceHeight) / 2;

    for (int panel = 0; panel < 2; ++panel) {
        const int panelLeft = firstPanelLeft + panel * (panelWidth + gap);
        const int centerX = panelLeft + panelWidth / 2;
        for (int y = 0; y < surfaceHeight; ++y) {
            const float depth = static_cast<float>(y) /
                                static_cast<float>(surfaceHeight - 1);
            const int rowWidth = topWidth + static_cast<int>(
                depth * depth * static_cast<float>(bottomWidth - topWidth));
            const int rowLeft = centerX - rowWidth / 2;
            const std::size_t mipLevel = chooseMipLevel(chain, rowWidth);

            for (int x = 0; x < rowWidth; ++x) {
                const float u = rowWidth == 1
                    ? 0.0F
                    : static_cast<float>(x) / static_cast<float>(rowWidth - 1);
                const float v = depth;
                const std::uint32_t color = panel == 0
                    ? sampleBilinear(texture, u, v)
                    : sampleMipBilinear(chain[mipLevel], u, v);
                framebuffer[static_cast<std::size_t>(
                    (surfaceTop + y) * kWindowWidth + rowLeft + x)] = color;
            }
        }
    }
}

void renderAutomaticMipSelection(
    const MipChain& chain,
    Framebuffer& framebuffer)
{
    const int requestedSizes[] = {64, 32, 16, 8};
    const int totalSize = 2 * kAutoPreviewSize + kAutoPreviewGap;
    const int startX = (kWindowWidth - totalSize) / 2;
    const int startY = (kWindowHeight - totalSize) / 2;

    for (int index = 0; index < 4; ++index) {
        const std::size_t level = chooseMipLevel(chain, requestedSizes[index]);
        const int column = index % 2;
        const int row = index / 2;
        renderMipPanel(
            chain[level],
            framebuffer,
            startX + column * (kAutoPreviewSize + kAutoPreviewGap),
            startY + row * (kAutoPreviewSize + kAutoPreviewGap),
            kAutoPreviewSize);
    }
}

std::array<std::uint8_t, 7> glyph(char character)
{
    switch (character) {
    case 'A': return {14, 17, 17, 31, 17, 17, 17};
    case 'B': return {30, 17, 17, 30, 17, 17, 30};
    case 'C': return {14, 17, 16, 16, 16, 17, 14};
    case 'E': return {31, 16, 16, 30, 16, 16, 31};
    case 'F': return {31, 16, 16, 30, 16, 16, 16};
    case 'I': return {31, 4, 4, 4, 4, 4, 31};
    case 'L': return {16, 16, 16, 16, 16, 16, 31};
    case 'M': return {17, 27, 21, 21, 17, 17, 17};
    case 'N': return {17, 25, 21, 19, 17, 17, 17};
    case 'O': return {14, 17, 17, 17, 17, 17, 14};
    case 'P': return {30, 17, 17, 30, 16, 16, 16};
    case 'R': return {30, 17, 17, 30, 20, 18, 17};
    case 'S': return {15, 16, 16, 14, 1, 1, 30};
    case 'T': return {31, 4, 4, 4, 4, 4, 4};
    case 'U': return {17, 17, 17, 17, 17, 17, 14};
    case 'V': return {17, 17, 17, 17, 17, 10, 4};
    default: return {0, 0, 0, 0, 0, 0, 0};
    }
}

void drawText(
    Framebuffer& framebuffer,
    int left,
    int top,
    const std::string& text,
    int scale,
    std::uint32_t color)
{
    int characterLeft = left;
    for (char character : text) {
        const auto rows = glyph(character);
        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if ((rows[row] & (1U << (4 - column))) == 0) {
                    continue;
                }
                for (int pixelY = 0; pixelY < scale; ++pixelY) {
                    for (int pixelX = 0; pixelX < scale; ++pixelX) {
                        const int x = characterLeft + column * scale + pixelX;
                        const int y = top + row * scale + pixelY;
                        if (x >= 0 && x < kWindowWidth &&
                            y >= 0 && y < kWindowHeight) {
                            framebuffer[static_cast<std::size_t>(
                                y * kWindowWidth + x)] = color;
                        }
                    }
                }
            }
        }
        characterLeft += 6 * scale;
    }
}

void drawCenteredLabel(
    Framebuffer& framebuffer,
    int centerX,
    int top,
    const std::string& text)
{
    constexpr int scale = 2;
    const int width = static_cast<int>(text.size()) * 6 * scale - scale;
    drawText(
        framebuffer,
        centerX - width / 2 + 2,
        top + 2,
        text,
        scale,
        MFB_RGB(15, 18, 24));
    drawText(
        framebuffer,
        centerX - width / 2,
        top,
        text,
        scale,
        MFB_RGB(235, 235, 235));
}

void renderMode(
    const Texture& texture,
    const Texture& denseTexture,
    const MipChain& denseMipChain,
    const MipChain& regularMipChain,
    Framebuffer& framebuffer,
    int mode,
    int interactiveOutputSize)
{
    framebuffer.fill(MFB_RGB(45, 49, 58));

    if (mode == 8) {
        renderPerspectiveComparison(texture, regularMipChain, framebuffer);
        drawCenteredLabel(
            framebuffer, kWindowWidth / 2, 10, "PERSPECTIVE COMPARISON");
        drawCenteredLabel(framebuffer, 258, 42, "BILINEAR");
        drawCenteredLabel(framebuffer, 702, 42, "MIPMAP");
        return;
    }

    if (mode == 7) {
        const std::size_t level =
            chooseMipLevel(regularMipChain, interactiveOutputSize);
        const int left = (kWindowWidth - interactiveOutputSize) / 2;
        const int top = (kWindowHeight - interactiveOutputSize) / 2;
        renderMipPanel(
            regularMipChain[level],
            framebuffer,
            left,
            top,
            interactiveOutputSize);
        drawCenteredLabel(
            framebuffer, kWindowWidth / 2, 20, "INTERACTIVE MIPMAP");
        return;
    }

    if (mode == 6) {
        renderAutomaticMipSelection(regularMipChain, framebuffer);
        drawCenteredLabel(
            framebuffer, kWindowWidth / 2, 20, "AUTOMATIC MIPMAP");
        return;
    }

    if (mode == 5) {
        const int previewSizes[] = {256, 160, 96, 56, 32, 16, 8};
        int left = 70;
        for (std::size_t level = 0; level < regularMipChain.size(); ++level) {
            const int previewSize = previewSizes[level];
            const int top = (kWindowHeight - previewSize) / 2;
            renderMipPanel(
                regularMipChain[level], framebuffer, left, top, previewSize);
            left += previewSize + 24;
        }
        drawCenteredLabel(
            framebuffer, kWindowWidth / 2, 20, "MIPMAP LEVELS");
        return;
    }

    if (mode == 4) {
        const int totalWidth = 2 * kComparisonSize + kComparisonGap;
        const int left = (kWindowWidth - totalWidth) / 2;
        const int top = (kWindowHeight - kComparisonSize) / 2;
        renderPanel(
            denseTexture,
            framebuffer,
            left,
            top,
            kComparisonSize,
            kMinifiedSize,
            true);
        renderMipPanel(
            denseMipChain[1],
            framebuffer,
            left + kComparisonSize + kComparisonGap,
            top,
            kComparisonSize);
        drawCenteredLabel(
            framebuffer, left + kComparisonSize / 2, top - 34, "BILINEAR");
        drawCenteredLabel(
            framebuffer,
            left + kComparisonSize + kComparisonGap + kComparisonSize / 2,
            top - 34,
            "MIPMAP");
        drawCenteredLabel(
            framebuffer, kWindowWidth / 2, 20, "MIPMAP COMPARISON");
        return;
    }

    if (mode == 2 || mode == 3) {
        const int totalWidth = 2 * kComparisonSize + kComparisonGap;
        const int left = (kWindowWidth - totalWidth) / 2;
        const int top = (kWindowHeight - kComparisonSize) / 2;
        const Texture& selectedTexture = mode == 3 ? denseTexture : texture;
        const int samplingGridSize =
            mode == 3 ? kMinifiedSize : kComparisonSize;
        renderPanel(
            selectedTexture,
            framebuffer,
            left,
            top,
            kComparisonSize,
            samplingGridSize,
            false);
        renderPanel(
            selectedTexture,
            framebuffer,
            left + kComparisonSize + kComparisonGap,
            top,
            kComparisonSize,
            samplingGridSize,
            true);
        drawCenteredLabel(
            framebuffer, left + kComparisonSize / 2, top - 34, "NEAREST");
        drawCenteredLabel(
            framebuffer,
            left + kComparisonSize + kComparisonGap + kComparisonSize / 2,
            top - 34,
            "BILINEAR");
        drawCenteredLabel(
            framebuffer,
            kWindowWidth / 2,
            20,
            mode == 3 ? "MINIFICATION" : "FILTER COMPARISON");
        return;
    }

    const int left = (kWindowWidth - kDisplaySize) / 2;
    const int top = (kWindowHeight - kDisplaySize) / 2;
    renderPanel(
        texture,
        framebuffer,
        left,
        top,
        kDisplaySize,
        kDisplaySize,
        mode == 1);
    drawCenteredLabel(
        framebuffer,
        kWindowWidth / 2,
        20,
        mode == 1 ? "BILINEAR" : "NEAREST");
}

} // namespace

int main()
{
    const Texture texture = makeCheckerboard(kCheckerSize);
    const Texture denseTexture = makeCheckerboard(1);
    const MipChain denseMipChain = makeMipChain(denseTexture);
    const MipChain regularMipChain = makeMipChain(texture);
    // A 960 x 640 RGBA framebuffer is about 2.5 MB, larger than the default
    // Windows stack. Static storage prevents a stack-overflow crash at startup.
    static Framebuffer framebuffer{};
    int currentMode = 0;
    int currentOutputSize = gRequestedOutputSize;
    renderMode(
        texture,
        denseTexture,
        denseMipChain,
        regularMipChain,
        framebuffer,
        currentMode,
        gRequestedOutputSize);

    mfb_window* window = mfb_open_ex(
        "Texture Filtering Lab - Nearest Neighbor [1/N]",
        kWindowWidth,
        kWindowHeight,
        MFB_WF_RESIZABLE);
    if (window == nullptr) {
        std::cerr << "Failed to create the MiniFB window.\n";
        return 1;
    }

    std::cout << "Iteration 9 adds a perspective minification comparison.\n";
    std::cout << "Press 1/N for Nearest, 2/B for Bilinear, 3/S for Split,"
                 " 4/M for Minification, 5/P for Mipmap, 6/L for Levels,"
                 " 7/A for Auto, 8/I for Interactive, 9/R for Perspective,"
                 " or Escape to exit.\n";
    std::cout << "In Interactive mode, use Up/Down to change output size.\n";
    mfb_set_keyboard_callback(window, keyboardCallback);

    while (mfb_update_events(window) != MFB_STATE_EXIT) {
        if (gQuitRequested) {
            break;
        }

        const bool modeChanged = gRequestedMode != currentMode;
        const bool sizeChanged =
            currentMode == 7 && gRequestedOutputSize != currentOutputSize;
        if (modeChanged || sizeChanged) {
            currentMode = gRequestedMode;
            currentOutputSize = gRequestedOutputSize;
            renderMode(
                texture,
                denseTexture,
                denseMipChain,
                regularMipChain,
                framebuffer,
                currentMode,
                currentOutputSize);
            if (currentMode == 0) {
                mfb_set_title(window, "Texture Filtering Lab - Nearest Neighbor [1/N]");
                std::cout << "Mode: Nearest Neighbor\n";
            } else if (currentMode == 1) {
                mfb_set_title(window, "Texture Filtering Lab - Bilinear [2/B]");
                std::cout << "Mode: Bilinear\n";
            } else if (currentMode == 2) {
                mfb_set_title(
                    window,
                    "Texture Filtering Lab - Split: Nearest | Bilinear [3/S]");
                std::cout << "Mode: Split comparison (Nearest | Bilinear)\n";
            } else if (currentMode == 3) {
                mfb_set_title(
                    window,
                    "Texture Filtering Lab - Minification 64x64 to 32x32 [4/M]");
                std::cout << "Mode: Minification preview (Nearest | Bilinear)\n";
            } else if (currentMode == 4) {
                mfb_set_title(
                    window,
                    "Texture Filtering Lab - Bilinear Base | Mipmap 32x32 [5/P]");
                std::cout << "Mode: Bilinear base texture | averaged mipmap\n";
            } else if (currentMode == 5) {
                mfb_set_title(
                    window,
                    "Texture Filtering Lab - Mipmap Chain 64 to 1 [6/L]");
                std::cout << "Mode: Mipmap levels 64, 32, 16, 8, 4, 2, 1\n";
            } else if (currentMode == 6) {
                mfb_set_title(
                    window,
                    "Texture Filtering Lab - Automatic Mipmap Selection [7/A]");
                std::cout << "Automatic choices: output 64 -> level 64, "
                             "32 -> 32, 16 -> 16, 8 -> 8\n";
            } else if (currentMode == 7) {
                const std::size_t level =
                    chooseMipLevel(regularMipChain, currentOutputSize);
                const int mipSize = regularMipChain[level].size;
                const std::string title =
                    "Texture Filtering Lab - Output " +
                    std::to_string(currentOutputSize) + " | Mip " +
                    std::to_string(mipSize) + " [Up/Down]";
                mfb_set_title(window, title.c_str());
                std::cout << "Interactive: output " << currentOutputSize
                          << " -> mip level " << mipSize << "\n";
            } else {
                mfb_set_title(
                    window,
                    "Texture Filtering Lab - Perspective: Bilinear | Mipmap [9/R]");
                std::cout << "Perspective: direct bilinear left | automatic mipmap right\n";
            }
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
