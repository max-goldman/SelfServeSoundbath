#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// ============================================================================
// Pin Definitions
// ============================================================================

// WS2812b LED data pins
[[maybe_unused]] constexpr uint8_t PIN_LED_ZONED = 2;
[[maybe_unused]] constexpr uint8_t PIN_LED_GENERAL_A = 3;
[[maybe_unused]] constexpr uint8_t PIN_LED_GENERAL_B = 4;

// Control inputs
[[maybe_unused]] constexpr uint8_t PIN_TOGGLE_DAYNIGHT = 5;

// Reserved for future use in v0
[[maybe_unused]] constexpr uint8_t PIN_TOGGLE_RESERVED_6 = 6;
[[maybe_unused]] constexpr uint8_t PIN_TOGGLE_RESERVED_7 = 7;
[[maybe_unused]] constexpr uint8_t PIN_TOGGLE_RESERVED_8 = 8;

// Heartbeat LED (onboard)
[[maybe_unused]] constexpr uint8_t PIN_HEARTBEAT = 13;

// Serial1 (RX1/TX1) for LD2410C mmWave on pins 0, 1 (defined by Teensy)

// ============================================================================
// LED Run Configuration
// ============================================================================

[[maybe_unused]] constexpr uint8_t RUN_ZONED = 0;
[[maybe_unused]] constexpr uint8_t RUN_GENERAL_A = 1;
[[maybe_unused]] constexpr uint8_t RUN_GENERAL_B = 2;

// LED counts per run
[[maybe_unused]] constexpr uint16_t ZONED_LED_COUNT = 48;
[[maybe_unused]] constexpr uint16_t GENERAL_A_LED_COUNT = 180;
[[maybe_unused]] constexpr uint16_t GENERAL_B_LED_COUNT = 90;

// Faucet panel mix section (driven by two knobs jointly)
[[maybe_unused]] constexpr uint8_t FAUCET_MIX_FIRST = 41;
[[maybe_unused]] constexpr uint8_t FAUCET_MIX_COUNT = 3;

// ============================================================================
// Knob Configuration
// ============================================================================

constexpr uint8_t KNOB_COUNT = 16;

// Render modes for LED visualization
enum KnobRender {
  RENDER_COMPLEMENTARY,  // 3 LEDs, hue lerp between a complementary pair
  RENDER_MONO,           // 3 LEDs, black -> white (the special jet knob)
  RENDER_COLD,           // 1 LED, pure blue, brightness tracks CC (faucet a)
  RENDER_HOT,            // 1 LED, pure red,  brightness tracks CC (faucet b)
  RENDER_GAMMA           // 4 LEDs, white, brightness splits left/right (f,g,h,i)
};

// Configuration for each knob
struct KnobConfig {
  uint8_t  cc;                 // MIDI CC number
  uint8_t  pinA, pinB;         // encoder quadrature pins
  uint8_t  ledRun;             // RUN_ZONED for all 16 in v0
  uint8_t  ledFirstIndex;      // first LED index in the run
  uint8_t  ledCount;           // number of LEDs
  uint8_t  activeDefault;      // CC value on entering ACTIVE
  uint8_t  passiveDefault;     // CC value on entering PASSIVE
  int32_t  countsPerFullRange; // encoder counts for a full 0..127 sweep
  uint8_t  render;             // KnobRender mode for this knob
  uint16_t hueLow, hueHigh;    // degrees; complementary pair, only for RENDER_COMPLEMENTARY
};

// Knob table: 16 knobs with their MIDI, pin, LED, and rendering configuration
// All rows have: ledRun = RUN_ZONED, activeDefault = 63, passiveDefault = 0
constexpr KnobConfig KNOBS[KNOB_COUNT] = {
  // aux A panel (knobs 0-3)
  { 102, 9,  10, RUN_ZONED, 0,  3, 63, 0, 160, RENDER_COMPLEMENTARY, 60,  240 },
  { 103, 11, 12, RUN_ZONED, 3,  3, 63, 0, 160, RENDER_COMPLEMENTARY, 75,  255 },
  { 104, 14, 15, RUN_ZONED, 6,  3, 63, 0, 160, RENDER_COMPLEMENTARY, 90,  270 },
  { 105, 16, 17, RUN_ZONED, 9,  3, 63, 0, 160, RENDER_COMPLEMENTARY, 105, 285 },
  // aux B panel (knobs 4-7)
  { 106, 18, 19, RUN_ZONED, 12, 3, 63, 0, 160, RENDER_COMPLEMENTARY, 120, 300 },
  { 107, 20, 21, RUN_ZONED, 15, 3, 63, 0, 160, RENDER_COMPLEMENTARY, 135, 315 },
  { 108, 22, 23, RUN_ZONED, 18, 3, 63, 0, 160, RENDER_COMPLEMENTARY, 150, 330 },
  { 109, 24, 25, RUN_ZONED, 21, 3, 63, 0, 160, RENDER_COMPLEMENTARY, 165, 345 },
  // jet panel (knobs 8-12)
  { 110, 26, 27, RUN_ZONED, 24, 3, 63, 0, 160, RENDER_COMPLEMENTARY, 180, 0   },
  { 111, 28, 29, RUN_ZONED, 27, 3, 63, 0, 160, RENDER_COMPLEMENTARY, 195, 15  },
  { 112, 30, 31, RUN_ZONED, 30, 3, 63, 0, 160, RENDER_COMPLEMENTARY, 210, 30  },
  { 113, 32, 33, RUN_ZONED, 33, 3, 63, 0, 160, RENDER_COMPLEMENTARY, 225, 45  },
  { 114, 34, 35, RUN_ZONED, 36, 3, 63, 0, 160, RENDER_MONO,          0,   0   },
  // faucet panel (knobs 13-15)
  { 115, 36, 37, RUN_ZONED, 39, 1, 63, 0, 192, RENDER_COLD,          0,   0   },
  { 116, 38, 39, RUN_ZONED, 40, 1, 63, 0, 192, RENDER_HOT,           0,   0   },
  { 117, 40, 41, RUN_ZONED, 44, 4, 63, 0, 160, RENDER_GAMMA,         0,   0   },
};

// ============================================================================
// Tuning Constants (calibration knobs for real hardware)
// ============================================================================

// LED brightness ceiling (0..255). SAFETY LIMIT, not aesthetic.
// 318 LEDs of full white is ~19 A against a 20 A supply.
// Raise only with a clamp meter on the bench.
[[maybe_unused]] constexpr uint8_t LED_BRIGHTNESS_CEILING = 96;

// Daytime multiplier when PIN_TOGGLE_DAYNIGHT reads day
[[maybe_unused]] constexpr float DAY_BRIGHTNESS_SCALE = 0.5f;

// Continuous agreement required in BOTH directions (ms)
[[maybe_unused]] constexpr uint16_t PRESENCE_DEBOUNCE_MS = 4000;

// Ignore targets beyond this range, so someone walking past the tub
// does not start a session (cm)
[[maybe_unused]] constexpr uint8_t LD2410_MAX_DISTANCE_CM = 120;

// Set to 1 to bypass the presence sensor for bench testing
[[maybe_unused]] constexpr uint8_t FORCE_ACTIVE = 0;

// MIDI channel (1-based, matches usbMIDI.sendControlChange's channel arg; valid range 1..16)
[[maybe_unused]] constexpr uint8_t MIDI_CHANNEL = 1;

// LED refresh interval (~60 Hz render) (ms)
[[maybe_unused]] constexpr uint16_t LED_FRAME_INTERVAL_MS = 16;

// General runs breathe to this percentage of the ceiling when passive (%)
[[maybe_unused]] constexpr uint8_t PASSIVE_BREATHE_PEAK_PCT = 10;

// One full slow breath cycle (ms)
[[maybe_unused]] constexpr uint16_t PASSIVE_BREATHE_PERIOD_MS = 6000;

// General runs' color cycle period when active (ms)
[[maybe_unused]] constexpr uint32_t GENERAL_CYCLE_PERIOD_MS = 60000;

// ============================================================================
// Compile-time Validation
// ============================================================================

// Verify no knob's LED range exceeds the zoned run
static_assert(KNOBS[0].ledFirstIndex + KNOBS[0].ledCount <= ZONED_LED_COUNT);
static_assert(KNOBS[1].ledFirstIndex + KNOBS[1].ledCount <= ZONED_LED_COUNT);
static_assert(KNOBS[2].ledFirstIndex + KNOBS[2].ledCount <= ZONED_LED_COUNT);
static_assert(KNOBS[3].ledFirstIndex + KNOBS[3].ledCount <= ZONED_LED_COUNT);
static_assert(KNOBS[4].ledFirstIndex + KNOBS[4].ledCount <= ZONED_LED_COUNT);
static_assert(KNOBS[5].ledFirstIndex + KNOBS[5].ledCount <= ZONED_LED_COUNT);
static_assert(KNOBS[6].ledFirstIndex + KNOBS[6].ledCount <= ZONED_LED_COUNT);
static_assert(KNOBS[7].ledFirstIndex + KNOBS[7].ledCount <= ZONED_LED_COUNT);
static_assert(KNOBS[8].ledFirstIndex + KNOBS[8].ledCount <= ZONED_LED_COUNT);
static_assert(KNOBS[9].ledFirstIndex + KNOBS[9].ledCount <= ZONED_LED_COUNT);
static_assert(KNOBS[10].ledFirstIndex + KNOBS[10].ledCount <= ZONED_LED_COUNT);
static_assert(KNOBS[11].ledFirstIndex + KNOBS[11].ledCount <= ZONED_LED_COUNT);
static_assert(KNOBS[12].ledFirstIndex + KNOBS[12].ledCount <= ZONED_LED_COUNT);
static_assert(KNOBS[13].ledFirstIndex + KNOBS[13].ledCount <= ZONED_LED_COUNT);
static_assert(KNOBS[14].ledFirstIndex + KNOBS[14].ledCount <= ZONED_LED_COUNT);
static_assert(KNOBS[15].ledFirstIndex + KNOBS[15].ledCount <= ZONED_LED_COUNT);

// Verify MIDI CC values are the contiguous range 102..117
static_assert(KNOBS[0].cc == 102);
static_assert(KNOBS[1].cc == 103);
static_assert(KNOBS[2].cc == 104);
static_assert(KNOBS[3].cc == 105);
static_assert(KNOBS[4].cc == 106);
static_assert(KNOBS[5].cc == 107);
static_assert(KNOBS[6].cc == 108);
static_assert(KNOBS[7].cc == 109);
static_assert(KNOBS[8].cc == 110);
static_assert(KNOBS[9].cc == 111);
static_assert(KNOBS[10].cc == 112);
static_assert(KNOBS[11].cc == 113);
static_assert(KNOBS[12].cc == 114);
static_assert(KNOBS[13].cc == 115);
static_assert(KNOBS[14].cc == 116);
static_assert(KNOBS[15].cc == 117);

// Verify encoder pins are distinct and don't collide with LED/control pins.
// constexpr helper walks the pin list pairwise; this runs only at compile
// time, so it is not the runtime validation function the brief forbids.
constexpr bool allPinsDistinct(const uint8_t* pins, int count) {
  for (int i = 0; i < count; ++i) {
    for (int j = i + 1; j < count; ++j) {
      if (pins[i] == pins[j]) return false;
    }
  }
  return true;
}

constexpr uint8_t GUARDED_PINS[] = {
  KNOBS[0].pinA,  KNOBS[0].pinB,
  KNOBS[1].pinA,  KNOBS[1].pinB,
  KNOBS[2].pinA,  KNOBS[2].pinB,
  KNOBS[3].pinA,  KNOBS[3].pinB,
  KNOBS[4].pinA,  KNOBS[4].pinB,
  KNOBS[5].pinA,  KNOBS[5].pinB,
  KNOBS[6].pinA,  KNOBS[6].pinB,
  KNOBS[7].pinA,  KNOBS[7].pinB,
  KNOBS[8].pinA,  KNOBS[8].pinB,
  KNOBS[9].pinA,  KNOBS[9].pinB,
  KNOBS[10].pinA, KNOBS[10].pinB,
  KNOBS[11].pinA, KNOBS[11].pinB,
  KNOBS[12].pinA, KNOBS[12].pinB,
  KNOBS[13].pinA, KNOBS[13].pinB,
  KNOBS[14].pinA, KNOBS[14].pinB,
  KNOBS[15].pinA, KNOBS[15].pinB,
  PIN_LED_ZONED, PIN_LED_GENERAL_A, PIN_LED_GENERAL_B,
  PIN_TOGGLE_DAYNIGHT, PIN_HEARTBEAT,
};

static_assert(allPinsDistinct(GUARDED_PINS, sizeof(GUARDED_PINS) / sizeof(GUARDED_PINS[0])),
              "encoder pins must be distinct and not collide with LED/control pins");

// Verify all defaults are valid MIDI values (0..127)
static_assert(KNOBS[0].activeDefault <= 127);
static_assert(KNOBS[0].passiveDefault <= 127);
static_assert(KNOBS[1].activeDefault <= 127);
static_assert(KNOBS[1].passiveDefault <= 127);
static_assert(KNOBS[2].activeDefault <= 127);
static_assert(KNOBS[2].passiveDefault <= 127);
static_assert(KNOBS[3].activeDefault <= 127);
static_assert(KNOBS[3].passiveDefault <= 127);
static_assert(KNOBS[4].activeDefault <= 127);
static_assert(KNOBS[4].passiveDefault <= 127);
static_assert(KNOBS[5].activeDefault <= 127);
static_assert(KNOBS[5].passiveDefault <= 127);
static_assert(KNOBS[6].activeDefault <= 127);
static_assert(KNOBS[6].passiveDefault <= 127);
static_assert(KNOBS[7].activeDefault <= 127);
static_assert(KNOBS[7].passiveDefault <= 127);
static_assert(KNOBS[8].activeDefault <= 127);
static_assert(KNOBS[8].passiveDefault <= 127);
static_assert(KNOBS[9].activeDefault <= 127);
static_assert(KNOBS[9].passiveDefault <= 127);
static_assert(KNOBS[10].activeDefault <= 127);
static_assert(KNOBS[10].passiveDefault <= 127);
static_assert(KNOBS[11].activeDefault <= 127);
static_assert(KNOBS[11].passiveDefault <= 127);
static_assert(KNOBS[12].activeDefault <= 127);
static_assert(KNOBS[12].passiveDefault <= 127);
static_assert(KNOBS[13].activeDefault <= 127);
static_assert(KNOBS[13].passiveDefault <= 127);
static_assert(KNOBS[14].activeDefault <= 127);
static_assert(KNOBS[14].passiveDefault <= 127);
static_assert(KNOBS[15].activeDefault <= 127);
static_assert(KNOBS[15].passiveDefault <= 127);

#endif // CONFIG_H
