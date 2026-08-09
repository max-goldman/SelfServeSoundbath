// Host-compiled check for mapping.h. No test framework: plain <cassert>,
// a main() that returns 0, and a final "all N checks passed" line so a
// passing run is visibly a passing run.
//
// Build & run (from repo root):
//   c++ -std=c++17 -Wall -Wextra test/test_mapping.cpp -o /tmp/t && /tmp/t

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "../mapping.h"
#include "../config.h"

namespace {

int gChecks = 0;

void check(bool cond, const char* msg) {
  ++gChecks;
  if (!cond) {
    std::fprintf(stderr, "FAILED: %s\n", msg);
  }
  assert(cond);
}

bool approxEqual(float a, float b, float eps = 0.01f) {
  return std::fabs(a - b) <= eps;
}

bool redDominant(RGB c)   { return c.r > c.g && c.r > c.b; }
bool greenDominant(RGB c) { return c.g > c.r && c.g > c.b; }
bool blueDominant(RGB c)  { return c.b > c.r && c.b > c.g; }

}  // namespace

int main() {
  // ---- countsToCC ----
  check(countsToCC(0, 160) == 0, "countsToCC(0,160) hits the low rail exactly");
  check(countsToCC(160, 160) == 127, "countsToCC(160,160) hits the high rail exactly");
  check(countsToCC(0, 192) == 0, "countsToCC(0,192) hits the low rail exactly");
  check(countsToCC(192, 192) == 127, "countsToCC(192,192) hits the high rail exactly");
  check(countsToCC(-50, 160) == 0, "counts below range clamp to 0, not wrap");
  check(countsToCC(1000, 160) == 127, "counts above range clamp to 127, not wrap");
  check(countsToCC(-1000000, 192) == 0, "far-negative counts still clamp to 0");
  check(countsToCC(1000000, 192) == 127, "far-over-range counts still clamp to 127");

  {
    bool monotonic = true;
    int32_t range = 160;
    uint8_t prev = countsToCC(-10, range);
    for (int32_t counts = -10; counts <= range + 10; ++counts) {
      uint8_t cc = countsToCC(counts, range);
      if (cc < prev) monotonic = false;
      prev = cc;
    }
    check(monotonic, "countsToCC is monotonic non-decreasing across a full sweep");
  }

  // ---- lerpHue ----
  check(lerpHue(60, 240, 0) == 60, "lerpHue ascending: cc=0 is exactly hueLow");
  check(lerpHue(60, 240, 127) == 240, "lerpHue ascending: cc=127 is exactly hueHigh");

  {
    uint16_t mid = lerpHue(60, 240, 63);
    check(approxEqual((float)mid, 150.0f, 2.0f), "lerpHue ascending midpoint is ~150 deg");
    RGB midColor = hsv2rgb(mid, 255, 255);
    check(greenDominant(midColor), "hsv2rgb of the ascending midpoint hue is green-dominant");
  }

  check(lerpHue(180, 0, 0) == 180, "lerpHue wrap case: cc=0 is exactly hueLow");
  check(lerpHue(180, 0, 127) == 0, "lerpHue wrap case: cc=127 is exactly hueHigh");

  {
    uint16_t mid = lerpHue(180, 0, 63);
    check(approxEqual((float)mid, 270.0f, 2.0f),
          "lerpHue wrap midpoint is ~270 deg, not 90 (wrong direction around the wheel)");
  }

  {
    bool endpointsExact = true;
    bool allInRange = true;
    for (uint8_t i = 0; i < KNOB_COUNT; ++i) {
      if (KNOBS[i].render != RENDER_COMPLEMENTARY) continue;
      uint16_t low = KNOBS[i].hueLow;
      uint16_t high = KNOBS[i].hueHigh;
      if (lerpHue(low, high, 0) != low) endpointsExact = false;
      if (lerpHue(low, high, 127) != high) endpointsExact = false;
      for (int cc = 0; cc <= 127; ++cc) {
        uint16_t h = lerpHue(low, high, (uint8_t)cc);
        if (h >= 360) allInRange = false;
      }
    }
    check(endpointsExact, "lerpHue hits exact endpoints for all 12 complementary knob pairs");
    check(allInRange, "lerpHue stays in [0,360) for all 12 complementary knob pairs across the full sweep");
  }

  // ---- gammaSplit ----
  {
    GammaSplit g = gammaSplit(63);
    check(approxEqual(g.fg, 0.504f, 0.01f), "gammaSplit(63).fg is near 0.504, not forced to exactly 0.5");
    check(approxEqual(g.hi, 0.496f, 0.01f), "gammaSplit(63).hi is near 0.496, not forced to exactly 0.5");
  }
  {
    GammaSplit g = gammaSplit(127);
    check(approxEqual(g.fg, 0.0f, 1e-4f), "gammaSplit(127).fg == 0");
    check(approxEqual(g.hi, 1.0f, 1e-4f), "gammaSplit(127).hi == 1");
  }
  {
    GammaSplit g = gammaSplit(0);
    check(approxEqual(g.fg, 1.0f, 1e-4f), "gammaSplit(0).fg == 1 (mirror of cc=127)");
    check(approxEqual(g.hi, 0.0f, 1e-4f), "gammaSplit(0).hi == 0 (mirror of cc=127)");
  }
  {
    bool sumsToOne = true;
    for (int cc = 0; cc <= 127; ++cc) {
      GammaSplit g = gammaSplit((uint8_t)cc);
      if (!approxEqual(g.fg + g.hi, 1.0f, 1e-4f)) sumsToOne = false;
    }
    check(sumsToOne, "gammaSplit fg+hi == 1 for every cc in 0..127");
  }

  // ---- applyBrightness ----
  {
    bool neverExceeds = true;
    uint8_t ceilings[] = {0, 1, 50, LED_BRIGHTNESS_CEILING, 200, 255};
    for (uint8_t ceiling : ceilings) {
      for (int v = 0; v <= 255; ++v) {
        RGB c{(uint8_t)v, (uint8_t)v, (uint8_t)v};
        RGB nightOut = applyBrightness(c, ceiling, false, DAY_BRIGHTNESS_SCALE);
        RGB dayOut = applyBrightness(c, ceiling, true, DAY_BRIGHTNESS_SCALE);
        if (nightOut.r > ceiling || nightOut.g > ceiling || nightOut.b > ceiling) neverExceeds = false;
        if (dayOut.r > ceiling || dayOut.g > ceiling || dayOut.b > ceiling) neverExceeds = false;
      }
    }
    check(neverExceeds, "applyBrightness never exceeds its ceiling, swept across channel values and several ceilings");
  }
  {
    RGB c{255, 255, 255};
    RGB night = applyBrightness(c, LED_BRIGHTNESS_CEILING, false, DAY_BRIGHTNESS_SCALE);
    RGB day = applyBrightness(c, LED_BRIGHTNESS_CEILING, true, DAY_BRIGHTNESS_SCALE);
    check(approxEqual((float)day.r, (float)night.r * DAY_BRIGHTNESS_SCALE, 1.0f),
          "applyBrightness daytime halves red relative to night");
    check(approxEqual((float)day.g, (float)night.g * DAY_BRIGHTNESS_SCALE, 1.0f),
          "applyBrightness daytime halves green relative to night");
    check(approxEqual((float)day.b, (float)night.b * DAY_BRIGHTNESS_SCALE, 1.0f),
          "applyBrightness daytime halves blue relative to night");
  }
  {
    RGB black{0, 0, 0};
    RGB nightOut = applyBrightness(black, LED_BRIGHTNESS_CEILING, false, DAY_BRIGHTNESS_SCALE);
    check(nightOut.r == 0 && nightOut.g == 0 && nightOut.b == 0, "applyBrightness: pure black stays black at night");
    RGB dayOut = applyBrightness(black, LED_BRIGHTNESS_CEILING, true, DAY_BRIGHTNESS_SCALE);
    check(dayOut.r == 0 && dayOut.g == 0 && dayOut.b == 0, "applyBrightness: pure black stays black in daytime");
  }

  // ---- hsv2rgb ----
  check(redDominant(hsv2rgb(0, 255, 255)), "hsv2rgb(0 deg) is red-dominant");
  check(greenDominant(hsv2rgb(120, 255, 255)), "hsv2rgb(120 deg) is green-dominant");
  check(blueDominant(hsv2rgb(240, 255, 255)), "hsv2rgb(240 deg) is blue-dominant");
  {
    RGB a = hsv2rgb(0, 255, 255);
    RGB b = hsv2rgb(360, 255, 255);
    check(a.r == b.r && a.g == b.g && a.b == b.b, "hsv2rgb(360 deg) wraps to the same result as 0 deg");
  }
  {
    RGB a = hsv2rgb(60, 255, 255);
    RGB b = hsv2rgb(420, 255, 255);
    check(a.r == b.r && a.g == b.g && a.b == b.b, "hsv2rgb(420 deg) wraps to the same result as 60 deg");
  }
  {
    RGB c = hsv2rgb(123, 200, 0);
    check(c.r == 0 && c.g == 0 && c.b == 0, "hsv2rgb val=0 is black regardless of hue");
  }

  // ---- lerpRGB ----
  {
    RGB a{10, 20, 30};
    RGB b{200, 150, 100};
    RGB atStart = lerpRGB(a, b, 0.0f);
    check(atStart.r == a.r && atStart.g == a.g && atStart.b == a.b, "lerpRGB t=0 returns a exactly");
    RGB atEnd = lerpRGB(a, b, 1.0f);
    check(atEnd.r == b.r && atEnd.g == b.g && atEnd.b == b.b, "lerpRGB t=1 returns b exactly");
    RGB atMid = lerpRGB(a, b, 0.5f);
    check(atMid.r == (uint8_t)((a.r + b.r) / 2) &&
              atMid.g == (uint8_t)((a.g + b.g) / 2) &&
              atMid.b == (uint8_t)((a.b + b.b) / 2),
          "lerpRGB t=0.5 is the per-channel average");
    RGB below = lerpRGB(a, b, -5.0f);
    check(below.r == a.r && below.g == a.g && below.b == a.b, "lerpRGB clamps t below 0 to a");
    RGB above = lerpRGB(a, b, 5.0f);
    check(above.r == b.r && above.g == b.g && above.b == b.b, "lerpRGB clamps t above 1 to b");
  }

  std::printf("all %d checks passed\n", gChecks);
  return 0;
}
