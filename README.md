# SelfServeSoundbath

Background: The self serve soundbath is an audiovisual art installation. A 5.1 surround sound system has been installed into a bathtub 

Role: You are responsible for programming a Teensy 4.1 to act as a MIDI controller and lighting controller. You can govern the wiring to the Teensy as you need, there is no wiring diagram as of right now.
Hardware setup is still underway, so testing is not an option right now, try to get the code right on the first try.

Project Description:
A Teensy 4.1 needs to be programmed for the following functionality:
1. A MIDI controller with 16 knobs as input
   a. (ranging 0 - 127, two 360 degree rotations for full range - this should be an easily configurable variable, some may only need one rotation)
2. A lighting controller for 3 runs of WS2812b LEDs - one underlighting, one up the shower pole, and one running around all of the knobs
Detailer behavior is described in the Behavior section

Hardware:
Teensy 4.1 - microcontroller
14 generic EC11 detented rotary encoders, 20 detents - synth controlling knobs, button not needed
2 Bournes PEC12R-4022 detentless rotary encoders - synth controlling knobs, button not needed
3 runs of WS2812b LEDs - One is 48 LEDs long, one is ~180 LEDs long, and one is ~90 LEDs long
LD2410C mmWave sensor or VL53L1X ToF sensor (undecided, please write logic for both, comment out the ToF sensor) - for presence detection in tub
As many toggle switches as needed to fill the rest of the top GPIO inputs, for any mode changes on hidden control panels (lighting modes or any other nice toggle-able features you can think of)
5v 20A power supply
The sound system is all set, and a Mac running Logic is doing the audio synthesis, and is ready to configure around given MIDI controls from the Teensy.
The tub is 30x60in on the outside top, and tapers in to 18x40in on the bottom inside, with the front wall relatively flat and the back wall sloped.

Behavior:
On startup, there should be some sort of brief animation with the LEDs to verify functionality. Other than that, nothing is needed.
After startup, the tub will only be in one of two states: Idle, or Active.
The state change is governed by the presence sensor (mmWave or ToF undecided for now). 
The sensor is mounted at the front of the tub under the faucet, with an adjustable line of sight across the tub towards the rear sloped section.
If the sensor detects presence in the tub for more than 5 (or configurable) seconds
