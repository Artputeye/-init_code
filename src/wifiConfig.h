#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H
#include "globals.h"

extern const uint8_t AP_PIN;
extern char MacAddr[18];

void restart();
void APmode();
void wifi_Setup();
void mac_config();
void readNetwork();
void setupWiFiMode();
void setupIPConfig();
IPAddress parseIP(const char *ipStr);
void showAPClients();
void WiFiEvent(WiFiEvent_t event);

#endif