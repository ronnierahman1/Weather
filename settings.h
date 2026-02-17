#pragma once

#include <Arduino.h>

void initSettings();
int getTimezoneHours();
long getGmtOffsetSec();
void saveTimezoneHours(int tzHours);

// The shared GMT offset variable (seconds east of UTC)
extern long GMT_OFFSET_SEC;
