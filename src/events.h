#ifndef FBC_EVENT_H
#define FBC_EVENT_H

#include <stdint.h>
#include "io.h"

#define EV_OPEN_BIT(idx)  (1u << (idx))
#define EV_CLOSE_BIT(idx) (1u << ((idx) + CHANNELS_CNT))

typedef enum {
    EV_TRIGGER_L_OPEN = EV_OPEN_BIT(CHANNEL_L),
    EV_TRIGGER_R_OPEN = EV_OPEN_BIT(CHANNEL_R),
    EV_TRIGGER_L_CLOSE = EV_CLOSE_BIT(CHANNEL_L),
    EV_TRIGGER_R_CLOSE = EV_CLOSE_BIT(CHANNEL_R),
    EV_TICK_1S = (1u << (CHANNELS_CNT * 2)),
    EV_AMBILIGHT_TOGGLE_ON = (1u << ((CHANNELS_CNT * 2) + 1)),
} event_mask_t;

void add_event(event_mask_t event_mask);
void remove_event(event_mask_t event_mask);
uint8_t take_events(void);
bool has_pending_events(void);

#endif //FBC_EVENT_H