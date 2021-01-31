/*! @file SmoothLED.cpp
 @section SmoothLEDcpp_intro_section Description

Arduino Library for Smooth LED library\n\n
See main library header file for details
*/

#include "SmoothLED.h"

#define fadeTimerOn TIMSK0 |= _BV(OCIE0A);    //!< Enable the interrupt on TIMER0 Match A
#define fadeTimerOff TIMSK0 &= ~_BV(OCIE0A);  //!< Disable the interrupt on TIMER0 Match A
#define pwmTimerOn TIMSK1 |= _BV(TOIE1);      //!< Enable the interrupt on TIMER1 Overflow
#define pwmTimerOff TIMSK1 &= ~_BV(TOIE1);    //!< Disable the interrupt on TIMER0 Overflow
const uint8_t PWM_ACTIVE{8};                  //!< Set when PWM is active on the pin (not 0 or 255)
const uint8_t TIMER1_PIN{16};                 //!< Set pin is on TIMER1, needs special handling
smoothLED *smoothLED::_firstLink{nullptr};    // static member declaration outside of class for init
uint8_t    smoothLED::_counterPWM{0};         // static pwm loop counter

smoothLED::smoothLED() {
  /*!
  @brief   Class constructor
  @details There can be many instances of this class, as many as one per pin for the given ATMega
           processor. Instead of preallocating storage we'll create a simple linked list of class
           instances with just forward pointers and no backwards pointers. The interrupt routine
           uses this list to iterate through all instances and perform the appropriate PWM actions
  */
  if (_firstLink == nullptr) {            // If first instantiation
    _firstLink = this;                    // This is the first link (static variable)
    _nextLink  = nullptr;                 // no next link in list
  } else {                                // otherwise
    smoothLED *last = _firstLink;         // Working pointer to determine last link
    while (last->_nextLink != nullptr) {  // loop to find last link
      last = last->_nextLink;             // go to next link
    }                                     // while not last link loop
    last->_nextLink = this;               // previous link now points to this instance
    _nextLink       = nullptr;            // this instance has no next link
  }                                       // if-then-else first instance of class
}  // of smoothLED class constructor
smoothLED::~smoothLED() {
  /*!
  @brief   Class destructor
  @details Class instances are constructed like a FIFO stack, so the first instance destroyed is the
           last one that was instantiated. So when we destroy an instance we just need to remove the
           last link in the list of instances.  When destroying the last surviving instance we
           disable any interrupts that have been set and null the static first link pointer.
  */
  uint8_t originalSREG = SREG;               // Save original SREG value before disabling interrupts
  cli();                                     // disable interrupts while changing registers
  while (_nextSet != nullptr) {              // free up any stored "set()" actions
    setStructure *tempPtr = _nextSet;        // Save current address
    _nextSet              = _nextSet->next;  // point to next element in list
    delete tempPtr;                          // free up storage
  }                                          // while we have stored actions remove
  if (this == _firstLink) {                  // remove interrupts if this is the last instance
    fadeTimerOff;                            // disable fade timer
    pwmTimerOff;                             // disable PWM timer
    _firstLink = nullptr;                    // and null out first link
  } else {                                   // otherwise
    smoothLED *p = _firstLink;               // set pointer to first link in order to traverse list
    while (p->_nextLink != this) {           // loop until we get to next-to-last link in list
      p = p->_nextLink;                      // increment to next element
    }                                        // of while loop to traverse linked list
    p->_nextLink = nullptr;                  // remove the last element from linked list
  }                                          // if-then-else only link
  SREG = originalSREG;                       // Restore interrupt state to original
}  // of class destructor
smoothLED &smoothLED::operator++() {
  /*!
    @brief   ++ Overload
    @details The "++" pre-increment operator increments the target LED level
  */
  ++_targetLevel;  // increment
  fadeTimerOn;     // turn on fade interrupt
  pwmTimerOn;      // turn on PWM interrupt
  return *this;    // Return new class value
}  // of overload
smoothLED &smoothLED::operator--() {
  /*!
    @brief   -- Overload
    @details The "--" pre-decrement operator increments the target LED level
  */
  --_targetLevel;  // decrement
  fadeTimerOn;     // turn on fade interrupt
  pwmTimerOn;      // turn on PWM interrupt
  return *this;    // Return new class value
}  // of overload
smoothLED &smoothLED::operator+(const int16_t &value) {
  /*!
    @brief   + Overload
    @details The "+" operator increments the target LED level by the specified value
  */
  this->_targetLevel += value;
  fadeTimerOn;   // turn on fade interrupt
  pwmTimerOn;    // turn on PWM interrupt
  return *this;  // Return new class value
}
smoothLED &smoothLED::operator-(const int16_t &value) {
  /*!
    @brief   - Overload
    @details The "-" operator decrements the target LED level by the specified value
  */
  this->_targetLevel -= value;
  fadeTimerOn;   // turn on fade interrupt
  pwmTimerOn;    // turn on PWM interrupt
  return *this;  // Return new class value
}
smoothLED &smoothLED::operator+=(const int16_t &value) {
  /*!
  @brief   += Overload
  @details The "+=" operator increments the target LED level by the specified value
*/
  this->_targetLevel += value;
  fadeTimerOn;   // turn on fade interrupt
  pwmTimerOn;    // turn on PWM interrupt
  return *this;  // Return new class value
}
smoothLED &smoothLED::operator-=(const int16_t &value) {
  /*!
  @brief   - Overload
  @details The "-" operator decrements the target LED level by the specified value
*/
  this->_targetLevel -= value;
  fadeTimerOn;   // turn on fade interrupt
  pwmTimerOn;    // turn on PWM interrupt
  return *this;  // Return new class value
}
ISR(TIMER0_COMPA_vect) {
  /*!
    @brief   Interrupt vector for TIMER0_COMPA
    @details Indirect call to the faderISR() which performs fading every millisecond
  */
  smoothLED::faderISR();  // call the actual handler
}  // ISR "TIMER0_COMPA_vect()"
ISR(TIMER1_OVF_vect) {
  /*!
    @brief   Interrupt vector for TIMER1_OVF
    @details Indirect call to the pwmISR() which is called frequently to perform software PWM
  */
  smoothLED::pwmISR();  // call the actual handler
}  // ISR "TIMER0_COMPA_vect()"
void smoothLED::pinOn() const {
  /*!
  @brief   Turn the LED to 100% on
  @details Since keeping PWM on and setting the register to the highest value doesn't actually
           result in a 100% duty cyle, this function turns on PWM and does a digital write to set
           the pin to 1
  @return  void returns nothing
*/
  if (_flags & INVERT_LED) {
    *_portRegister &= ~_registerBitMask;
  } else {
    *_portRegister |= _registerBitMask;
  }  // if-then-else _inverted
}  // of function "pinOn()"
void smoothLED::pinOff() const {
  /*!
    @brief   Turn the LED off
    @details Since keeping PWM on and setting the register to the lowest value doesn't actually
             result in the LED being completely off, this function turns off PWM and does a digital
             write to set  the pin to 0
    @return  void returns nothing
  */
  if (_flags & INVERT_LED) {
    *_portRegister |= _registerBitMask;
  } else {
    *_portRegister &= ~_registerBitMask;
  }  // if-then-else _inverted
}  // of function "pinOff()"
void smoothLED::pwmISR() {
  /*!
  @brief     Function to actually perform software PWM on all pins
  @details   This function is the interrupt handler for TIMER1_OVF and performs PWM turning ON and
             OFF of all the pins defined in the instances of the class that use software PWM.  It is
             called very often and therefore needs to be as compact as possible so as to impact the
             main program as little as as possible . The TIMER1 is set so that this results in a
             rate of about 60Hz (60 * 256 times a second). At 16MHz the microprocessor only executes
             16 instructions per microsecond so it is really important to minimize time spent here.
             This function iterates through all the instances of the smoothLED class and sets each
             pin ON or OFF for the appropriate number of cycles.
             When no pins have active software PWM (values "OFF" and "ON" turn off PWM), then this
             interrupt is disabled until needed to minimize impact.
  */
  smoothLED *p = _firstLink;                // Local pointer set to start of linked list
  while (p != nullptr) {                    // Loop through linked list until end is reached
    if (p->_portRegister != nullptr &&      // Skip processing if the pin is not initialized,
        (p->_flags & SOFTWARE_MODE)) {      // or isn't performing software PWM
      if (p->_currentCIE == _counterPWM) {  // If we've reached the PWM counter threshold
        p->pinOff();                        // then turn pin off
      } else {                              // otherwise
        if (_counterPWM == 0) {             // if we've rolled over and are at the beginning,
          p->pinOn();                       // turn the pin on
        }                                   // if-then turn ON LED
      }                                     // if-then-else turn off LED
    }                                       // if then a valid pin
    p = p->_nextLink;                       // go to next class instance
  }                                         // of while loop to traverse  list
  ++_counterPWM;                            // Pre-increment, overflows from 255 back to 0
}  // of function "pwmISR()"
bool smoothLED::begin(const uint8_t pin, const uint8_t flags) {
  /*!
    @brief     Initializes the LED
    @details   The function returns an error (false) if a pin doesn't exist, or the pin has already
               been defined. The pin is made an output pin and the register address for the PORT
               number and bitmask are stored with the class instance along with flag on whether the
               LED is inverted (where 0 denotes full ON and 255 means OFF); as LEDs can be attached
               to the pin in either direction.
    @param[in] pin   The Arduino pin number of the LED
    @param[in] flags Various flags can be passed, they can be "OR"d or added together:
                     either "NO_INVERT_LED" (default) or "INVERT_LED"
                     either "CIE_MODE" (default) or "NO_CIE_MODE"
                     either "HARDWARE_MODE" (default) or "SOFTWARE_MODE"
    @return    bool  TRUE on success, FALSE when the pin is not a PWM-Capable one
  */
  if (pin > NUM_DIGITAL_PINS) return false;                      // return immediately when bad pin
  uint8_t originalSREG = SREG;                                   // Save original SREG value
  cli();                                                         // disable interrupts
  _flags           = flags & B1111;                              // Copy first 4 flag bits
  _registerBitMask = digitalPinToBitMask(pin);                   // get the bitmask for pin
  _portRegister    = portOutputRegister(digitalPinToPort(pin));  // get PORTn for pin
  smoothLED *p     = _firstLink;                                 // Start pointer at top of list
  bool       firstBegin{true};                                   // Remains true if no others init
  while (p != nullptr) {                                         // loop through all instances
    if (p->_portRegister == _portRegister &&                     // Check to see if re-using
        p->_registerBitMask == _registerBitMask &&               // a pin already defined
        p != this) {                                             // and skip our own link
      _portRegister = nullptr;                                   // set back to null
      SREG          = originalSREG;                              // Restore registers
      return false;                                              // return error
    } else {                                                     // otherwise
      if (p->_portRegister != nullptr &&                         // If _portRegister is not
          p->_portRegister != _portRegister) {                   // null and not current,
        firstBegin = false;                                      // then set flag to false
      }                                                          // if-then first begin() call
    }                                                            // if-then reusing pin
    p = p->_nextLink;                                            // increment to next element
  }                                                              // of while loop
  if (firstBegin) {
    /***********************************************************************************************
     ** TIMER0 is used by the Arduino system for timing. The timer is set so that it triggers an  **
     ** interrupt at overflow that takes care of timings with the millis() function). We piggy-   **
     ** back off this timer using OCR0A (comparison register A) to get a 1KHz interrupt rate for  **
     ** software brightening/fading effects. This interrupt is only enable and active when there's**
     ** an active fade in progress, otherwise it is disabled to save CPU cycles. Start off with   **
     ** the interrupt disabled until needed.                                                      **
     **********************************************************************************************/
#if defined(TIMSK0)
    fadeTimerOff;
#else
#error Register TIMSK0 is not defined
#endif
    /***********************************************************************************************
    ** TIMER1 is generally a 16-bit timer and we use this for high-speed interrupts for the soft- **
    ** ware PWM functionality.  Unfortunately most ways of making use of this timer preclude the  **
    ** use of the 2 or 3 hardware PWM pins attached to the timer. Hence we "cheat" and set the WGM**
    ** (waveform generation mode) to "Fast PWM" and "9-bit" mode. This, coupled with setting the  **
    ** pre-scaler to "no pre-scaling", give us an effective PWM of around 60Hz. This rate is      **
    ** sufficiently fast to avoid any flickering at low power levels but still leaves enough CPU  **
    ** cycles for the main program to be only moderately affected during software PWM.            **
    ** This interrupt is turned off when there are no pins requiring software PWM. A non-PWM pin  **
    ** set to "OFF" (0) or "ON" (255) does not require PWM.                                       **
    ***********************************************************************************************/
#if defined(OCR1AL) && defined(TIMSK1)
    pwmTimerOff;        // Disable the interrupt on TIMER1 Overflow until we need it
    sbi(TCCR1B, CS10);  // Set 3 "Clock Select" bits to no pre-scaling
    cbi(TCCR1B, CS11);
    cbi(TCCR1B, CS12);
    sbi(TCCR1A, WGM10);  // Set "Fast PWM, 10-bit" mode
    sbi(TCCR1A, WGM11);
    sbi(TCCR1B, WGM12);
    cbi(TCCR1B, WGM13);
#else
#error No TIMSK1 defined for 16-bit register TIMER1
#endif
  }                                       // if-then first begin() call
  _timerPWMPin = digitalPinToTimer(pin);  // Get timer register for pin
  if (_timerPWMPin != NOT_ON_TIMER) {     // If on a TIMER, then set up
    if (!(_flags & SOFTWARE_MODE)) {      // unless in software mode
      switchHardwarePWM(true);            // set PWM hardware mode
      /*********************************************************************************************
       ** Now determine which PWM timer register is associated with the pin. This can be an 8-bit **
       ** register or a 16-bit register, depending upon which timer it is associated with. Since  **
       ** the library has to handle all of the Atmel microcontrollers there are a quite a few     **
       ** conditional compile statements in the switch clause, but most of these are not used and **
       ** are therefore not compiled into the code at runtime. By default the Arduino IDE sets the**
       ** 16-bit timers to operate in 8-bit mode so no changes are made to the WGM and CS mode    **
       ** registers since it is assumed that the registers are setup correctly.                   **
       ********************************************************************************************/
      switch (_timerPWMPin) {
#if defined(TCCR0) && defined(COM00) && !defined(__AVR_ATmega8__)
        case TIMER0A:  // connect pwm to pin on timer 0
          _PWMRegister = &OCR0;
          break;
#endif
#if defined(TCCR0A) && defined(COM0A1)
        case TIMER0A:  // connect pwm to pin on timer 0, channel A
          _PWMRegister = &OCR0A;
          break;
#endif
#if defined(TCCR0A) && defined(COM0B1)
        case TIMER0B:  // connect pwm to pin on timer 0, channel B
          _PWMRegister = &OCR0B;
          break;
#endif
#if defined(TCCR1A) && defined(COM1A1)
        case TIMER1A:            // connect pwm to pin on timer 1, channel A
          _flags |= TIMER1_PIN;  // Pin is on TIMER1
          _PWMRegister = &OCR1AL;
          break;
#endif
#if defined(TCCR1B) && defined(COM1B1)
        case TIMER1B:            // connect pwm to pin on timer 1, channel B
          _flags |= TIMER1_PIN;  // Pin is on TIMER1
          _PWMRegister = &OCR1BL;
          break;
#endif
#if defined(TCCR1C) && defined(COM1C1)
        case TIMER1C:            // connect pwm to pin on timer 1, channel C
          _flags |= TIMER1_PIN;  // Pin is on TIMER1
          _PWMRegister = &OCR1CL;
          break;
#endif
#if defined(TCCR2) && defined(COM21)
        case TIMER2:  // connect pwm to pin on timer 2
          _PWMRegister = &OCR2;
          break;
#endif
#if defined(TCCR2A) && defined(COM2A1)
        case TIMER2A:  // connect pwm to pin on timer 2, channel A
          _PWMRegister = &OCR2A;
          break;
#endif
#if defined(TCCR2A) && defined(COM2B1)
        case TIMER2B:  // connect pwm to pin on timer 2, channel B
          _PWMRegister = &OCR2B;
          break;
#endif
#if defined(TCCR3A) && defined(COM3A1)
        case TIMER3A:  // connect pwm to pin on timer 3, channel A
          _PWMRegister = &OCR3AL;
          break;
#endif
#if defined(TCCR3A) && defined(COM3B1)
        case TIMER3B:  // connect pwm to pin on timer 3, channel B
          _PWMRegister = &OCR3BL;
          break;
#endif
#if defined(TCCR3A) && defined(COM3C1)
        case TIMER3C:  // connect pwm to pin on timer 3, channel C
          _PWMRegister = &OCR3CL;
          break;
#endif
#if defined(TCCR4A)
        case TIMER4A:  // connect pwm to pin on timer 4, channel A
#if defined(OCR4AH)
          _PWMRegister = &OCR4AL;
#else
          _PWMRegister = &OCR4A;
#endif
          break;
#endif
#if defined(TCCR4A) && defined(COM4B1)
        case TIMER4B:  // connect pwm to pin on timer 4, channel B
#if defined(OCR4BH)
          _PWMRegister = &OCR4BL;
#else
          _PWMRegister = &OCR4B;
#endif
          break;
#endif
#if defined(TCCR4A) && defined(COM4C1)
        case TIMER4C:  // connect pwm to pin on timer 4, channel C
#if defined(OCR4CH)
          _PWMRegister = &OCR4CL;
#else
          _PWMRegister = &OCR4C;
#endif
          break;
#endif
#if defined(TCCR4C) && defined(COM4D1)
        case TIMER4D:  // connect pwm to pin on timer 4, channel D
          _PWMRegister = &OCR4D;
          break;
#endif
#if defined(TCCR5A) && defined(COM5A1)
        case TIMER5A:  // connect pwm to pin on timer 5, channel A
          _PWMRegister = &OCR5AL;
          break;
#endif
#if defined(TCCR5A) && defined(COM5B1)
        case TIMER5B:  // connect pwm to pin on timer 5, channel B
          _PWMRegister = &OCR5BL;
          break;
#endif
#if defined(TCCR5A) && defined(COM5C1)
        case TIMER5C:  // connect pwm to pin on timer 5, channel C
          _PWMRegister = &OCR5CL;
          break;
#endif
      }                        // of switch
    }                          // if-then software mode
  } else {                     // otherwise
    _flags |= SOFTWARE_MODE;   // non-PWM pins are set to software mode
    switchHardwarePWM(false);  // set PWM hardware mode
  }                            // if-then-else hardware PWM pin
  volatile uint8_t *ddr = portModeRegister(digitalPinToPort(pin));  // get DDRn port for pin
  *ddr |= _registerBitMask;                                         // make the pin an output
  set(0);                                                           // Turn off to start with
  SREG = originalSREG;                                              // Restore registers
  return true;                                                      // Return success
}  // of function "begin()"
void smoothLED::switchHardwarePWM(const bool state) {
  /*!
    @brief     Turns hardware PWM on or off
    @details   This function sets the registers on hardware PWM pins. If hardware PWM is turned off,
               then the corresponding COMnXn bit is unset to disable the pin being changed,
               otherwise the bit is turned on to the default "clear on compare match" mode.
               If the pin is not capable of hardware PWM then nothing is done and the routine
               returns.
    @param[in] state Boolean TRUE or hardware PWM turned "ON", otherwise "OFF"
  */
  if (_timerPWMPin == NOT_ON_TIMER) {  // Not a PWM capable pin
    _flags |= SOFTWARE_MODE;           // set the flag to software mode
    return;                            // and return
  } else {
    if (state && !(_flags & SOFTWARE_MODE)) {  // if ON and not in software mode
      switch (_timerPWMPin) {
#if defined(TCCR0) && defined(COM00) && !defined(__AVR_ATmega8__)
        case TIMER0A:  // connect pwm to pin on timer 0
          sbi(TCCR0, COM00);
          break;
#endif
#if defined(TCCR0A) && defined(COM0A1)
        case TIMER0A:  // connect pwm to pin on timer 0, channel A
          sbi(TCCR0A, COM0A1);
          break;
#endif
#if defined(TCCR0A) && defined(COM0B1)
        case TIMER0B:  // connect pwm to pin on timer 0, channel B
          sbi(TCCR0A, COM0B1);
          break;
#endif
#if defined(TCCR1A) && defined(COM1A1)
        case TIMER1A:  // connect pwm to pin on timer 1, channel A
          sbi(TCCR1A, COM1A1);
          break;
#endif
#if defined(TCCR1A) && defined(COM1B1)
        case TIMER1B:  // connect pwm to pin on timer 1, channel B
          sbi(TCCR1A, COM1B1);
          break;
#endif
#if defined(TCCR1A) && defined(COM1C1)
        case TIMER1C:  // connect pwm to pin on timer 1, channel C
          sbi(TCCR1A, COM1C1);
          break;
#endif
#if defined(TCCR2) && defined(COM21)
        case TIMER2:  // connect pwm to pin on timer 2
          sbi(TCCR2, COM21);
          break;
#endif
#if defined(TCCR2A) && defined(COM2A1)
        case TIMER2A:  // connect pwm to pin on timer 2, channel A
          sbi(TCCR2A, COM2A1);
          break;
#endif
#if defined(TCCR2A) && defined(COM2B1)
        case TIMER2B:  // connect pwm to pin on timer 2, channel B
          sbi(TCCR2A, COM2B1);
          break;
#endif
#if defined(TCCR3A) && defined(COM3A1)
        case TIMER3A:  // connect pwm to pin on timer 3, channel A
          sbi(TCCR3A, COM3A1);
          break;
#endif
#if defined(TCCR3A) && defined(COM3B1)
        case TIMER3B:  // connect pwm to pin on timer 3, channel B
          sbi(TCCR3A, COM3B1);
          break;
#endif
#if defined(TCCR3A) && defined(COM3C1)
        case TIMER3C:  // connect pwm to pin on timer 3, channel C
          sbi(TCCR3A, COM3C1);
          break;
#endif
#if defined(TCCR4A)
        case TIMER4A:  // connect pwm to pin on timer 4, channel A
          sbi(TCCR4A, COM4A1);
#if defined(COM4A0)  // only used on 32U4
          cbi(TCCR4A, COM4A0);
#endif
          break;
#endif
#if defined(TCCR4A) && defined(COM4B1)
        case TIMER4B:  // connect pwm to pin on timer 4, channel B
          sbi(TCCR4A, COM4B1);
          break;
#endif
#if defined(TCCR4A) && defined(COM4C1)
        case TIMER4C:  // connect pwm to pin on timer 4, channel C
          sbi(TCCR4A, COM4C1);
          break;
#endif
#if defined(TCCR4C) && defined(COM4D1)
        case TIMER4D:  // connect pwm to pin on timer 4, channel D
          sbi(TCCR4C, COM4D1);
#if defined(COM4D0)  // only used on 32U4
          cbi(TCCR4C, COM4D0);
#endif
          break;
#endif
#if defined(TCCR5A) && defined(COM5A1)
        case TIMER5A:  // connect pwm to pin on timer 5, channel A
          sbi(TCCR5A, COM5A1);
          break;
#endif
#if defined(TCCR5A) && defined(COM5B1)
        case TIMER5B:  // connect pwm to pin on timer 5, channel B
          sbi(TCCR5A, COM5B1);
          break;
#endif
#if defined(TCCR5A) && defined(COM5C1)
        case TIMER5C:  // connect pwm to pin on timer 5, channel C
          sbi(TCCR5A, COM5C1);
          break;
#endif
      }  // of switch
    } else {
      switch (_timerPWMPin) {
#if defined(TCCR0) && defined(COM00) && !defined(__AVR_ATmega8__)
        case TIMER0A:  // connect pwm to pin on timer 0
          cbi(TCCR0, COM00);
          break;
#endif
#if defined(TCCR0A) && defined(COM0A1)
        case TIMER0A:  // connect pwm to pin on timer 0, channel A
          cbi(TCCR0A, COM0A1);
          break;
#endif
#if defined(TCCR0A) && defined(COM0B1)
        case TIMER0B:  // connect pwm to pin on timer 0, channel B
          cbi(TCCR0A, COM0B1);
          break;
#endif
#if defined(TCCR1A) && defined(COM1A1)
        case TIMER1A:  // connect pwm to pin on timer 1, channel A
          cbi(TCCR1A, COM1A1);
          break;
#endif
#if defined(TCCR1A) && defined(COM1B1)
        case TIMER1B:  // connect pwm to pin on timer 1, channel B
          cbi(TCCR1A, COM1B1);
          break;
#endif
#if defined(TCCR1A) && defined(COM1C1)
        case TIMER1C:  // connect pwm to pin on timer 1, channel C
          cbi(TCCR1A, COM1C1);
          break;
#endif
#if defined(TCCR2) && defined(COM21)
        case TIMER2:  // connect pwm to pin on timer 2
          cbi(TCCR2, COM21);
          break;
#endif
#if defined(TCCR2A) && defined(COM2A1)
        case TIMER2A:  // connect pwm to pin on timer 2, channel A
          cbi(TCCR2A, COM2A1);
          break;
#endif
#if defined(TCCR2A) && defined(COM2B1)
        case TIMER2B:  // connect pwm to pin on timer 2, channel B
          cbi(TCCR2A, COM2B1);
          break;
#endif
#if defined(TCCR3A) && defined(COM3A1)
        case TIMER3A:  // connect pwm to pin on timer 3, channel A
          cbi(TCCR3A, COM3A1);
          break;
#endif
#if defined(TCCR3A) && defined(COM3B1)
        case TIMER3B:  // connect pwm to pin on timer 3, channel B
          cbi(TCCR3A, COM3B1);
          break;
#endif
#if defined(TCCR3A) && defined(COM3C1)
        case TIMER3C:  // connect pwm to pin on timer 3, channel C
          cbi(TCCR3A, COM3C1);
          break;
#endif
#if defined(TCCR4A)
        case TIMER4A:  // connect pwm to pin on timer 4, channel A
          cbi(TCCR4A, COM4A1);
#if defined(COM4A0)  // only used on 32U4
          cbi(TCCR4A, COM4A0);
#endif
          break;
#endif
#if defined(TCCR4A) && defined(COM4B1)
        case TIMER4B:  // connect pwm to pin on timer 4, channel B
          cbi(TCCR4A, COM4B1);
          break;
#endif
#if defined(TCCR4A) && defined(COM4C1)
        case TIMER4C:  // connect pwm to pin on timer 4, channel C
          cbi(TCCR4A, COM4C1);
          break;
#endif
#if defined(TCCR4C) && defined(COM4D1)
        case TIMER4D:  // connect pwm to pin on timer 4, channel D
          cbi(TCCR4C, COM4D1);
#if defined(COM4D0)  // only used on 32U4
          cbi(TCCR4C, COM4D0);
#endif
          break;
#endif
#if defined(TCCR5A) && defined(COM5A1)
        case TIMER5A:  // connect pwm to pin on timer 5, channel A
          cbi(TCCR5A, COM5A1);
          break;
#endif
#if defined(TCCR5A) && defined(COM5B1)
        case TIMER5B:  // connect pwm to pin on timer 5, channel B
          cbi(TCCR5A, COM5B1);
          break;
#endif
#if defined(TCCR5A) && defined(COM5C1)
        case TIMER5C:  // connect pwm to pin on timer 5, channel C
          cbi(TCCR5A, COM5C1);
          break;
#endif
      }  // of switch
    }    // if ON or OFF mode
  }      // if-then-else not a PWM pin
}  // of function "hardwarePWM()"
void smoothLED::set(const uint8_t val, const uint16_t speed, const uint16_t delay) { /*!
   @brief     sets the LED
   @details   This public function is called to set the instance variables used for software and
              hardware PWM to the appropriate values. The actual setting of pin as well as the
              software PWM fading is done in the "faderISR()" function.
   @param[in] val   The value 0-255 to set the LED. Defaults to 0 (OFF)
   @param[in] speed The rate of change in milliseconds.
   @param[in] delay The delay in milliseconds after reaching target
 */
  uint8_t originalSREG = SREG;  // Save original SREG value before disabling interrupts
  cli();                        // disable interrupts while changing registers
  /*************************************************************************************************
   ** If there is no active fade going on (defined by current=target and no wait time), then      **
   ** perform this set(); otherwise add it onto the list of actions and it will get executed once **
   ** the current action is finished.                                                             **
   ************************************************************************************************/
  if (_currentLevel == _targetLevel && _waitTime == 0) {  // if no actions are active, then
    _targetLevel = val;                                   // set new target (regardless of mode),
    _waitTime    = delay;                                 // and set the post-fade delay time
    /***********************************************************************************************
    ** There are two distinct types of setup:                                                     **
    ** 1. Immediate: When "speed" is 0, then immediately set the pin to the requested PWM value   **
    ** 2. Fade:      When "speed" is nonzero, we fade from whatever the current setting is to the **
    **               target value at a speed computed here.                                       **
    ***********************************************************************************************/
    if (speed == 0) {       // Set a value directly and immediately
      _currentLevel = val;  // set current to value to force an immediate set
    } else {                // otherwise we have a delta between actual and target
      /*********************************************************************************************
      ** Compute the "_changeDelays" value from the delta between current and target and the speed**
      ** value. Since the interrupt is called 1000 times a second we compute "1000/delta" and     **
      ** multiply by 128 so that we don't need to use floating point. This is what the variable   **
      ** "_changeTicker" is set to and each interrupt this is decremented by 128 until it is 0 or **
      ** less, then the fade is applied. So if a delta is 500 then "1000 * 128 / delta" = 256     **
      ** (rounded to int). A fade is done done every 2 calls to the interrupt for this example.   **
      *********************************************************************************************/
      uint32_t temp = (_currentLevel > _targetLevel) ? _currentLevel - _targetLevel
                                                     : _targetLevel - _currentLevel;
      temp = ((uint32_t)speed << 7) / temp;         // compute the delay factor, see comments above
      if (temp > UINT16_MAX) {                      // if the value is bigger than fits
        temp = UINT16_MAX;                          // clamp it to range,
      } else if (temp < 128) {                      // and if it is less than minimum
        temp = 128;                                 // then set it to minimum
      }                                             // if-then-else out of range
      _changeDelays = static_cast<uint16_t>(temp);  // Set the value, knowing it is in range
    }                                               // if-then-else immediate change or fading
    fadeTimerOn;                                    // turn on fade interrupt
    pwmTimerOn;                                     // turn on PWM interrupt
  } else {
    /***********************************************************************************************
    ** Attempt to allocate space for storing the action, skip and ignore if unsuccessful          **
    ***********************************************************************************************/
    setStructure *p = new setStructure;
    if (p != nullptr) {
      p->changeSpeed = speed;
      p->delayMS     = delay;
      p->targetLevel = val;
      p->next        = nullptr;
      if (_nextSet == nullptr) {
        _nextSet = p;  // this is the first element
      } else {
        setStructure *tmpPtr = _nextSet;  // start at beginning of list
        while (tmpPtr->next != nullptr) {
          tmpPtr = tmpPtr->next;
        }                  // while go to end of list
        tmpPtr->next = p;  // assign the next element to list
      }                    // if-then first in list
    }                      // if-then we can allocate space
  }                        // if-then no active fade
  SREG = originalSREG;     // Restore interrupts register
}  // of function "set()"
void smoothLED::setNow(const uint8_t val, const uint16_t speed, const uint16_t delay) {
  /*!
    @brief     sets the LED, and cancels any active or stored actions
    @details   This function is identical to "set()", but will override any active and stored
               actions for the pin, unlike "set()".
    @param[in] val   The value 0-255 to set the LED. Defaults to 0 (OFF)
    @param[in] speed The rate of change in milliseconds.
    @param[in] delay The delay in milliseconds after reaching target
 */
  uint8_t originalSREG = SREG;               // Save original SREG value before disabling interrupts
  cli();                                     // disable interrupts while changing registers
  while (_nextSet != nullptr) {              // free up any stored "set()" actions
    setStructure *tempPtr = _nextSet;        // Save current address
    _nextSet              = _nextSet->next;  // point to next element in list
    delete tempPtr;                          // free up storage
  }                                          // while we have stored actions to remove
  _currentLevel = _targetLevel;              // make equal for set() call to work
  _waitTime     = 0;                         // set to zero for set() call to work
  set(val, speed, delay);                    // and now call set()
  SREG = originalSREG;                       // Restore register interrupts
}  // of function "setnow()"
void smoothLED::faderISR() {
  /*!
    @brief   Performs fading PWM functions
    @details While the "pwmISR()" is called via TIMER0_COMPA_vect every millisecond, since we are
             piggybacking off the standard TIMER0 settings which are used by the Arduino IDE for the
             millis() timing function.
  */
  bool turnPWMoff{true};   // set to false when any pin has software PWM
  bool turnFadeOff{true};  // set to false when any pin is fading
  /*************************************************************************************************
  ** Traverse the whole linked list, checking each LED pin to see if we need to do something      **
  *************************************************************************************************/
  smoothLED *p = _firstLink;            // set ptr to first link for loop
  while (p != nullptr) {                // loop through all class instances
    if (p->_portRegister != nullptr) {  // Skip processing if the pin is not initialized
      /*********************************************************************************************
      ** If the pin hasn't reached the target level, then perform the dynamic PWM change at the   **
      ** appropriate speed                                                                        **
      *********************************************************************************************/
      if (p->_currentLevel != p->_targetLevel) {     // Perform the fade
        turnFadeOff = false;                         // Fading still active
        p->_changeTicker -= 128;                     // decrement the ticker
        if (p->_changeTicker <= 0) {                 // When ticker goes to zero
          p->_changeTicker += p->_changeDelays;      // add delay factor to ticker for new value
          if (p->_currentLevel > p->_targetLevel) {  // choose direction
            --p->_currentLevel;                      // current > target
          } else {                                   // otherwise
            ++p->_currentLevel;                      // current < target
          }                                          // if-then-else get dimmer
        }                                            // if-then  change current value
      } else {                                       // otherwise we have current = target, so
        if (p->_waitTime) {                          // and if we have a wait time then
          --p->_waitTime;                            // decrement it and make sure to mark
          turnFadeOff = false;                       // the fading status as still active
        } else {
          /*****************************************************************************************
          ** If we've reached the target setting and have no wait cycles, then check to see if    **
          ** there is a another set() command on the stack. If so, we pop it off the stack and    **
          ** set the values to the cached command                                                 **
          *****************************************************************************************/
          if (p->_nextSet != nullptr) {
            turnFadeOff          = false;        // switch flag off
            setStructure *tmpPtr = p->_nextSet;  // point to beginning
            p->set(tmpPtr->targetLevel, tmpPtr->changeSpeed, tmpPtr->delayMS);  // set new values
            p->_nextSet = p->_nextSet->next;  // link to next one in list
            delete tmpPtr;                    // free up the space in linked list
            break;                            // leave loop
          }                                   // if-then we have another set command
        }                                     // if-then-else waitTime is nonzero
      }                                       // if-then-else no change in PWM
      /*********************************************************************************************
      ** Compute the CIE or use the value directly if CIE is turned off                           **
      *********************************************************************************************/
#ifdef CIE_MODE_ACTIVE
      if (p->_flags & NO_CIE_MODE) {
        p->_currentCIE = p->_currentLevel;
      } else {
        p->_currentCIE = pgm_read_word(kcie + p->_currentLevel);
      }  // if-then no CIE
#else
      p->_currentCIE = p->_currentLevel;
#endif
      /*********************************************************************************************
       ** If the pin is set to "ON" or "OFF", then turn off PWM and explicitly set the pin to the **
       ** state requested                                                                         **
       ********************************************************************************************/
      if (p->_currentLevel == 0 || p->_currentLevel == 255) {  // if PWM on and value is ON or OFF
        p->_flags &= ~PWM_ACTIVE;                              // turn off PWM flag
        p->switchHardwarePWM(false);                           // turn off PWM mode if using HW PWM
        if (p->_currentLevel == 0) {
          p->pinOff();
        } else {
          p->pinOn();
        }  // if-then turn "OFF" or "ON"
      } else {
        p->_flags |= PWM_ACTIVE;  // turn on PWM flag
        /*******************************************************************************************
        ** If we have a PWM value and we are in hardware mode, then actually set the register to  **
        ** the PWM value. If we are in software mode, then do nothing, the toggling is handled by **
        ** the interrupt handler "pwmISR()".                                                      **
        *******************************************************************************************/
        if (!(p->_flags & SOFTWARE_MODE)) {  // If we are in hardware PWM mode
          p->switchHardwarePWM(true);        // turn on PWM mode if using HW pin
          if (p->_flags & INVERT_LED) {      // Set depending upon inverted flag state
            if (p->_flags & TIMER1_PIN) {    // TIMER1 pins are 10 bit and re-casting and shifting
              *(volatile uint16_t *)p->_PWMRegister = ((uint16_t)(255) - p->_currentCIE) << 2;
            } else {
              *p->_PWMRegister = 255 - p->_currentCIE;
            }  // if-then TIMER1 pin
          } else {
            if (p->_flags & TIMER1_PIN) {  // TIMER1 pins are 10 bit and re-casting and shifting
              *(volatile uint16_t *)p->_PWMRegister = (uint16_t)p->_currentCIE << 2;
            } else {
              *p->_PWMRegister = p->_currentCIE;
            }  // if-then TIMER1 pin
          }    // if-then inverted
        }      // if-then hardware PWM
      }        // if-then-else "ON" or "OFF"

      if ((p->_flags & PWM_ACTIVE) &&     // and PWM is on,
          (p->_flags & SOFTWARE_MODE)) {  // and not using hardware mode
        turnPWMoff = false;               // set flag
      }                                   // if-then software PWM
    }                                     // if pin defined
    p = p->_nextLink;                     // go to next class instance
  }                                       // of while loop to traverse list
  /*************************************************************************************************
  ** If no pins in our class instances are actively fading, the we can turn off this interrupt    **
  ** and save a bit of CPU cycles. Interrupts are re-enabled in the "set()" function              **
  *************************************************************************************************/
  if (turnFadeOff) {  // Disable interrupts when not needed
    fadeTimerOff;
    /***********************************************************************************************
    ** And perform a check to see if we can turn off TIMER1 if no pins in our class instances are **
    ** using software PWM,  then we can save lots of CPU cycles by disabling it. Interrupts are   **
    ** re-enabled in the "set()" function                                                         **
    ***********************************************************************************************/
    if (turnPWMoff) {  // Disable interrupts when not needed
      pwmTimerOff;
    }  // if-then turn PWM off
  }    // if-then turn off fading
}  // of function "faderISR()"
