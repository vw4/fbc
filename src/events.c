#include <avr/interrupt.h>
#include "events.h"

volatile uint8_t g_events = 0;

void add_event(event_mask_t event_mask) {
    g_events |= event_mask;
}

void remove_event(event_mask_t event_mask) {
    g_events &= ~event_mask;
}

uint8_t take_events(void) {
    uint8_t events = 0;
    cli();
    events = g_events;
    g_events = 0;
    sei();
    return events;
}

bool has_pending_events(void) {
    return g_events != 0;
}
