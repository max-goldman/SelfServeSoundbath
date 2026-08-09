// mapping.h — pure math for encoder->MIDI and MIDI->LED color mapping.
//
// Header-only, stateless: every function is inline or constexpr, no
// globals. Compiled both by the Arduino toolchain and by plain host c++
// (see test/test_mapping.cpp), so this file includes nothing but
// <stdint.h> and <math.h>.

#ifndef MAPPING_H
#define MAPPING_H

#include <stdint.h>
#include <math.h>

struct RGB {
  uint8_t r, g, b;
};

struct GammaSplit {
  float fg, hi;
};

// Maps a clamped encoder count to a MIDI CC value. Counts outside
// [0, countsPerFullRange] clamp to the rails rather than wrapping.
// Hits exactly 0 and exactly 127 at the rails.
inline uint8_t countsToCC(int32_t counts, int32_t countsPerFullRange) {
  if (counts <= 0) return 0;
  if (counts >= countsPerFullRange) return 127;
  int32_t v = (counts * 127 + countsPerFullRange / 2) / countsPerFullRange;
  if (v < 0) v = 0;
  if (v > 127) v = 127;
  return (uint8_t)v;
}

// Standard HSV to RGB. hueDeg wraps for values >= 360. sat/val are 0..255.
inline RGB hsv2rgb(uint16_t hueDeg, uint8_t sat, uint8_t val) {
  hueDeg = (uint16_t)(hueDeg % 360);
  float h = hueDeg / 60.0f;
  float s = sat / 255.0f;
  float v = val / 255.0f;
  float c = v * s;
  float x = c * (1.0f - fabsf(fmodf(h, 2.0f) - 1.0f));
  float m = v - c;
  float r = 0, g = 0, b = 0;
  int i = (int)h;
  switch (i) {
    case 0: r = c; g = x; b = 0; break;
    case 1: r = x; g = c; b = 0; break;
    case 2: r = 0; g = c; b = x; break;
    case 3: r = 0; g = x; b = c; break;
    case 4: r = x; g = 0; b = c; break;
    default: r = c; g = 0; b = x; break;
  }
  auto toByte = [](float ch) -> uint8_t {
    float v255 = ch * 255.0f;
    if (v255 < 0.0f) v255 = 0.0f;
    if (v255 > 255.0f) v255 = 255.0f;
    return (uint8_t)roundf(v255);
  };
  RGB out;
  out.r = toByte(r + m);
  out.g = toByte(g + m);
  out.b = toByte(b + m);
  return out;
}

// Zone 1 / zone 2 complementary interpolation. Interpolates ascending
// from hueLow to hueHigh; if hueHigh < hueLow, 360 is added to hueHigh
// first so the sweep goes the "long way" around correctly (e.g. 180 ->
// 360 rather than 180 -> 0). Result is normalized to [0, 360).
inline uint16_t lerpHue(uint16_t hueLow, uint16_t hueHigh, uint8_t cc) {
  float low = hueLow;
  float high = hueHigh;
  if (high < low) high += 360.0f;
  float t = cc / 127.0f;
  float h = low + t * (high - low);
  h = fmodf(h, 360.0f);
  if (h < 0.0f) h += 360.0f;
  return (uint16_t)((uint16_t)roundf(h) % 360);
}

// Straight per-channel linear interpolation, t clamped to [0,1].
inline RGB lerpRGB(RGB a, RGB b, float t) {
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  RGB out;
  out.r = (uint8_t)roundf(a.r + t * (float)(b.r - a.r));
  out.g = (uint8_t)roundf(a.g + t * (float)(b.g - a.g));
  out.b = (uint8_t)roundf(a.b + t * (float)(b.b - a.b));
  return out;
}

// Drives faucet lights f,g (left, fg) and h,i (right, hi). Center leaves
// all four at half; turning right increases hi and fades fg.
inline GammaSplit gammaSplit(uint8_t cc) {
  GammaSplit g;
  g.hi = cc / 127.0f;
  g.fg = 1.0f - g.hi;
  return g;
}

// Scales c by ceiling/255, then by dayScale if daytime. No output channel
// ever exceeds ceiling — this is the power safety limit for the 20A supply.
inline RGB applyBrightness(RGB c, uint8_t ceiling, bool daytime, float dayScale) {
  float scale = ceiling / 255.0f;
  if (daytime) scale *= dayScale;
  auto scaleChannel = [ceiling, scale](uint8_t v) -> uint8_t {
    float out = v * scale;
    if (out > ceiling) out = ceiling;
    if (out < 0.0f) out = 0.0f;
    return (uint8_t)roundf(out);
  };
  RGB out;
  out.r = scaleChannel(c.r);
  out.g = scaleChannel(c.g);
  out.b = scaleChannel(c.b);
  return out;
}

#endif  // MAPPING_H
