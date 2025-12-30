#ifndef FBC_IO_H
#define FBC_IO_H

#include <stdint.h>
#include <stdbool.h>

#define CHANNELS_CNT 2
typedef enum { CHANNEL_L = 0, CHANNEL_R = 1 } channel_t;

void setup_io(void);
void set_pwm_output(channel_t idx, uint8_t duty);
bool is_trigger_active(channel_t idx);
bool is_ambient_enabled(void);

#endif //FBC_IO_H
