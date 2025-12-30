#include "channel.h"

void init_state(Channel *channel, Event event) {
    switch (event) {
        case EVENT_OPEN:
            channel->state_fn = fade_in_state;
            break;
        case EVENT_CLOSE:
        case EVENT_CLOSE_WITH_AMBILIGHT_ON:
        case EVENT_NOOP:
            channel->state_fn = off_state;
            break;
        default: break;
    }
}

void fade_in_state(Channel *channel, Event event) {
    switch (event) {
        case EVENT_COMPLETE:
            channel->cutoff_timer_s = 0;
            channel->state_fn = on_state;
            break;
        case EVENT_CLOSE:
        case EVENT_CLOSE_WITH_AMBILIGHT_ON:
            channel->state_fn = fade_out_state;
            break;
        case EVENT_NOOP:
            if (channel->duty == UINT8_MAX) {
                channel->state_fn(channel, EVENT_COMPLETE);
            } else {
                channel->duty++;
            }
            break;
        default: break;
    }
}

void fade_out_state(Channel *channel, Event event) {
    switch (event) {
        case EVENT_COMPLETE:
            channel->state_fn = off_state;
            break;
        case EVENT_OPEN:
            channel->state_fn = fade_in_state;
            break;
        case EVENT_NOOP:
            if (channel->duty == 0) {
                channel->state_fn(channel, EVENT_COMPLETE);
            } else {
                channel->duty--;
            }
            break;
        default: break;
    }
}

void on_state(Channel *channel, Event event) {
    switch (event) {
        case EVENT_CLOSE:
            channel->exit_delay_timer_s = 0;
            channel->state_fn = exit_delay_state;
            break;
        case EVENT_CUTOFF:
        case EVENT_CLOSE_WITH_AMBILIGHT_ON:
            channel->state_fn = fade_out_state;
            break;
        case EVENT_S_TICK:
            if (++channel->cutoff_timer_s >= ON_CUTOFF_TIMER_S) {
                channel->state_fn(channel, EVENT_CUTOFF);
            }
            break;
        case EVENT_NOOP:
            if (channel->duty != UINT8_MAX) {
                channel->duty = UINT8_MAX;
            }
            break;
        default: break;
    }
}

void off_state(Channel *channel, Event event) {
    switch (event) {
        case EVENT_OPEN:
            channel->state_fn = fade_in_state;
            channel->is_off = false;
            break;
        case EVENT_NOOP:
            channel->is_off = true;
            if (channel->duty != 0) {
                channel->duty = 0;
            }
            break;
        default: break;
    }
}

void exit_delay_state(Channel *channel, Event event) {
    switch (event) {
        case EVENT_OPEN:
            channel->cutoff_timer_s = 0;
            channel->state_fn = on_state;
            break;
        case EVENT_COMPLETE:
        case EVENT_AMBILIGHT_ON:
            channel->state_fn = fade_out_state;
            break;
        case EVENT_S_TICK:
            if (++channel->exit_delay_timer_s >= EXIT_DELAY_S) {
                channel->state_fn(channel, EVENT_COMPLETE);
            }
            break;
        default: break;
    }
}
