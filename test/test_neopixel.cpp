// Mac-side tester for the NeoPixel ring driver.
// Renders the ring's current physical-LED state to a PPM file so you can
// see what the real ring would look like after orientation correction.

#include "neopixel.h"
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <string>
#include <sys/stat.h>
#include <vector>

static const char* OUT_DIR = "test_output";

static void ensureOutDir() {
    mkdir(OUT_DIR, 0755);
}

static void writePpm(const std::string& path, int W, int H,
                     const std::vector<uint8_t>& rgb) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) { std::perror(path.c_str()); return; }
    std::fprintf(f, "P6\n%d %d\n255\n", W, H);
    std::fwrite(rgb.data(), 1, rgb.size(), f);
    std::fclose(f);
}

// Renders the PHYSICAL LED state — i.e. what the real ring would look like.
static void renderRing(const NeoPixel& ring, const std::string& outPath) {
    constexpr int W = 256;
    constexpr int H = 256;
    constexpr int CX = W / 2;
    constexpr int CY = H / 2;
    constexpr int RING_R = 100;
    constexpr int LED_R  = 16;

    std::vector<uint8_t> img(W * H * 3, 24);

    auto putPixel = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || x >= W || y < 0 || y >= H) return;
        int idx = (y * W + x) * 3;
        img[idx + 0] = r; img[idx + 1] = g; img[idx + 2] = b;
    };
    auto drawDisc = [&](int cx, int cy, int rad, uint8_t r, uint8_t g, uint8_t b) {
        for (int y = -rad; y <= rad; ++y)
            for (int x = -rad; x <= rad; ++x)
                if (x*x + y*y <= rad*rad) putPixel(cx + x, cy + y, r, g, b);
    };

    int n = ring.count();
    for (int phys = 0; phys < n; ++phys) {
        // Physical pixel `phys` is drawn at angle (2*pi*phys/n) starting at top, going clockwise.
        // (This is the *visualization* convention; it's the assumed physical hardware layout.)
        double theta = (2.0 * M_PI * phys) / n - M_PI / 2.0;
        int lx = CX + static_cast<int>(std::round(std::cos(theta) * RING_R));
        int ly = CY + static_cast<int>(std::round(std::sin(theta) * RING_R));

        uint8_t r, g, b;
        ring.getPhysicalPixel(phys, r, g, b);

        double br = ring.brightness();
        r = static_cast<uint8_t>(r * br);
        g = static_cast<uint8_t>(g * br);
        b = static_cast<uint8_t>(b * br);

        drawDisc(lx, ly, LED_R, r, g, b);
    }

    writePpm(outPath, W, H, img);
}

int main() {
    ensureOutDir();

    NeoPixel ring;
    if (!ring.begin(16)) {
        std::fprintf(stderr, "begin() failed\n");
        return 1;
    }
    ring.setBrightness(1.0);

    // ---- Color demos (default orientation: offset=0, clockwise) ----------
    ring.setOrientation(0, true);

    ring.clear();
    ring.setPixel(0, 255, 200, 60);
    ring.show();
    renderRing(ring, std::string(OUT_DIR) + "/ring_sun_top.ppm");

    ring.clear();
    ring.setPixel(2, 255, 102, 0);
    ring.show();
    renderRing(ring, std::string(OUT_DIR) + "/ring_sun_afternoon.ppm");

    ring.clear();
    for (int i = 12; i < 16; ++i) ring.setPixel(i, 80, 80, 255);
    for (int i = 0;  i < 4;  ++i) ring.setPixel(i, 80, 80, 255);
    ring.show();
    renderRing(ring, std::string(OUT_DIR) + "/ring_moon_half.ppm");

    ring.clear();
    for (int i = 0; i < 16; ++i) {
        double t = i / 16.0;
        ring.setPixel(i,
            static_cast<uint8_t>(255 * std::cos(t * 2 * M_PI) * 0.5 + 128),
            static_cast<uint8_t>(255 * std::cos((t + 0.33) * 2 * M_PI) * 0.5 + 128),
            static_cast<uint8_t>(255 * std::cos((t + 0.66) * 2 * M_PI) * 0.5 + 128));
    }
    ring.setBrightness(0.4);
    ring.show();
    renderRing(ring, std::string(OUT_DIR) + "/ring_rainbow.ppm");

    // ---- Orientation demo ----
    // Keep the SAME logical state (LED 0 = red) but vary orientation.
    // The real ring would show LED 0 in different physical spots.
    ring.setBrightness(1.0);
    ring.clear();
    ring.setPixel(0, 255, 0, 0);    // logical 0 = red
    ring.setPixel(4, 0, 255, 0);    // logical 4 = green
    ring.setPixel(8, 0, 0, 255);    // logical 8 = blue
    ring.setPixel(12, 255, 255, 0); // logical 12 = yellow

    ring.setOrientation(0, true);   // default
    ring.show();
    renderRing(ring, std::string(OUT_DIR) + "/orient_default_cw_off0.ppm");

    ring.setOrientation(4, true);   // shifted by 4 clockwise
    ring.show();
    renderRing(ring, std::string(OUT_DIR) + "/orient_cw_off4.ppm");

    ring.setOrientation(0, false);  // counterclockwise from same start
    ring.show();
    renderRing(ring, std::string(OUT_DIR) + "/orient_ccw_off0.ppm");

    ring.setOrientation(8, false);  // shifted 8, counterclockwise
    ring.show();
    renderRing(ring, std::string(OUT_DIR) + "/orient_ccw_off8.ppm");

    std::printf("Wrote PPMs to %s/\n", OUT_DIR);
    std::printf("Bytes that would be sent on last show(): %zu\n",
                NeoPixel::mockLastShowBytes());

    ring.end();
    return 0;
}