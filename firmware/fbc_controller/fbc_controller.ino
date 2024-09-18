/**
 * Footwell Backlight Controller
 * 
 *   - Independent two-channel control with smooth fade transitions
 *   - Delayed switch-off feature (can be disabled via T58b input)
 *   - Low power standby mode
 * 
 *   Atmel ATtiny25/45/85 Pinout
 * 
 *                                ┌───────u───────┐
 *          PCINT5/RESET/ADC0/dW ─┤ 1 PB5   VCC 8 ├─ 
 *   PCINT3/XTAL1/CLKI/OC1B/ADC3 ─┤ 2 PB3   PB2 7 ├─ SCK/USCK/SCL/ADC1/T0/INT0/PCINT2
 *   PCINT4/XTAL2/CLKO/OC1B/ADC2 ─┤ 3 PB4   PB1 6 ├─ MISO/DO/AIN1/OC0B/OC1A/PCINT1
 *                               ─┤ 4 GND   PB0 5 ├─ MOSI/DI/SDA/AIN0/OC0A/OC1A/AREF/PCINT0
 *                                └───────────────┘
 *   Pins General Info
 * 
 *           ┌─────────┬───────────┬─────────────┬─────────────┬─────────┐
 *           │ IC Lead │ Atmel Pin │ Arduino Pin │ Arduino ISP │ Purpose │
 *           ├─────────┼───────────┼─────────────┼─────────────┼─────────┤
 *           │    1    │    PB5    │             │     10      │   RST   │
 *           │    2    │    PB3    │ (D 3) Ain3  │             │   SW0   │
 *           │    3    │    PB4    │ (D 4) Ain2  │             │   SW1   │
 *           │    5    │    PB0    │ (D 0) pwm0  │     11      │   BL1   │
 *           │    6    │    PB1    │ (D 1) pwm1  │     12      │   BL0   │
 *           │    7    │    PB2    │ (D 2) Ain1  │     13      │   ILM   │
 *           └─────────┴───────────┴─────────────┴─────────────┴─────────┘
 * 
 *   Programming ATtiny25/45/85 with Arduino ISP
 *     1. Install attiny core files: https://github.com/damellis/attiny
 *     2. Prepare an Arduino:
 *       Examples > 11. ArduinoISP > ArduinoISP
 *       Sketch > Upload
 *     3. Wire ISP
 *       Arduino > ATtiny25/45/85
 *       5V      > VCC
 *       GND     > GND
 *       13      > PB2
 *       12      > PB1
 *       11      > PB0
 *       10      > RST
 *     4. Tools > Clock: "Internal 8MHz"
 *     5. Tools > Burn Bootloader
 *     6. Sketch > Upload Using Programmer
 *
 *   Fuses
 *     https://www.engbedded.com/fusecalc/?P=ATtiny25&V_LOW=0xE2&V_HIGH=0xDF&V_EXTENDED=0xFF&O_HEX=Apply+values
 * 
 *   Controller States
 * 
 *                                     ┌─────┐
 *              ┌──←─ door opened ─←───┤ OFF ├─← fade down completed ─┐
 *              │                      └─────┘                        │
 *              ↓          ┌──────←─ door opened ─←───────┐           ↑
 *         ┌────┴──────────┴────┐                   ┌─────┴───────────┴─────┐
 *         │ FADE_BACKLIGHT_UP  ├─→─ door closed ─→─┤  FADE_BACKLIGHT_DOWN  │
 *         └──────────┬─────────┘                   └───────────┬───────────┘
 *  fade up completed ↓                                         ↑ delay completed or illumination is on                     
 *            ┌───────┴───────┐                        ┌── ─────┴─────────┐
 *            │ BACKLIGHT_ON  ├───→─ door closed ─→────┤ SWITCH_OFF_DELAY │
 *            └───────┬───────┘                        └────────┬─────────┘
 *                    └───────────←─ door opened ─←─────────────┘
 */

#define ILLUMINATION_INPUT 2

#define FADE_STEP 1
#define SWITCH_OFF_DELAY_VALUE 15000

bool isIlluminationActive() {
  return digitalRead(ILLUMINATION_INPUT) == LOW;
}

enum StateLevel {
  OFF,
  FADE_BACKLIGHT_UP,
  BACKLIGHT_ON,
  SWITCH_OFF_DELAY,
  FADE_BACKLIGHT_DOWN,
};

struct channel {
  uint8_t inputPin;
  uint8_t backlightOutputPin;

  StateLevel state = OFF;
  uint8_t backlightValue = 0;
  uint16_t switchOffDelay = 0;

  bool previousInputValue = false;
  bool previousIlluminationValue = false;

  channel(uint8_t inputPin, uint8_t backlightOutputPin): inputPin(inputPin), backlightOutputPin(backlightOutputPin) {}
  
  bool isInputActive() {
    return digitalRead(inputPin) == LOW;
  }

  bool isInputsChanged() {
    return isInputActive() ^ previousInputValue || isIlluminationActive() ^ previousIlluminationValue;
  }

  bool isOff() {
    return state == OFF;
  }

  bool isFading() {
    return state == FADE_BACKLIGHT_UP || state == FADE_BACKLIGHT_DOWN;
  }

  bool isDelay() {
    return state == SWITCH_OFF_DELAY;
  }

  void setup() {
    setupInputPin(inputPin);
    digitalWrite(backlightOutputPin, LOW);
    pinMode(backlightOutputPin, OUTPUT);
    if (isInputsChanged()) {
      onChange();
    }
  }

  void writeOutput() {    
    analogWrite(backlightOutputPin, backlightValue);
    setNextBacklightValue();
  }

  void setNextBacklightValue() {
    if (state == FADE_BACKLIGHT_UP) {
      if (backlightValue != 255) {
        backlightValue = min(255, backlightValue + FADE_STEP);
      } else {
        onChange();
      }
    } else if (state == FADE_BACKLIGHT_DOWN) {
      if (backlightValue != 0) {
        backlightValue = max(0, backlightValue - FADE_STEP);
      } else {
        onChange();
      }
    }
  }

  void decrementSwitchOffDelay() {
    if (switchOffDelay > 0) {
      switchOffDelay = max(switchOffDelay - 4, 0);
    } else if (state == SWITCH_OFF_DELAY) {      
      onChange();
    }
  }

  void onChange() {
    switch (state) {
      case OFF:
      case FADE_BACKLIGHT_DOWN:
        if (isInputActive()) {
          state = FADE_BACKLIGHT_UP;
        } else if (backlightValue == 0) {
          state = OFF;
        }
        break;

      case FADE_BACKLIGHT_UP:
        if (!isInputActive()) {
          state = FADE_BACKLIGHT_DOWN;
        } else if (backlightValue == 255) {
          state = BACKLIGHT_ON;
        }
        break;

      case BACKLIGHT_ON:
        if (!isInputActive()) {
          if (isIlluminationActive()) {
            state = FADE_BACKLIGHT_DOWN;
          } else {
            switchOffDelay = SWITCH_OFF_DELAY_VALUE;
            state = SWITCH_OFF_DELAY;
          }
        }
        break;

      case SWITCH_OFF_DELAY:        
        if (isInputActive()) {
          state = BACKLIGHT_ON;
        } else if (isIlluminationActive() || switchOffDelay == 0) {
          state = FADE_BACKLIGHT_DOWN;
        }
        break;
    }
    previousInputValue = isInputActive();
    previousIlluminationValue = isIlluminationActive();
  }
};

#define CH_COUNT 2
channel channels[CH_COUNT] = {{3, 1}, {4, 0}};

void setupTimer1() {
  // millis();
  // Clear registers
  TCNT1 = 0;  
  // CTC and Prescaler 128
  TCCR1 = (1 << CTC1) | (1 << CS13);
  // 255.10204081632654 Hz (8000000/((244+1)*128))
  OCR1C = 255;
  // interrupt COMPA
  OCR1A = OCR1C;
  // Output Compare Match A Interrupt Enable
  TIMSK |= (1 << OCIE1A);
}

void setupInputPin(uint8_t inputPin) {
  pinMode(inputPin, INPUT_PULLUP);
  *digitalPinToPCMSK(inputPin) |= (1<<digitalPinToPCMSKbit(inputPin));
  *digitalPinToPCICR(inputPin) |= (1<<digitalPinToPCICRbit(inputPin));
}

void setupPowerSaveMode() {
  // Disable ADC
  ADCSRA &= ~(1<<ADEN);
  PRR |= 1 << PRADC;
  // Power down mode and sleep enable
  MCUCR |= 1 << SM1 | 1 << SE;
}

ISR(TIMER1_COMPA_vect) {
  if (channels[0].isOff() && channels[1].isOff()) {
    sei();
    asm volatile("sleep");
    return;
  }

  for (uint8_t i = 0; i < CH_COUNT; i++) {
    if (channels[i].isFading()) {
      channels[i].writeOutput();
    } else if (channels[i].isDelay()) {
      channels[i].decrementSwitchOffDelay();
    }
  }
}

ISR (PCINT0_vect) {
  for (uint8_t i = 0; i < CH_COUNT; i++) {
    if (channels[i].isInputsChanged()) {
      channels[i].onChange();
    }
  }
}

void setup() {
  cli();
  setupPowerSaveMode();
  setupInputPin(ILLUMINATION_INPUT);
  channels[0].setup();
  channels[1].setup();
  setupTimer1();
  sei();
}

void loop() {
  // put your main code here, to run repeatedly:
}
