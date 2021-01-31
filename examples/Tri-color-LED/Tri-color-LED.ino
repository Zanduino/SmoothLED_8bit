/*! @file Tri-color-LED.ino

@section Tri-color-LED_intro_section Description

Example for using the smoothLED library using a 3-color LED with a common cathode

@section Tri-color-LED_license GNU General Public License v3.0
This program is free software: you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version. This program is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details. You should have
received a copy of the GNU General Public License along with this program.  If not, see
<http://www.gnu.org/licenses/>.

@section Tri-color-LED_Demoauthor Author

Written by Arnd <Arnd@Zanduino.Com> at https://www.github.com/SV-Zanshin

@section Tri-color-LED_Demoversions Changelog

| Version| Date       | Developer  | Comments                                                      |
| ------ | ---------- | ---------- | ------------------------------------------------------------- |
| 1.0.0  | 2021-01-31 | SV-Zanshin | Updated for 8-bit version                                     |
| 1.0.0  | 2021-01-20 | SV-Zanshin | Initial coding                                                |
*/

#include "SmoothLED.h"  // Include the library
#ifndef __AVR__
  #error This library and program is designed for Atmel ATMega processors
#endif

const uint8_t RED_PIN{11};    //!< Red Pin number
const uint8_t GREEN_PIN{10};  //!< Red Pin number
const uint8_t BLUE_PIN{9};    //!< Red Pin number

smoothLED red,  //!< instance of smoothLED pointing to red
    green,      //!< instance of smoothLED pointing to green
    blue;       //!< instance of smoothLED pointing to blue

void setup() {
  /*!
      @brief    Arduino method called once at startup to initialize the system
      @details  This is an Arduino IDE method which is called first upon boot or restart. It is only
                called one time and then control goes to the main "loop()" method, from which
                control never returns
      @return   void
  */
  Serial.begin(115200);
#ifdef __AVR_ATmega32U4__  // If a 32U4 processor, wait 2 seconds
  delay(3000);
#endif
  Serial.println("Starting Tri-color-LED test program");
  Serial.println("Initializing LEDs: ");

  Serial.print("Red  : ");
  if (red.begin(RED_PIN, INVERT_LED))
    Serial.println("Success");
  else
    Serial.println("Error!");

  Serial.print("Green: ");
  if (green.begin(GREEN_PIN, INVERT_LED))
    Serial.println("Success");
  else
    Serial.println("Error!");

  Serial.print("Blue : ");
  if (blue.begin(BLUE_PIN, INVERT_LED))
    Serial.println("Success");
  else
    Serial.println("Error!");

}  // of method "setup()"

void loop() {
  /*!
      @brief    Arduino method for the main program loop
      @details  Main program for the Arduino IDE, it is an infinite loop and keeps on repeating
      @return   void
  */
  Serial.println(F("Setting all 3 colors to middle value"));
  red.set(127);
  green.set(127);
  blue.set(127);
  Serial.println(F("Wait 5 seconds"));
  delay(5000);
  Serial.println(F("Fade green & blue off while raising red to full"));
  green.set(0, 5000);
  blue.set(0, 5000);
  red.set(255, 5000);
  delay(10000);
  red.set(127);
  green.set(127);
  blue.set(127);
  Serial.println(F("Fade red & blue off while raising green to full"));
  green.set(255, 5000);
  blue.set(0, 5000);
  red.set(0, 5000);
  delay(10000);
  red.set(127);
  green.set(127);
  blue.set(127);
  Serial.println(F("Fade red & green off while raising blue to full"));
  green.set(0, 5000);
  blue.set(255, 5000);
  red.set(0, 5000);
  delay(10000);
}  // of method "loop()"
