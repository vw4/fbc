#ifndef FBC_CHANNEL_H
#define FBC_CHANNEL_H

#include <stdbool.h>
#include <stdint.h>

#define ON_CUTOFF_TIMER_S 900
#define EXIT_DELAY_S 5

typedef struct Channel Channel;

typedef enum {
    EVENT_NOOP = 0,
    EVENT_OPEN = 1,
    EVENT_CLOSE = 2,
    EVENT_CLOSE_WITH_AMBILIGHT_ON = 3,
    EVENT_AMBILIGHT_ON = 4,
    EVENT_COMPLETE = 5,
    EVENT_CUTOFF = 6,
    EVENT_S_TICK = 7,
} Event;

typedef void (*StateFn)(Channel *channel, Event event);

struct Channel {
    StateFn state_fn;
    uint8_t duty;
    uint8_t exit_delay_timer_s;
    uint16_t cutoff_timer_s;
    bool is_off;
};

void init_state(Channel *channel, Event event);
void fade_in_state(Channel *channel, Event event);
void fade_out_state(Channel *channel, Event event);
void on_state(Channel *channel, Event event);
void off_state(Channel *channel, Event event);
void exit_delay_state(Channel *channel, Event event);

#endif //FBC_CHANNEL_H
