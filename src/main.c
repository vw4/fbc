#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdint.h>

#include "channel.h"
#include "io.h"
#include "gamma.h"
#include "events.h"

static volatile uint8_t g_prev_state = 0;

static Channel channels[] = {
    {.state_fn = init_state},
    {.state_fn = init_state},
};

static void setup_power_reduction(void);
static void setup_isr(void);
static void sleep_if_idle(void);
static inline void emit(channel_t idx, Event event);
static void update_trigger_events(void);

ISR(INT0_vect) {
    // Note: INT0 is not a Wake-up Source (if not only level interrupt)
    sleep_disable();
    add_event(EV_AMBILIGHT_TOGGLE_ON);
}

ISR(PCINT0_vect) {
    sleep_disable();
    update_trigger_events();
}

ISR(TIM1_COMPA_vect) {
    sleep_disable();
    add_event(EV_TICK_1S);
}

int main(void) {
    cli();
    setup_power_reduction();
    setup_io();
    update_trigger_events();
    setup_isr();
    sei();

    while (1) {
        bool is_ambient = is_ambient_enabled();
        uint8_t ev = take_events();
        for (channel_t idx = CHANNEL_L; idx <= CHANNEL_R; idx++) {
            if (ev & EV_TICK_1S) {
                emit(idx, EVENT_S_TICK);
            }
            if (ev & EV_AMBILIGHT_TOGGLE_ON) {
                emit(idx, EVENT_AMBILIGHT_ON);
            }
            if (ev & EV_CLOSE_BIT(idx)) {
                emit(idx, is_ambient ? EVENT_CLOSE_WITH_AMBILIGHT_ON : EVENT_CLOSE);
            }
            if (ev & EV_OPEN_BIT(idx)) {
                emit(idx, EVENT_OPEN);
            }
            emit(idx, EVENT_NOOP);
            set_pwm_output(idx, get_gamma(channels[idx].duty));
        }
        if (channels[CHANNEL_L].is_off && channels[CHANNEL_R].is_off) {
            sleep_if_idle();
        }
        _delay_ms(3);
    }
}

static void setup_power_reduction(void) {
    // Shut down ADC and USI
    ADCSRA &= ~(1 << ADEN); // Disable the ADC before shut down
    PRR |= (1 << PRADC) | (1 << PRUSI);
    // Sleep Mode Select: Power-down
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

static void setup_isr(void) {
    // Pin Change Mask Register (PB3, PB4)
    PCMSK |= (1 << PCINT3) | (1 << PCINT4);
    // The falling edge of INT0 generates an interrupt request (PB2)
    MCUCR = ((MCUCR & ~(1 << ISC00)) | (1 << ISC01));
    // PCIE: Pin Change Interrupt Enable, INT0: External Interrupt Request 0 Enable
    GIMSK |= (1 << PCIE) | (1 << INT0);

    // Timer 1 ~ 1Hz
    OCR1C = 243; // TOP for Timer1
    OCR1A = OCR1C; // interrupt at TOP
    // Clear Output Compare Flag 1 A
    TIFR |= (1 << OCF1A);
    // Timer/Counter1 Output Compare Match A Interrupt Enable
    TIMSK |= (1 << OCIE1A);
    // Prescaler /4096
    TCCR1 |= (1 << CS13) | (1 << CS12) | (0 << CS11) | (1 << CS10);

    // Clear pending INT0 and PCIF flag !important to do this right before sei!
    GIFR = (1 << INTF0) | (1 << PCIF);
}

static void sleep_if_idle(void) {
    cli();
    if (has_pending_events()) {
        sei();
        return;
    }
    sleep_enable();
    sei();
    sleep_cpu();
    sleep_disable();
}

static inline void emit(channel_t idx, Event event) {
    channels[idx].state_fn(&channels[idx], event);
}

static void update_trigger_events(void) {
    uint8_t curr_state = 0;
    for (channel_t idx = CHANNEL_L; idx <= CHANNEL_R; idx++) {
        bool is_active = is_trigger_active(idx);
        curr_state |= (is_active << idx);
        bool is_changed = ((g_prev_state ^ curr_state) & (1u << idx));
        if (is_changed) {
            if (is_active) {
                add_event(EV_OPEN_BIT(idx));
                remove_event(EV_CLOSE_BIT(idx));
            } else {
                add_event(EV_CLOSE_BIT(idx));
                remove_event(EV_OPEN_BIT(idx));
            }
        }
    }
    g_prev_state = curr_state;
}

