// Mac-side tester for the RingDisplay controller.
// Renders the ring's state at several simulated times of day, on a single
// real date, so we can see the daily progression of sun + moon behavior.

#include "ring.h"
#include "neopixel.h"
#include "celestial.h"

#include <cstdio>
#include <cstdint>
#include <cmath>
#include <ctime>
#include <string>
#include <sys/stat.h>
#include <vector>

static const char* OUT_DIR = "test_output";
static void ensureOutDir() { mkdir(OUT_DIR, 0755); }

static void writePpm(const std::string& path, int W, int H,
                     const std::vector<uint8_t>& rgb) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) { std::perror(path.c_str()); return; }
    std::fprintf(f, "P6\n%d %d\n255\n", W, H);
    std::fwrite(rgb.data(), 1, rgb.size(), f);
    std::fclose(f);
}

static void renderRing(const NeoPixel& ring, const std::string& outPath) {
    constexpr int W = 256, H = 256, CX = 128, CY = 128, RING_R = 100, LED_R = 16;
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

// Build a Unix timestamp for a specific local date+time.
static std::time_t makeLocalTime(int year, int month, int day, int hour, int minute) {
    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon  = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min  = minute;
    tm.tm_sec  = 0;
    tm.tm_isdst = -1;
    return std::mktime(&tm);
}

int main() {
    ensureOutDir();

    NeoPixel ring;
    if (!ring.begin(16)) {
        std::fprintf(stderr, "begin() failed\n");
        return 1;
    }
    ring.setBrightness(1.0);
    ring.setOrientation(0, true);  // default; we'll re-test on the Pi

    celestial::Observer westmont{41.7958, -87.9756};
    RingDisplay rd;
    rd.configure(&ring, westmont);

    struct Snapshot {
        const char* name;
        int hour, minute;
    };

    // Same date as our celestial test (May 9, 2026). Sunrise ~5:38, sunset ~19:59.
    Snapshot snaps[] = {
        {"00_midnight",   0,  0},
        {"01_03am",       3,  0},
        {"02_dawn_0540",  5, 40},
        {"03_morning",    8, 30},
        {"04_noon",      12, 50},  // local solar noon for Westmont
        {"05_afternoon", 15, 30},
        {"06_dusk_2000", 20,  0},
        {"07_2200",      22,  0},
    };

    for (const auto& s : snaps) {
        std::time_t t = makeLocalTime(2026, 5, 9, s.hour, s.minute);
        rd.update(t);
        std::string path = std::string(OUT_DIR) + "/ring_day_" + s.name + ".ppm";
        renderRing(ring, path);

        // Also print what's happening at that moment.
        bool day = celestial::isDaytime(westmont, t);
        std::printf("%-18s %02d:%02d  %s",
                    s.name, s.hour, s.minute, day ? "DAY  " : "NIGHT");
        if (day) {
            double f = celestial::sunFractionOfDay(westmont, t);
            std::printf(" sun %3.0f%% through day\n", f * 100.0);
        } else {
            std::printf(" moon %4.1f%% illum, %s\n",
                        celestial::moonIllumination(t) * 100.0,
                        celestial::isMoonWaxing(t) ? "waxing" : "waning");
        }
    }

    ring.end();
    std::printf("\nWrote PPMs to %s/\n", OUT_DIR);
    return 0;
}