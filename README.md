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

### Pin assignment notes

- All Teensy 4.x digital pins support interrupts, so encoder pins can go anywhere.
- Keep the presence sensor on **Serial1 (pins 0/1)**. The mid-numbered pins (5-8, 14, 20/21) are the usual DMA parallel-output candidates for the LED driver, and Serial2/Serial3 sit in that same block.
- Assign LED data pins explicitly in the config file rather than accepting a library default that may collide.

## Chain of command / order of operations:
1. user moves of the rotary encoders
2. rotary encoder change leads to its corresponding midi signal output
3. rotary encoder change effects its corresponding LED behavior

## Lighting

This project has two kinds of lighting: general and zoned

General: These lights respond generally to a combination of all rotary encoder inputs. for simplicity in v0 they should just color cycle

Zoned: Exists in 3 sections each with their own behavior

### Zone 1 - Aux panel (two of these)
- each aux panel contains 4 knobs, each knob controls its own 3 leds
- total of 12 lights per panel
- each knobs 3 led matrix will represent the state of the knob with a corresponiding color intensity that correlates to the status of the knob
- knob default will be midi intensity 63/127
- default color will the midpoint of two complimentary colors and as the knob turns each direction will make the color of the leds gravitate towards one of the primary colors. example: all the way left is yellow, all the way right is blue, midpoint is green with shifting intensity

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


## Overall system Behavior:
A mm wave sensor is polling for the presence of a person, the system will remain in an idle state until a disruption is detected, at which point the active state will trigger. as soon as the mm wave sensor stops detecting a person it will return to idle state. transition between states will include a delay of 4 seconds, meaning the sensor must detect or not detect something for 4 consecutive seconds before changing states

There is a set of active default and passive default values for both the leds and the midi. 

upon change of states from active back to passive, each midi and led value will not retain its value. meaning once the "session" is completed, the next session will not retain the prior sessions midi or led values and will start back at the active default

### Passive state
- none of the knobs should work in the passive state, not reading input
- each midi is at its default passive value
- each every led is set to its default passive value

### Transition function passive -> active
- to be defined later

### Transition function active -> passive
- to be defined later

### Active state

1. normal relation of knob to Midi and LED: as the knob is turned it shifts default values either larger or smaller and changed the corresponding leds as specified above

___

### other coding specs:
- a file should be created that can control the default value for each knobs MIDI and LED number. this will contain the passive state defaults and active state defaults
- 
