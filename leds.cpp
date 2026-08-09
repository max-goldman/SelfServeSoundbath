// leds.cpp — OctoWS2811 output and the zone renderers.
//
// One OctoWS2811 instance drives all three physical runs (PIN_LED_ZONED,
// PIN_LED_GENERAL_A, PIN_LED_GENERAL_B) as three "pins" of a single
// multi-pin strip. All three are sized to the longest run (180); the
// shorter runs use a prefix of their strip and never touch the rest.
// Buffer sizing follows the OctoWS2811 Teensy4_PinList example (arbitrary
// pin list, DMAMEM display buffer + regular drawing buffer, both sized
// ledsPerStrip * numPins * 3 bytes, as ints so the compiler 32-bit aligns
// them).
//
// DMA-backed (not bit-banged): show() starts a DMA transfer and returns;
// busy() polls without blocking. leds_update() never busy-waits for a
// transfer to finish, per the hard requirement that LED refresh must not
// block encoder input.

#include "leds.h"

#include <OctoWS2811.h>

#include "mapping.h"

namespace {

// ----------------------------------------------------------------------
// Tuning values not pinned by the brief or config.h (named here, not
// buried as magic numbers in a renderer):
// ----------------------------------------------------------------------

// WS2812b wiring color order + bit timing. Standard for most WS2812b
// reels; if colors come out channel-swapped on the bench, this is the
// constant to change.
constexpr uint8_t LED_COLOR_CONFIG = WS2811_GRB | WS2811_800kHz;

// ----------------------------------------------------------------------
// OctoWS2811 buffers
// ----------------------------------------------------------------------

constexpr uint16_t LED_STRIP_LEN = GENERAL_A_LED_COUNT;  // 180, longest run
constexpr uint8_t LED_NUM_PINS = 3;
constexpr uint8_t LED_BYTES_PER_PIXEL = 3;  // RGB, not RGBW

const uint8_t ledPinList[LED_NUM_PINS] = {PIN_LED_ZONED, PIN_LED_GENERAL_A,
                                           PIN_LED_GENERAL_B};

DMAMEM int displayMemory[LED_STRIP_LEN * LED_NUM_PINS * LED_BYTES_PER_PIXEL / 4];
int drawingMemory[LED_STRIP_LEN * LED_NUM_PINS * LED_BYTES_PER_PIXEL / 4];

OctoWS2811 leds(LED_STRIP_LEN, displayMemory, drawingMemory, LED_COLOR_CONFIG,
                 LED_NUM_PINS, ledPinList);

uint32_t lastFrameMs = 0;
bool currentDaytime = false;

// ----------------------------------------------------------------------
// The single choke point. Every pixel write in this file goes through
// this function so the brightness ceiling (power safety limit) can never
// be forgotten by a renderer.
// ----------------------------------------------------------------------

void led_set(uint8_t run, uint16_t index, RGB c) {
  RGB scaled = applyBrightness(c, LED_BRIGHTNESS_CEILING, currentDaytime,
                                DAY_BRIGHTNESS_SCALE);
  leds.setPixel((uint32_t)run * LED_STRIP_LEN + index, scaled.r, scaled.g,
                scaled.b);
}

// cc (0..127) -> logical 0..255, per the brief's "v = cc * 255 / 127".
uint8_t scale255(uint8_t cc) {
  return (uint8_t)(((uint16_t)cc * 255) / 127);
}

// ----------------------------------------------------------------------
// Zoned run (48 LEDs)
// ----------------------------------------------------------------------

void renderZonedActive(const uint8_t cc[KNOB_COUNT]) {
  uint8_t coldVal = 0;  // captured from the RENDER_COLD row for the mix
  uint8_t hotVal = 0;   // captured from the RENDER_HOT row for the mix

  for (uint8_t i = 0; i < KNOB_COUNT; ++i) {
    const KnobConfig& k = KNOBS[i];
    const uint8_t v = cc[i];

    switch (k.render) {
      case RENDER_COMPLEMENTARY: {
        uint16_t hue = lerpHue(k.hueLow, k.hueHigh, v);
        RGB c = hsv2rgb(hue, 255, 255);
        for (uint8_t j = 0; j < k.ledCount; ++j) {
          led_set(k.ledRun, k.ledFirstIndex + j, c);
        }
        break;
      }
      case RENDER_MONO: {
        uint8_t val = scale255(v);
        RGB c = {val, val, val};
        for (uint8_t j = 0; j < k.ledCount; ++j) {
          led_set(k.ledRun, k.ledFirstIndex + j, c);
        }
        break;
      }
      case RENDER_COLD: {
        coldVal = scale255(v);
        RGB c = {0, 0, coldVal};
        led_set(k.ledRun, k.ledFirstIndex, c);
        break;
      }
      case RENDER_HOT: {
        hotVal = scale255(v);
        RGB c = {hotVal, 0, 0};
        led_set(k.ledRun, k.ledFirstIndex, c);
        break;
      }
      case RENDER_GAMMA: {
        GammaSplit g = gammaSplit(v);
        uint8_t fgVal = (uint8_t)roundf(g.fg * 255.0f);
        uint8_t hiVal = (uint8_t)roundf(g.hi * 255.0f);
        RGB fgColor = {fgVal, fgVal, fgVal};
        RGB hiColor = {hiVal, hiVal, hiVal};
        led_set(k.ledRun, k.ledFirstIndex + 0, fgColor);
        led_set(k.ledRun, k.ledFirstIndex + 1, fgColor);
        led_set(k.ledRun, k.ledFirstIndex + 2, hiColor);
        led_set(k.ledRun, k.ledFirstIndex + 3, hiColor);
        break;
      }
    }
  }

  // Faucet mix (lights c, d, e): the blend of cold and hot, belongs to no
  // single knob, rendered after the loop. 0.25/0.50/0.75 spacing (not
  // 0/0.5/1) keeps c and e visibly distinct from a and b.
  RGB cold = {0, 0, coldVal};
  RGB hot = {hotVal, 0, 0};
  constexpr float mixT[FAUCET_MIX_COUNT] = {0.25f, 0.50f, 0.75f};
  for (uint8_t j = 0; j < FAUCET_MIX_COUNT; ++j) {
    led_set(RUN_ZONED, FAUCET_MIX_FIRST + j, lerpRGB(cold, hot, mixT[j]));
  }
}

void renderZonedPassive() {
  RGB off = {0, 0, 0};
  for (uint16_t i = 0; i < ZONED_LED_COUNT; ++i) {
    led_set(RUN_ZONED, i, off);
  }
}

// ----------------------------------------------------------------------
// General runs (180 and 90)
// ----------------------------------------------------------------------

void renderGeneralActive(uint32_t nowMs) {
  uint32_t rem = nowMs % GENERAL_CYCLE_PERIOD_MS;
  float phase = (float)rem / (float)GENERAL_CYCLE_PERIOD_MS;  // [0, 1)
  uint16_t hue = (uint16_t)(phase * 360.0f);
  RGB c = hsv2rgb(hue, 255, 255);

  for (uint16_t i = 0; i < GENERAL_A_LED_COUNT; ++i) {
    led_set(RUN_GENERAL_A, i, c);
  }
  for (uint16_t i = 0; i < GENERAL_B_LED_COUNT; ++i) {
    led_set(RUN_GENERAL_B, i, c);
  }
}

void renderGeneralPassive(uint32_t nowMs) {
  constexpr float kTwoPi = 6.28318530717958647692f;
  uint32_t rem = nowMs % PASSIVE_BREATHE_PERIOD_MS;
  float phase = (float)rem / (float)PASSIVE_BREATHE_PERIOD_MS;  // [0, 1)
  // Raised sine: 0 at phase 0, peak at phase 0.5, back to 0 at phase 1.
  // No corner at the bottom.
  float s = 0.5f + 0.5f * sinf(phase * kTwoPi - kTwoPi / 4.0f);
  float peak = 255.0f * PASSIVE_BREATHE_PEAK_PCT / 100.0f;
  uint8_t val = (uint8_t)roundf(peak * s);
  RGB c = {val, val, val};

  for (uint16_t i = 0; i < GENERAL_A_LED_COUNT; ++i) {
    led_set(RUN_GENERAL_A, i, c);
  }
  for (uint16_t i = 0; i < GENERAL_B_LED_COUNT; ++i) {
    led_set(RUN_GENERAL_B, i, c);
  }
}

}  // namespace

void leds_begin() {
  leds.begin();
}

void leds_update(bool active, const uint8_t cc[KNOB_COUNT], bool daytime,
                  uint32_t nowMs) {
  if ((uint32_t)(nowMs - lastFrameMs) < LED_FRAME_INTERVAL_MS) return;
  if (leds.busy()) return;

  lastFrameMs = nowMs;
  currentDaytime = daytime;

  if (active) {
    renderZonedActive(cc);
    renderGeneralActive(nowMs);
  } else {
    renderZonedPassive();
    renderGeneralPassive(nowMs);
  }

  leds.show();
}
