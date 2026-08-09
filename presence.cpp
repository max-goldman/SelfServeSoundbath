// presence.cpp — one interface, two sensors.
//
// The hardware choice (LD2410C mmWave vs. VL53L1X ToF) is undecided, so
// both are written against the same two functions. Flip the #define below
// to swap which one compiles in; everything else in the firmware is
// unaffected.

#include "presence.h"

#include <Arduino.h>

#include "config.h"

// Set to 0 to compile in the VL53L1X path below instead.
#define PRESENCE_SENSOR_LD2410 1

#if PRESENCE_SENSOR_LD2410
// ============================================================================
// LD2410C mmWave (live path)
// ============================================================================
//
// Wired on Serial1 (RX1/TX1, pins 0/1). Factory default baud is 256000,
// matching the ld2410 library's own examples (examples/basicSensor).
//
// The LD2410 reports moving and stationary targets separately. Someone
// soaking in the tub barely moves, so a stationary-only target still counts
// as presence — a moving-target-only check would drop them after they
// settle. Both target types are gated by LD2410_MAX_DISTANCE_CM so a person
// walking past the tub doesn't register.

#include <ld2410.h>

namespace {
constexpr uint32_t LD2410_BAUD = 256000;
ld2410 radar;
uint32_t lastGoodMs = 0;
}  // namespace

bool presence_begin() {
  Serial1.begin(LD2410_BAUD);
  bool ok = radar.begin(Serial1);  // false unless the sensor actually answers
  lastGoodMs = millis();  // don't start already-stale before the first read
  return ok;
}

bool presence_detected() {
  // ld2410::read() (src/ld2410.cpp) pumps the UART and returns
  // `new_data || frame_processed` — true if any bytes arrived at all, not
  // strictly "a frame parsed." That's still the right staleness signal for
  // the failure this guards against: a popped cable stops bytes arriving
  // entirely, so read() goes false and stays false. It only under-detects a
  // sensor that's connected but emitting nothing but noise, which is not the
  // field failure mode this fix targets.
  if (radar.read()) {
    lastGoodMs = millis();
  }
  if ((uint32_t)(millis() - lastGoodMs) > LD2410_STALE_MS) {
    return false;  // dead/disconnected sensor: don't latch on stale fields
  }

  bool stationary = radar.stationaryTargetDetected() &&
                     radar.stationaryTargetDistance() <= LD2410_MAX_DISTANCE_CM;
  bool moving = radar.movingTargetDetected() &&
                radar.movingTargetDistance() <= LD2410_MAX_DISTANCE_CM;
  return stationary || moving;
}

#else
// ============================================================================
// VL53L1X ToF (compiled out — not installed, cannot be built in v0)
// ============================================================================
//
// Pololu VL53L1X library, I2C on pins 18/19 (Teensy 4.1 default Wire).
// Reuses LD2410_MAX_DISTANCE_CM as the gate distance rather than
// introducing a second, sensor-specific threshold — the name is a wart
// worth living with in v0.

#include <VL53L1X.h>
#include <Wire.h>

namespace {
VL53L1X tof;
}  // namespace

bool presence_begin() {
  Wire.begin();
  if (!tof.init()) return false;
  tof.setDistanceMode(VL53L1X::Long);
  tof.setMeasurementTimingBudget(50000);
  tof.startContinuous(50);
  return true;
}

bool presence_detected() {
  if (!tof.dataReady()) return false;  // no new reading yet; don't block

  uint16_t mm = tof.read(false);  // non-blocking: dataReady() already true
  if (tof.timeoutOccurred()) return false;  // timed-out read is not presence

  uint16_t cm = mm / 10;
  return cm <= LD2410_MAX_DISTANCE_CM;
}

#endif
