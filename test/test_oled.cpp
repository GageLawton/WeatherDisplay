// Mac-side tester for the OLED driver.
// Renders multiple time-display variants and dumps each as a PGM file
// (openable in Preview) into ./test_output/.
//
// Usage:
//   ./test_oled                  -> render all built-in variants
//   ./test_oled "HH:MM" 3        -> render a single custom variant

#include "oled.h"
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

static const char* OUT_DIR = "test_output";

static void ensureOutDir() {
    // mkdir is idempotent enough for our purposes; ignore EEXIST.
    if (mkdir(OUT_DIR, 0755) != 0) {
        // Errors other than "already exists" are fine to print but not fatal.
    }
}

static std::string outPath(const std::string& name) {
    return std::string(OUT_DIR) + "/" + name;
}

// Format helper: translate friendly placeholders to strftime ones.
static std::string formatTime(const std::string& fmt, std::time_t t) {
    std::tm tm = *std::localtime(&t);
    std::string sf;
    for (size_t i = 0; i < fmt.size(); ) {
        if (fmt.compare(i, 8, "HH:MM:SS") == 0) {
            sf += "%H:%M:%S"; i += 8;
        } else if (fmt.compare(i, 5, "HH:MM") == 0) {
            sf += "%H:%M"; i += 5;
        } else if (fmt.compare(i, 2, "SS") == 0) {
            sf += "%S"; i += 2;
        } else {
            sf += fmt[i++];
        }
    }
    char buf[64];
    std::strftime(buf, sizeof(buf), sf.c_str(), &tm);
    return buf;
}

static void writePgm(const std::string& path, const OLED& oled, int upscale = 4) {
    const int W = OLED::WIDTH  * upscale;
    const int H = OLED::HEIGHT * upscale;
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) { std::perror(path.c_str()); return; }
    std::fprintf(f, "P5\n%d %d\n255\n", W, H);
    for (int y = 0; y < OLED::HEIGHT; ++y) {
        for (int sy = 0; sy < upscale; ++sy) {
            for (int x = 0; x < OLED::WIDTH; ++x) {
                uint8_t v = oled.getPixel(x, y) ? 255 : 0;
                for (int sx = 0; sx < upscale; ++sx) std::fputc(v, f);
            }
        }
    }
    std::fclose(f);
}

// Render a single centered time string at the given scale (or auto-fit if scale==0).
static void renderSingle(OLED& oled, const std::string& text, int scale) {
    oled.clear();
    if (scale == 0) {
        for (int s = 4; s >= 1; --s) {
            if (OLED::textWidth(text, s) <= OLED::WIDTH) {
                int y = (OLED::HEIGHT - OLED::textHeight(s)) / 2;
                oled.drawTextCenteredFit(y, text, s);
                break;
            }
        }
    } else {
        int y = (OLED::HEIGHT - OLED::textHeight(scale)) / 2;
        oled.drawTextCenteredFit(y, text, scale);
    }
}

// Render a "combo" view: HH:MM big and centered, :SS small below.
static void renderCombo(OLED& oled, std::time_t t) {
    oled.clear();
    std::string hhmm = formatTime("HH:MM", t);
    std::string ss   = formatTime("SS",    t);

    int bigScale = 4;
    while (bigScale > 1 && OLED::textWidth(hhmm, bigScale) > OLED::WIDTH) --bigScale;
    int bigW = OLED::textWidth(hhmm, bigScale);
    int bigH = OLED::textHeight(bigScale);
    int bigX = (OLED::WIDTH - bigW) / 2;

    int smallScale = 2;
    int smallH     = OLED::textHeight(smallScale);
    int gap        = 2;
    int totalH     = bigH + gap + smallH;
    int yTop       = (OLED::HEIGHT - totalH) / 2;
    oled.drawText(bigX, yTop, hhmm, bigScale);

    int smallW = OLED::textWidth(ss, smallScale);
    int smallX = (OLED::WIDTH - smallW) / 2;
    int smallY = yTop + bigH + gap;
    oled.drawText(smallX, smallY, ss, smallScale);
}

int main(int argc, char** argv) {
    ensureOutDir();
    std::time_t t = 1700000000;  // fixed for reproducibility

    OLED oled;
    if (!oled.begin(0x3C)) {
        std::fprintf(stderr, "begin() failed\n");
        return 1;
    }

    // ---- Custom one-off mode: ./test_oled "HH:MM" 3 ----
    if (argc > 1) {
        std::string fmt   = argv[1];
        int         scale = (argc > 2) ? std::atoi(argv[2]) : 0;
        std::string text  = formatTime(fmt, t);
        renderSingle(oled, text, scale);
        oled.show();
        std::string path = outPath("out.pgm");
        writePgm(path, oled);
        std::printf("Wrote %s  (\"%s\", scale=%s)\n",
                    path.c_str(), text.c_str(), scale == 0 ? "auto" : argv[2]);
        oled.end();
        return 0;
    }

    // ---- Default: render all variants ----
    struct Variant {
        const char* name;
        const char* fmt;
        int         scale; // 0 = auto
    };
    Variant variants[] = {
        {"HH-MM-SS_auto.pgm", "HH:MM:SS", 0},
        {"HH-MM-SS_s1.pgm",   "HH:MM:SS", 1},
        {"HH-MM-SS_s2.pgm",   "HH:MM:SS", 2},
        {"HH-MM_auto.pgm",    "HH:MM",    0},
        {"HH-MM_s3.pgm",      "HH:MM",    3},
        {"HH-MM_s4.pgm",      "HH:MM",    4},
    };
    for (const Variant& v : variants) {
        std::string text = formatTime(v.fmt, t);
        renderSingle(oled, text, v.scale);
        oled.show();
        std::string path = outPath(v.name);
        writePgm(path, oled);
        std::printf("Wrote %-30s (\"%s\", scale=%s)\n",
                    path.c_str(), text.c_str(),
                    v.scale == 0 ? "auto" : std::to_string(v.scale).c_str());
    }
    // Combo: HH:MM big + :SS small
    renderCombo(oled, t);
    oled.show();
    std::string comboPath = outPath("combo.pgm");
    writePgm(comboPath, oled);
    std::printf("Wrote %-30s (HH:MM big + :SS small)\n", comboPath.c_str());

    oled.end();
    return 0;
}