#include "state_pub.h"
#include <string.h>
#include <stdio.h>
#include "esp_timer.h"

static device_state_t g_state;

void state_init(const char *hostname) {
    memset(&g_state, 0, sizeof(g_state));
    if (hostname && *hostname) {
        strncpy(g_state.hostname, hostname, sizeof(g_state.hostname)-1);
    } else {
        strncpy(g_state.hostname, "axira", sizeof(g_state.hostname)-1);
    }
}

void state_update_sensors(float flow, float pressure) {
    g_state.flow = flow;
    g_state.pressure = pressure;
    g_state.uptime_ms = (unsigned long)(esp_timer_get_time() / 1000ULL);
}

void state_on_net_available(bool up) {
    g_state.net_up = up;
}

void state_build_json(char *out, size_t n) {
    if (!out || n == 0) return;
    g_state.uptime_ms = (unsigned long)(esp_timer_get_time() / 1000ULL);
    snprintf(out, n,
        "{\"flow\":%.3f,\"pressure\":%.3f,\"net_up\":%s,\"hostname\":\"%s\",\"uptime_ms\":%lu}",
        g_state.flow, g_state.pressure, g_state.net_up ? "true":"false", g_state.hostname, g_state.uptime_ms);
}

const device_state_t* state_get(void) { return &g_state; }
