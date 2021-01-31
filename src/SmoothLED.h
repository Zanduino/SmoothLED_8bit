/*! @file SmoothLED.h

@mainpage Arduino Library Header to use 8-bit PWM on any pin

@section Smooth_LED_intro_section Description

Hardware PWM on the Atmel ATMega microprocessors can only be done on certain pins, and these are
attached to specific timers. This library allows doing 8-bit PWM with 256 levels on any pin,
regardless of whether they support hardware PWM or not. By default, if the pin can support hardware
PWM this library will use, although that can be disabled during the begin() call.

Since the PWM is done in software, it "steals" CPU cycles from the main sketch and the more LEDs
defined in the library the more CPU cycles it consumes.

User documentation is located at https://github.com/Zanduino/SmoothLED_8bit/wiki

@section Smooth_LED_doxygen Doxyygen configuration
This library is built with the standard "Doxyfile", which is located at
https://github.com/Zanduino/Common/blob/main/Doxygen. As described on that page, there are only 5
environment variables used, and these are set in the project's actions file, located at
https://github.com/Zanduino/SmoothLED_8bit/blob/master/.github/workflows/ci-doxygen.yml
Edit this file and set the 5 variables: PRETTYNAME, PROJECT_NAME, PROJECT_NUMBER, PROJECT_BRIEF and
PROJECT_LOGO so that these values are used in the doxygen documentation. The local copy of the
doxyfile should be in the project's root directory in order to do local doxygen testing, but the
file is ignored on upload to GitHub.

@section Smooth_LEDclang clang-format
Part of the GitHub actions for CI is running every source file through "clang-format" to ensure
that coding formatting is done the same for all files. The configuration file ".clang-format" is
located at https://github.com/Zanduino/Common/tree/main/clang-format and this is used for CI tests
when pushing to GitHub. The local file, if present in the root directory, is ignored when
committing and uploading.

@section Smooth_LEDlicense GNU General Public License v3.0

This program is free software: you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version. This program is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details. You should have
received a copy of the GNU General Public License along with this program.  If not, see
<http://www.gnu.org/licenses/>.

@section Smooth_LED_author Author

Written by Arnd <Arnd@Zanduino.Com> at https://www.github.com/SV-Zanshin

@section Smooth_LED_versions Changelog

| Version| Date       | Developer  | Comments                                                      |
| ------ | ---------- | ---------- | ------------------------------------------------------------- |
| 1.0.0  | 2021-01-21 | SV-Zanshin | Created new library for the class                             |
*/

#ifndef _smoothLED_h
#define _smoothLED_h
#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif
#define CIE_MODE_ACTIVE //!< Set the CIE 1931 mode to be active
#ifdef CIE_MODE_ACTIVE
/*! @brief   Linear PWM brightness progression table using CIE brightness levels
    @details CIE 1931 color space and PWM. Fading a LED with PWM from 255 to 0 linearly will not
             result in a visually linear fade due to the way our eyes work. Setting PWM to half
             strength (50% duty cycle) actually looks much brighter than it should.  The formula for
             lightness uses floating point and cubes so is not fast enough for an Arduino to compute
             on the fly at runtime. Hence a table is used for the lookup, but this comes at a price
             of 256Bytes of memory which is a pretty significant amount of memory for a small micro-
             processor.  CIE mode is turned on by default in this library, but if the following
             "#define CIE_MODE" is commented out the library will shrink by over 2kB in size and the
             dimming / fading will no longer have CIE adjustments applied.
             The kcie table was generated using a program written by Jared Sanson and explained on
             https://jared.geek.nz/2013/feb/linear-led-pwm. */

const PROGMEM uint8_t kcie[] = {
    0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,
    2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,
    4,   5,   5,   5,   5,   5,   6,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,   8,   8,
    9,   9,   9,   9,   10,  10,  10,  11,  11,  11,  11,  12,  12,  12,  13,  13,  13,  14,  14,
    15,  15,  15,  16,  16,  16,  17,  17,  18,  18,  19,  19,  19,  20,  20,  21,  21,  22,  22,
    23,  23,  24,  24,  25,  25,  26,  27,  27,  28,  28,  29,  29,  30,  31,  31,  32,  33,  33,
    34,  35,  35,  36,  37,  37,  38,  39,  39,  40,  41,  42,  42,  43,  44,  45,  45,  46,  47,
    48,  49,  50,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  59,  60,  61,  62,  63,  64,
    65,  66,  67,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  80,  81,  82,  83,  84,  85,
    87,  88,  89,  90,  92,  93,  94,  96,  97,  98,  100, 101, 102, 104, 105, 106, 108, 109, 111,
    112, 114, 115, 117, 118, 120, 121, 123, 124, 126, 127, 129, 131, 132, 134, 136, 137, 139, 141,
    142, 144, 146, 147, 149, 151, 153, 155, 156, 158, 160, 162, 164, 166, 168, 169, 171, 173, 175,
    177, 179, 181, 183, 185, 187, 189, 191, 194, 196, 198, 200, 202, 204, 206, 209, 211, 213, 215,
    218, 220, 222, 224, 227, 229, 231, 234, 236};
#endif

/***************************************************************************************************
** Not all of these macros are defined on all platforms, so redefine them here just in case       **
***************************************************************************************************/
#ifndef _BV
#define _BV(bit) (1 << (bit))  //!< bit shift macro
#endif
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))  //!< clear bit macro
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))  //!<  set bit macro
#endif
/***************************************************************************************************
** Define all constants that are to be globally visible                                           **
***************************************************************************************************/
const uint8_t NO_INVERT_LED{0};  //!< Default. When value is 0 it means off
const uint8_t INVERT_LED{1};     //!< A Value of 0 denotes 100% duty cycle when set
const uint8_t CIE_MODE{0};       //!< Default. Interpolate brightness using CIE table
const uint8_t NO_CIE_MODE{2};    //!< Use the PWM value directly, do not interpolate values
const uint8_t HARDWARE_MODE{0};  //!< Default. Use hardware PWM where possible
const uint8_t SOFTWARE_MODE{4};  //!< Use software PWM even on Hardware PWM pins
/*! Define the linked list structure for stacking set() commands */
struct setStructure {
  uint8_t       targetLevel{0};  //!< next target level
  uint16_t      changeSpeed{0};  //!< next change speed
  uint16_t      delayMS{0};      //!< next wait time
  setStructure* next{nullptr};   //!< next element in list
};                               // of struct "setStructure"
class smoothLED {
  /*!
    @class   smoothLED
    @brief   Class to allow any Arduino pins to be used with 10-bit PWM
  */
 public:                                                            // Declare visible members
  smoothLED();                                                      // Class constructor
  ~smoothLED();                                                     // Class destructor
  smoothLED(const smoothLED&) = delete;                             // disable copy constructor
  smoothLED(smoothLED&& led)  = delete;                             // disable move constructor
  smoothLED   operator++(int) = delete;                             // disallow postfix increment
  smoothLED   operator--(int) = delete;                             // disallow postfix decrement
  smoothLED&  operator++();                                         // prefix increment overload
  smoothLED&  operator--();                                         // prefix decrement overload
  smoothLED&  operator+=(const int16_t& value);                     // addition overload
  smoothLED&  operator-=(const int16_t& value);                     // subtraction overload
  smoothLED&  operator+(const int16_t& value);                      // addition overload
  smoothLED&  operator-(const int16_t& value);                      // subtraction overload
  bool        begin(const uint8_t pin, const uint8_t flags = 0);    // Initialize a pin for PWM
  void        set(const uint8_t  val   = 0,                         // Set a pin's PWM value
                  const uint16_t speed = 0,                         // Change speed in ms, optional
                  const uint16_t delay = 0);                        // Delay after fade, optional
  void        setNow(const uint8_t  val   = 0,                      // Set PWM value and override
                     const uint16_t speed = 0,                      // Change speed in ms, optional
                     const uint16_t delay = 0);                     // Delay after fade, optional
  static void pwmISR();                                             // Function for software PWM
  static void faderISR();                                           // Function for fading
 private:                                                           // declare private class
  static smoothLED* _firstLink;                                     //!< Static ptr to 1st instance
  static uint8_t    _counterPWM;                                    //!< Counter variable in ISR()
  smoothLED*        _nextLink{nullptr};                             //!< Ptr to the next instance
  volatile uint8_t  _flags{0};                                      //!< Status bits, see cpp file
  volatile uint8_t* _portRegister{nullptr};                         //!< Pointer to PORT{n} Register
  uint8_t           _registerBitMask{0};                            //!< bit mask used in PORT{n}
  uint8_t           _timerPWMPin{0};                                //!< Timer to Pin number for PWM
  volatile uint8_t* _PWMRegister{nullptr};                          //!< Ptr to the 8bit register
  volatile uint8_t  _currentLevel{0};                               //!< Current PWM level 0-255
  volatile uint8_t  _currentCIE{0};                                 //!< PWM level from cie table
  volatile uint16_t _waitTime{0};                                   //!< Time to wait after fade
  uint8_t           _targetLevel{0};                                //!< Target PWM level 0-255
  uint16_t          _changeDelays{0};                               //!< Delay milliseconds in fades
  volatile int16_t  _changeTicker{0};                               //!< Countdown timer for fading
  setStructure*     _nextSet{nullptr};                              //!< Next "set()" command to run
  void              switchHardwarePWM(const bool state);            // Turn HW PWM on or off
  inline void       pinOn() const __attribute__((always_inline));   // Turn LED on
  inline void       pinOff() const __attribute__((always_inline));  // Turn LED off
};                                                                  // of class definition
#endif
