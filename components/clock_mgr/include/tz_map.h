#pragma once

typedef struct {
    const char *iana;
    const char *posix;
} tz_map_t;

extern const tz_map_t TZ_MAP[];

void apply_timezone(const char *iana);
