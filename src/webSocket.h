#ifndef WEB_SOCKET_H
#define WEB_SOCKET_H
#include "globals.h"

extern String wsSerial;

String wsAllDataBase64();
void wsClear();
void notifyClients(const String &msg);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void initWebSocket();
void wsPrint(const String &msg);
void serialSent();
void wsloop();

#endif