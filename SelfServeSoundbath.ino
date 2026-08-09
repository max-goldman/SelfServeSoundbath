// SelfServeSoundbath.ino — the integration point.
//
// Owns exactly three things: encoder reading, MIDI sending, and the
// idle/active state machine. Rendering is leds.cpp's job and sensor talk is
// presence.cpp's job; this file only calls into them.
//
// Order of operations, every loop iteration (README "Chain of command"):
//   encoder delta -> knob value -> MIDI CC out -> that knob's LED zone
// LED state is always derived from knob state, never an independent source
// of truth.

#include <Encoder.h>

#include "config.h"
#include "mapping.h"
#include "leds.h"
#include "presence.h"

// ============================================================================
// Encoders
// ============================================================================

Encoder encoders[KNOB_COUNT] = {
  Encoder(KNOBS[0].pinA,  KNOBS[0].pinB),
  Encoder(KNOBS[1].pinA,  KNOBS[1].pinB),
  Encoder(KNOBS[2].pinA,  KNOBS[2].pinB),
  Encoder(KNOBS[3].pinA,  KNOBS[3].pinB),
  Encoder(KNOBS[4].pinA,  KNOBS[4].pinB),
  Encoder(KNOBS[5].pinA,  KNOBS[5].pinB),
  Encoder(KNOBS[6].pinA,  KNOBS[6].pinB),
  Encoder(KNOBS[7].pinA,  KNOBS[7].pinB),
  Encoder(KNOBS[8].pinA,  KNOBS[8].pinB),
  Encoder(KNOBS[9].pinA,  KNOBS[9].pinB),
  Encoder(KNOBS[10].pinA, KNOBS[10].pinB),
  Encoder(KNOBS[11].pinA, KNOBS[11].pinB),
  Encoder(KNOBS[12].pinA, KNOBS[12].pinB),
  Encoder(KNOBS[13].pinA, KNOBS[13].pinB),
  Encoder(KNOBS[14].pinA, KNOBS[14].pinB),
  Encoder(KNOBS[15].pinA, KNOBS[15].pinB),
};

// ============================================================================
// State
// ============================================================================

enum SessionState { PASSIVE, ACTIVE };

static SessionState state = PASSIVE;

// Debounce bookkeeping for the presence state machine.
static bool pending = false;
static uint32_t pendingSince = 0;

// Current CC value per knob. Doubles as the last-sent-MIDI tracker (sendCC's
// dedupe) and the array fed to leds_update — one number, one source of
// truth, same as the README's "LED state is derived from knob state" rule.
// 255 is not a valid MIDI value, so it forces the very first sendCC() call
// per knob (from the boot-time enterPassive()) to actually go out.
static uint8_t currentCC[KNOB_COUNT];

// ============================================================================
// The one MIDI send function — every CC in this firmware leaves through it.
// ============================================================================

static void sendCC(uint8_t knob, uint8_t value) {
  if (currentCC[knob] == value) return;
  currentCC[knob] = value;
  usbMIDI.sendControlChange(KNOBS[knob].cc, value, MIDI_CHANNEL);
}

// Commented-out starting point for patch switching (v0plan todo): trigger a
// MIDI program change when a detentless temperature knob (cold/hot) crosses
// a threshold. Found by render mode, not by literal knob index, since those
// two knobs' positions in the table are not part of the contract.
//
// static int8_t lastColdProgram = -1;
// static int8_t lastHotProgram = -1;
// static void maybeSendProgramChange(uint8_t knob, uint8_t cc) {
//   bool isCold = KNOBS[knob].render == RENDER_COLD;
//   bool isHot = KNOBS[knob].render == RENDER_HOT;
//   if (!isCold && !isHot) return;
//   uint8_t program = cc / 32;  // e.g. 4 programs per full sweep
//   int8_t &last = isCold ? lastColdProgram : lastHotProgram;
//   if (program == last) return;
//   last = program;
//   usbMIDI.sendProgramChange(program, MIDI_CHANNEL);  // once per crossing
// }

// Inverse of mapping.h's countsToCC: the nearest counts that maps back to
// cc. countsToCC rounds counts*127/range to nearest; this rounds the inverse
// the same way so the two round-trip.
static int32_t ccToCounts(uint8_t cc, int32_t countsPerFullRange) {
  int32_t counts = ((int32_t)cc * countsPerFullRange + 63) / 127;
  if (counts < 0) counts = 0;
  if (counts > countsPerFullRange) counts = countsPerFullRange;
  return counts;
}

// ============================================================================
// Transitions
// ============================================================================

// Re-seeds every encoder from activeDefault, discarding whatever
// accumulated while idle, then sends all 16 CCs and lets the LEDs follow.
static void enterActive() {
  state = ACTIVE;
  for (uint8_t i = 0; i < KNOB_COUNT; ++i) {
    encoders[i].write(ccToCounts(KNOBS[i].activeDefault, KNOBS[i].countsPerFullRange));
    sendCC(i, KNOBS[i].activeDefault);
  }
}

// Sends all 16 CCs at passiveDefault and lets the LEDs follow. Encoder
// counters are left alone — they get re-seeded on the next enterActive(),
// and nothing reads them while passive.
static void enterPassive() {
  state = PASSIVE;
  for (uint8_t i = 0; i < KNOB_COUNT; ++i) {
    sendCC(i, KNOBS[i].passiveDefault);
  }
}

// PASSIVE <-> ACTIVE, gated by PRESENCE_DEBOUNCE_MS of continuous agreement
// in both directions. A single disagreeing sample resets the timer to zero
// — this is not a rolling average or majority vote.
static void updateStateMachine(uint32_t now) {
  bool raw = FORCE_ACTIVE ? true : presence_detected();
  bool wantActive = raw;
  if (wantActive != (state == ACTIVE)) {
    if (!pending) {
      pending = true;
      pendingSince = now;
    } else if ((uint32_t)(now - pendingSince) >= PRESENCE_DEBOUNCE_MS) {
      if (wantActive) enterActive(); else enterPassive();
      pending = false;
    }
  } else {
    pending = false;  // agreement broken, restart the count
  }
}

// ============================================================================
// Encoder scan — only meaningful while ACTIVE; passive means passive.
// ============================================================================

static void scanEncoders() {
  for (uint8_t i = 0; i < KNOB_COUNT; ++i) {
    int32_t counts = encoders[i].read();
    if (counts < 0) counts = 0;
    if (counts > KNOBS[i].countsPerFullRange) counts = KNOBS[i].countsPerFullRange;
    encoders[i].write(counts);  // write back the clamp, or the knob "sticks"
    sendCC(i, countsToCC(counts, KNOBS[i].countsPerFullRange));
  }
}

// ============================================================================
// Heartbeat — pin 13, ~1 Hz, so "is the loop even running" is a glance away.
// ============================================================================

static void updateHeartbeat(uint32_t now) {
  static uint32_t last = 0;
  static bool on = false;
  if ((uint32_t)(now - last) >= 500) {
    last = now;
    on = !on;
    digitalWrite(PIN_HEARTBEAT, on ? HIGH : LOW);
  }
}

// ============================================================================
// setup / loop
// ============================================================================

void setup() {
  for (uint8_t i = 0; i < KNOB_COUNT; ++i) currentCC[i] = 255;  // force initial sends

  pinMode(PIN_TOGGLE_DAYNIGHT, INPUT_PULLUP);
  pinMode(PIN_HEARTBEAT, OUTPUT);

  leds_begin();
  presence_begin();

  enterPassive();  // boot state: passive, primes MIDI + LEDs at their defaults
}

void loop() {
  uint32_t now = millis();

  usbMIDI.read();  // drain inbound; required even though we ignore it

  updateHeartbeat(now);
  updateStateMachine(now);

  if (state == ACTIVE) {
    scanEncoders();
  }

  bool daytime = digitalRead(PIN_TOGGLE_DAYNIGHT) == LOW;  // closed switch = daytime
  leds_update(state == ACTIVE, currentCC, daytime, now);
}
