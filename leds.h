// leds.h — DMA LED output and the zone renderers.
//
// Turns knob state into pixels. Reads no pins, owns no MIDI, knows nothing
// about presence detection or the state machine. The sketch (Task 5) is the
// only caller.

#ifndef LEDS_H
#define LEDS_H

#include <stdint.h>
#include "config.h"

void leds_begin();

// Rate-limits itself to LED_FRAME_INTERVAL_MS and skips the frame if the
// previous DMA transfer is still in flight. Safe to call every loop
// iteration. cc[] is the current MIDI CC value (0-127) of each of the 16
// knobs, indexed the same as KNOBS[]. daytime comes from the day/night
// toggle, which the sketch reads.
void leds_update(bool active, const uint8_t cc[KNOB_COUNT], bool daytime,
                  uint32_t nowMs);

#endif  // LEDS_H
