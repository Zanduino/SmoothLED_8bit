/*! @file Builtin_LED_Demo.ino

@section _intro_section Description

Example for smoothLED using the Arduino built-in LED\n\n

This simple sketch illustrates the basic calls to the smoothLED library, using the builtin LED which
almost every Arduino has.

@section sram_read_write_testlicense GNU General Public License v3.0
This program is free software: you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version. This program is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details. You should have
received a copy of the GNU General Public License along with this program.  If not, see
<http://www.gnu.org/licenses/>.

@section Builtin_LED_Demoauthor Author

Written by Arnd <Arnd@Zanduino.Com> at https://www.github.com/SV-Zanshin

@section Builtin_LED_Demoversions Changelog

| Version| Date       | Developer  | Comments                                                      |
| ------ | ---------- | ---------- | ------------------------------------------------------------- |
| 1.0.0  | 2021-01-31 | SV-Zanshin | Adapted to 8-bit version                                      |
| 1.0.0  | 2021-01-19 | SV-Zanshin | Updated set() call to use milliseconds                        |
| 1.0.0  | 2021-01-17 | SV-Zanshin | Initial coding                                                |
*/

#include "SmoothLED.h"  // Include the library
#ifndef __AVR__
  #error This library and program is designed for Atmel ATMega processors
#endif

smoothLED Board;  //!< instance of smoothLED pointing to the builtin LED

void setup() {
  /*!
      @brief    Arduino method called once at startup to initialize the system
      @details  This is an Arduino IDE method which is called first upon boot or restart. It is only
                called one time and then control goes to the main "loop()" method, from which
                control never returns
      @return   void
  */
  Serial.begin(115200);
#ifdef __AVR_ATmega32U4__  // If a 32U4 processor, wait32 seconds
  delay(3000);
#endif
  Serial.println("Starting SmoothLED test program");
  Serial.print("Initializing Built-in LED (pin ");
  Serial.print(LED_BUILTIN);
  Serial.println("). ");
  if (Board.begin(LED_BUILTIN))
    Serial.println("Success");
  else
    Serial.println("Error!");
  /*************************************************************************************************
  ** This is the classic way, manually changing the PWM value from ON to OFF and introducing a    **
  ** delay so that it doesn't too fast. While the LED is being dimmed the sketch can't do anything**
  ** else.                                                                                        **
  *************************************************************************************************/
  Serial.println("Test 1 - manually fading the LED");
  for (uint16_t i = 255; i != 0; i--) {
    Board.set(i);  // set the pin to the PWM value
    delay(25);     // wait a bit
  }                // for-next manual dimming
  Serial.println("Test 2 - fading the LED using the library");
  Board.set(255);      // set full on
  Board.set(0, 5000);  // fade to 0 over 5000ms (5 seconds)
  Serial.println("Program continues, fade happens in the background");

  /* example loop - while this is running the onboard LED slowly fades away, independant of this. */
  for (uint8_t i = 0; i < 10; ++i) {
    Serial.print("*");
    delay(500);
  }  // for next-loop
  Serial.println();
}  // of method "setup()"

void loop() {
  /*!
      @brief    Arduino method for the main program loop
      @details  Main program for the Arduino IDE, it is an infinite loop and keeps on repeating
      @return   void
  */
  Board.set(0);               // set off
  Board.set(255, 1000, 500);  // fade to full over 1 second and wait 500ms
  Board.set(0, 500);          // fade off quickly
  delay(10000);
  Serial.println("Next loop.");
}  // of method "loop()"
