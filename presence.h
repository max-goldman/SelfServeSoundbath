// presence.h — "is someone there right now," nothing else.
//
// Answers one question: does the sensor see someone within range? No
// timing, no debounce, no hysteresis, no state machine, no LED/MIDI
// knowledge. The 4-second continuous-detection debounce and the
// FORCE_ACTIVE bench-test bypass both live in the sketch (Task 5), so they
// apply identically no matter which sensor is compiled in below.

#ifndef PRESENCE_H
#define PRESENCE_H

// Starts the presence sensor. Returns whether it actually responded, not
// just whether the serial port was opened.
bool presence_begin();

// The sensor's instantaneous, undebounced opinion. Non-blocking — safe to
// call every loop iteration alongside encoder reads and LED refresh.
bool presence_detected();

#endif  // PRESENCE_H
