#include "io.h"
#include <avr/interrupt.h>
#include <avr/io.h>

void setup_io(void) {
    // PB0, PB1 output, PB2-PB4 input
    DDRB = (DDRB & ~((1 << DDB2) | (1 << DDB3) | (1 << DDB4))) | (1 << DDB0) | (1 << DDB1);
    // Fast PWM, TOP=0xFF
    TCCR0A = (1 << WGM01) | (1 << WGM00);
    // Clock Select (F_CPU / N * 256, No prescaling - 3906Hz with 1Mhz clock)
    TCCR0B = (1 << CS00);
    // PB2-PB4 pullup
    PORTB |= (1 << PB2) | (1 << PB3) | (1 << PB4);
}

void set_pwm_output(channel_t idx, uint8_t duty) {
    // Note: If the OCR0A is set equal to BOTTOM, the output will be a narrow spike for each MAX+1 timer clock cycle
    // W/A - disable the fast pwm and force the output to low
    switch (idx) {
        case CHANNEL_L:
            OCR0A = duty;
            if (duty == 0) {
                TCCR0A &= ~(1 << COM0A1);
                PORTB &= ~(1 << PB0);
            } else {
                TCCR0A |= (1 << COM0A1);
            }
            break;
        case CHANNEL_R:
            OCR0B = duty;
            if (duty == 0) {
                TCCR0A &= ~(1 << COM0B1);
                PORTB &= ~(1 << PB1);
            } else {
                TCCR0A |= (1 << COM0B1);
            }
            break;
    }
}

bool is_trigger_active(channel_t idx) {
    switch (idx) {
        case CHANNEL_L:
            return (PINB & (1 << PB3)) != 0;
        case CHANNEL_R:
            return (PINB & (1 << PB4)) != 0;
    }
    return false;
}

bool is_ambient_enabled(void) {
    return (PINB & (1 << PB2)) == 0;
}
