#ifndef IOT_HA_H
#define IOT_HA_H
#include "globals.h"

extern HASwitch sw;
extern HASensorNumber temp;

void iotHAsetup();
void HA_Diagnostic();
void iotHArun();

#endif