# SelfServeSoundbath

## Background: The self serve soundbath is an audiovisual art installation. A 5.1 surround sound system has been installed into a bathtub 

## Project Description:
A Teensy 4.1 needs to be programmed for the following functionality:
1. A MIDI controller with 16 knobs as input
   a. (ranging 0 - 127, two 360 degree rotations for full range - this should be an easily configurable variable, some may only need one rotation)
2. A lighting controller for 3 zones of WS2812b LEDs

## Hardware:
Teensy 4.1 - microcontroller
- 14 generic EC11 detented rotary encoders, 20 detents - synth controlling knobs, button not needed
- 2 Bournes PEC12R-4022 detentless rotary encoders - synth controlling knobs, button not needed
- 3 runs of WS2812b LEDs - One is 48 LEDs long, one is ~180 LEDs long, and one is ~90 LEDs long
- LD2410C mmWave sensor or VL53L1X ToF sensor (undecided, please write logic for both, comment out the ToF sensor) - for presence detection in tub. LD2410C is UART (Serial RX/TX), VL53L1X is I2C (SDA/SCL) - 2 pins either way
- As many toggle switches as needed to fill the rest of the top GPIO inputs, for any mode changes on hidden control panels (lighting modes or any other nice toggle-able features you can think of)
- 5v 20A power supply
- The sound system agnostic
- Mac running Logic is doing the audio synthesis, and is ready to configure around given MIDI controls from the Teensy.
- The tub is 30x60in on the outside top, and tapers in to 18x40in on the bottom inside, with the front wall relatively flat and the back wall sloped.

## Pin budget

The Teensy 4.1 breaks out 42 breadboard-friendly digital I/O (pins 0-41). Pins 42-54 exist as surface pads on the bottom, but several are tied to the SD socket - don't plan on them.

| Use | Pins |
| --- | --- |
| 16 encoders x 2 | 32 |
| Presence sensor (Serial RX/TX, or SDA/SCL) | 2 |
| WS2812b data, 3 runs | 3 |
| **Used** | **37** |
| **Free** | **5** |

Those 5 pins cover toggle switches **and** any encoder push buttons - it is one shared pool, not 5 on top of the toggles.

### Encoder push buttons

Not in scope: both encoder types are specced button-not-needed and nothing in the state machine uses a press. If a function is ever named for them (per-knob reset to active default being the obvious one), do not wire them directly - 16 direct buttons do not fit.

Options, cheapest first:
- **MCP23017 on I2C** - 16 inputs for 2 pins, 3 left for toggles. Free if the VL53L1X path is chosen, since I2C is already up. A second chip covers the toggles on the same 2 pins.
- **Resistor ladder on one analog pin** - all 16 on 1 pin, 4 left for toggles. No chip, but one press at a time, and the thresholds need a calibration table (see Calibration), not baked-in constants.
- **Direct wiring** - 5 buttons max, zero toggles. A 2x3 matrix gets 6 for the same 5 pins plus 6 diodes; not worth it.

Buttons are slow SPST-to-ground contacts, so polling an expander at ~100 Hz is fine. Encoders are not - they stay on real GPIO.

### Pin assignments

The category table above says how many pins go where; this is the actual map, from `config.h`.

| Pin(s) | Use |
| --- | --- |
| 0, 1 | presence sensor - Serial1 RX/TX (LD2410C) |
| 2 | LED data - zoned run (48 LEDs) |
| 3 | LED data - general run A (~180 LEDs) |
| 4 | LED data - general run B (~90 LEDs) |
| 5 | day/night toggle |
| 6, 7, 8 | reserved - undefined in v0 |
| 9, 10 | encoder - aux panel A, knob 1 |
| 11, 12 | encoder - aux panel A, knob 2 |
| 13 | heartbeat LED (onboard) |
| 14, 15 | encoder - aux panel A, knob 3 |
| 16, 17 | encoder - aux panel A, knob 4 |
| 18, 19 | encoder - aux panel B, knob 1 |
| 20, 21 | encoder - aux panel B, knob 2 |
| 22, 23 | encoder - aux panel B, knob 3 |
| 24, 25 | encoder - aux panel B, knob 4 |
| 26, 27 | encoder - jet panel, knob 1 |
| 28, 29 | encoder - jet panel, knob 2 |
| 30, 31 | encoder - jet panel, knob 3 |
| 32, 33 | encoder - jet panel, knob 4 |
| 34, 35 | encoder - jet panel, special (black->white) knob |
| 36, 37 | encoder - faucet panel, alpha (cold) |
| 38, 39 | encoder - faucet panel, beta (hot) |
| 40, 41 | encoder - faucet panel, gamma |

What's still free: pin 13 is kept free of the encoder/toggle pool for the heartbeat LED; pins 6-8 are reserved for toggles the spec doesn't define yet; and the 13 bottom surface pads (42-54) remain further headroom, with the existing caveat that several are tied to the SD socket.

### Toggle switches

v0 wires exactly one: day/night, on pin 5. Daytime multiplies every LED value by 0.5. Pins 6-8 are reserved but unimplemented, so the "as many toggle switches as needed... for any mode changes" note above is otherwise not built yet.

Polarity: pin 5 is `INPUT_PULLUP`, and the switch closed to ground (reading `LOW`) means daytime. An installer wiring or labeling the switch by feel should treat closed/grounded as day, open as night.

### Pin assignment notes

- All Teensy 4.x digital pins support interrupts, so encoder pins can go anywhere.
- Keep the presence sensor on **Serial1 (pins 0/1)**. The mid-numbered pins (5-8, 14, 20/21) are the usual DMA parallel-output candidates for the LED driver, and Serial2/Serial3 sit in that same block.
- Assign LED data pins explicitly in the config file rather than accepting a library default that may collide.

## Chain of command / order of operations:
1. user moves of the rotary encoders
2. rotary encoder change leads to its corresponding midi signal output
3. rotary encoder change effects its corresponding LED behavior

## MIDI CC map

All 16 knobs send on channel 1, CC 102-117, one per knob, in the order below. 102-117 are undefined in the MIDI spec, so nothing in Logic claims them by accident.

| CC | Knob |
| --- | --- |
| 102 | aux panel A, knob 1 |
| 103 | aux panel A, knob 2 |
| 104 | aux panel A, knob 3 |
| 105 | aux panel A, knob 4 |
| 106 | aux panel B, knob 1 |
| 107 | aux panel B, knob 2 |
| 108 | aux panel B, knob 3 |
| 109 | aux panel B, knob 4 |
| 110 | jet panel, knob 1 |
| 111 | jet panel, knob 2 |
| 112 | jet panel, knob 3 |
| 113 | jet panel, knob 4 |
| 114 | jet panel, special black->white knob |
| 115 | faucet panel, alpha (cold) |
| 116 | faucet panel, beta (hot) |
| 117 | faucet panel, gamma |

## Firmware

v0 lives in eight files:
- `config.h` - single source of truth: pin map, the 16-row knob table (CC number, encoder pins, LED indices, active/passive defaults, counts-per-full-range scaling), and the tuning constants
- `mapping.h` - pure encoder/color math, shared by the firmware and its host test
- `leds.h` / `leds.cpp` - OctoWS2811 DMA output and the per-zone renderers
- `presence.h` / `presence.cpp` - LD2410C live, VL53L1X compiled out behind one interface
- `SelfServeSoundbath.ino` - the 16 encoders, MIDI CC out, and the idle/active state machine
- `test/test_mapping.cpp` - host-side test for `mapping.h`'s pure math, no hardware needed

Build:
```
arduino-cli lib install ld2410
arduino-cli compile --fqbn teensy:avr:teensy41:usb=midi .
```

Host test (pure math, no hardware needed):
```
c++ -std=c++17 test/test_mapping.cpp -o /tmp/t && /tmp/t
```

**Tools → USB Type must be a MIDI-capable mode** (`MIDI` or `Serial + MIDI`) in the Arduino IDE, or `usbMIDI.*` compiles fine and silently does nothing.

Flipping `presence.cpp`'s `PRESENCE_SENSOR_LD2410` to `0` switches to the VL53L1X path, which needs the Pololu `VL53L1X` library installed (`arduino-cli lib install "VL53L1X"`) and has never been compiled or bench-tested — the LD2410C is the live, verified path.

## Lighting

This project has two kinds of lighting: general and zoned

General: These lights respond generally to a combination of all rotary encoder inputs. for simplicity in v0 they should just color cycle

Zoned: Exists in 3 sections each with their own behavior

### LED run topology

The three runs are three separate data pins. The 48-LED zoned run is one daisy chain covering all three zones, addressed by index range:

| Section | Indices |
| --- | --- |
| aux panel A | 0-11 |
| aux panel B | 12-23 |
| jet panel | 24-38 |
| faucet panel | 39-47 |

The ~180 and ~90 runs are the general lighting. The jet panel's special knob gets 3 LEDs like the other four, which is what makes the jet panel 15 LEDs and the zoned total 48.

Faucet panel letter-to-index mapping: a=39, b=40, c=41, d=42, e=43, f=44, g=45, h=46, i=47.

### Power budget

`config.h`'s `LED_BRIGHTNESS_CEILING` (0..255, currently 96) is a **safety limit** against the 5V 20A supply, not an aesthetic choice: all ~318 LEDs across the three runs at full white would draw roughly 19A, which leaves almost no headroom. Do not raise the ceiling without measuring actual current draw with a clamp meter on the bench.

### Zone 1 - Aux panel (two of these)
- each aux panel contains 4 knobs, each knob controls its own 3 leds
- total of 12 lights per panel
- each knobs 3 led matrix will represent the state of the knob with a corresponiding color intensity that correlates to the status of the knob
- knob default will be midi intensity 63/127
- default color will the midpoint of two complimentary colors and as the knob turns each direction will make the color of the leds gravitate towards one of the primary colors. example: all the way left is yellow, all the way right is blue, midpoint is green with shifting intensity
- this interpolation is over hue, not a straight RGB blend: a straight RGB blend from yellow to blue passes through grey at the midpoint, while a hue sweep passes through green, which is the behavior the example above describes

### Zone 2 - Jet panel
- jet panel has 5 knobs, 4 of which behave exactly like described in zone 1
- 1 special jet panel knob will just go from black to white as the knob turns, default at 50% brightness

### Zone 3 - Faucet Panel
- this zone has 9 lights, and 3 knobs
- knob alpha, detentless (cold) will control light a directly. its color will be pure blue
- knob beta, detentless, (hot) will control light b directly. its color will be pure red
- as the user turns alpha and beta they will either decrease or increase their intensity of blue and red respectively
- alpha and beta mix their color inputs to control lights c,d,e. their color will be a mixed gradient of the colors of lights a and b. meaning light c will be the closest in color to a and light e will be the closest in color to light b
- knob gamma will control lights f,g,h,i
- the lights will correspond to the left right direction of knob turns, splitting in the middle and indicating knob direction with their brightness
- as knob gamma turns more to the right it will increase the brightness of lights h and i and decrease the colors of f and g and vice versa
- gamma's f, g, h, i are white. all four sit at half brightness at center; turning right ramps h,i toward full while f,g fade toward off, and turning left mirrors it
- alpha and beta default to 63/127 each, so light a rests at half blue, light b at half red, and c, d, e show a balanced blue -> purple -> red blend
- lights c, d, e are an RGB blend of a's and b's colors at 25%, 50%, and 75% - so c sits nearest a and e nearest b, as above


## Overall system Behavior:
A mm wave sensor is polling for the presence of a person, the system will remain in an idle state until a disruption is detected, at which point the active state will trigger. as soon as the mm wave sensor stops detecting a person it will return to idle state. transition between states will include a delay of 4 seconds, meaning the sensor must detect or not detect something for 4 consecutive seconds before changing states

There is a set of active default and passive default values for both the leds and the midi. 

upon change of states from active back to passive, each midi and led value will not retain its value. meaning once the "session" is completed, the next session will not retain the prior sessions midi or led values and will start back at the active default

### Passive state
- none of the knobs should work in the passive state, not reading input
- each midi is at its default passive value: 0, for all 16 knobs. this default is per-knob in the config file, and v0 sets it to 0 across the board because some synth parameters sound broken rather than quiet at zero - those are expected to be retuned there with Logic open
- zoned LEDs (aux, jet, faucet panels) are off; general lighting (the ~180 and ~90 runs) slow-breathes at ~10% of the brightness ceiling

### Transition function passive -> active
v0's transition is an instant snap in both directions - no fade, no ramp. Passive -> active:
- every encoder counter is re-seeded from its knob's active default, discarding whatever accumulated while idle
- all 16 CCs are sent
- the zoned LEDs wake

### Transition function active -> passive
Also an instant snap. Active -> passive:
- all 16 CCs are sent at their passive defaults
- the zoned LEDs go dark
- the general runs drop to the slow breathe

Every CC leaves through a single send function, so replacing the snap with an eventual "LEDs snap, MIDI crossfades" transition is a change in one place rather than a rewrite - worth keeping in mind if that's ever wanted.

### Active state

1. normal relation of knob to Midi and LED: as the knob is turned it shifts default values either larger or smaller and changed the corresponding leds as specified above

___

### other coding specs:
- a file should be created that can control the default value for each knobs MIDI and LED number. this will contain the passive state defaults and active state defaults
- 
