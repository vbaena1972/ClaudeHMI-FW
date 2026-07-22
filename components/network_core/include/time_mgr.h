#pragma once
#include <stdbool.h>

void time_mgr_init(void);
void time_mgr_start_sntp(void);
bool time_mgr_is_time_valid(void);