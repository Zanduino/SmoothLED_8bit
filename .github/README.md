[![License: GPL v3](https://zanduino.github.io/Badges/GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0) [![Build](https://github.com/Zanduino/SmoothLED_8bit/workflows/Build/badge.svg)](https://github.com/Zanduino/SmoothLED8_bit/actions?query=workflow%3ABuild) [![Format](https://github.com/Zanduino/SmoothLED_8bit/workflows/Format/badge.svg)](https://github.com/Zanduino/SmoothLED_8bit/actions?query=workflow%3AFormat) [![Wiki](https://zanduino.github.io/Badges/Documentation-Badge.svg)](https://github.com/Zanduino/SmoothLED_8bit/wiki) [![Doxygen](https://github.com/Zanduino/SmoothLED8bit/workflows/Doxygen/badge.svg)](https://Zanduino.github.io/SmoothLED_8bit/html/index.html) [![arduino-library-badge](https://www.ardu-badge.com/badge/Zanduino%20SmoothLED_8bit%20Library%2010-bit.svg?)](https://www.ardu-badge.com/Zanduino%20SmoothLED%20Library%208-bit)
# Smooth 8-bit LED control library

_Arduino_ library to control any number of LEDs on any available pins using 8-bit PWM with linear adjustment using CIE 1931 curves. Hardware PWM pins default to that mode but can also be switched to software PWM.

## Library description
Features:
- Any pin can be configured to do 8-bit PWM
- If a pin is PWM capable then hardware instead of software PWM is automatically selected, this reduces CPU load
- By default the CIE1931 lightness levels are used so that the PWM values look linear to the eye. This can be turned off per pin
- Brightening and fading a pin is done by the library in the background. For example, a call of "set(0);set(255,5000);" will turn an LED off and then brighten to FULL "ON" over 5 seconds. But it returns immediately and lets the program continue processing without having to wait 5 second.
- Inverted LEDs (for example, a 3-color LED with a common cathode) are supported
- Multiple LED commands are allowed. For example, a call of "set(0);set(255,1000,1000);set(0,1000);" will make the LED go off, then brighten to FULL over the course of 1 second and pause a second before finally fading back to OFF over the course of 1 second. And all of this happens in the background while the main program continues executing.

The library allows any number of pins, as many as the corresponding Atmel ATMega processor has, to be defined as 8-bit PWM output pins. It supports setting PWM values from 0-255 (where 0 is "OFF" and 255 is 100% "ON"). The library is optimized to use hardware PWM on any pins that support it, although this can optionally be turned off. Since the processing of PWM takes up CPU cycles in the background the library is optimized to turn off these expensive interrupts when they are not needed and turn them back on when required. Pins set to "OFF" (0) or "ON" (255) and pins using hardware PWM don't require any interrupts.

The details of how to setup the library along with all of the publicly available methods can be found on the [INA wiki pages](https://github.com/Zanduino/SmoothLED8bit/wiki).

Fading a LED so that it looks both smooth and linear requires a bit of work, including applying [CIE 1931 Compensation](https://github.com/Zanduino/SmoothLED8bit/wiki/CIE1931-Compensation). This makes a fade look linear end-to-end, but comes at a cost of reserving 512 Bytes of program memory. The link describes how to free up that memory if linear fading is not required.

Robert Heinlein coined the expression [TANSTAAFL](https://en.wikipedia.org/wiki/There_ain%27t_no_such_thing_as_a_free_lunch) and it certainly applies here - "_There ain't no such thing as a free lunch_". While certain pins support hardware PWM, they are bound to specific TIMER{n} registers. All of the other pins are relegated to being mere digital pins with only "on" or "off" settings.  This library uses the ATMega's TIMER0 and TIMER1 and creates an additional interrupt in the background on both which then takes care of setting the pin to "on" and "off" in the background (quickly enough so that it is effectively a PWM signal) and also for brightening and fading effects. But doing this via interrupts means that CPU cycles are being used and these affect how many CPU cycles are left for the currently active sketch. The more LEDs defined in the library and the higher the defined interrupt rate the less cycles are left over for the sketch.

The library uses 20 Bytes of memory per defined LED, and if several fade commands are "stacked", each stacked command temporarily allocates an additional 7 Bytes of memory until it is executed, whereupon that memory is freed.

## Documentation
The documentation has been done using Doxygen and can be found at [doxygen documentation](https://Zanduino.github.io/SmoothLED8bit/html/index.html)

[![Zanshin Logo](https://zanduino.github.io/Images/zanshinkanjitiny.gif) <img src="https://zanduino.github.io/Images/zanshintext.gif" width="75"/>](https://zanduino.github.io)
