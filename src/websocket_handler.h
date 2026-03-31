#ifndef WEB_SOCKET_H
#define WEB_SOCKET_H
#include "config.h"

extern AsyncWebSocket ws;
extern String wsSerial;

// WebSocket Functions
void initWebSocket();
void wsLoop();
void wsPrint(const String &msg);

// Internal Logic
void notifyClients();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
String getSerializedDataBase64();

#endif