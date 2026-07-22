#pragma once
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float flow;        // L/min
    float pressure;    // kPa (o unidad que uses)
    bool  net_up;      // conectividad IP por cualquier interfaz
    unsigned long uptime_ms;
    char hostname[32];
} device_state_t;

void state_init(const char *hostname);
void state_update_sensors(float flow, float pressure);
void state_on_net_available(bool up);
void state_build_json(char *out, size_t n);
const device_state_t* state_get(void);

#ifdef __cplusplus
}
#endif
