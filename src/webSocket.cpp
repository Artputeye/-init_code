#include "websocket.h"

AsyncWebSocket ws("/ws");
unsigned long lastTimeMonitor = 0;
unsigned long lastPingTime = 0;
unsigned long MonitorDelay = 3000;
const unsigned long pingInterval = 25000; // 25 วินาที

String wsSerial;
int len;

String wsAllDataBase64()
{
    JsonDocument doc;

    ///////////////////////ws data Sent////////////////////////////
        ///////////////////////Serial Sent////////////////////////////
    doc["Serial"] = wsSerial;

    doc["data1"] = "data1";
    doc["data2"] = "data2";

    /////////////////////////////////////////////////////////////////////////
    String jsonOut; // serialize JSON
    serializeJson(doc, jsonOut);
    return base64::encode(jsonOut); // encode Base64
}

void wsClear()
{
    JsonDocument doc;
    ///////////////////////Monitor////////////////////////////////
    doc["data1"] = "";
    doc["data2"] = "";

    String jsonOut; // serialize JSON
    serializeJson(doc, jsonOut);
}

void notifyClients(const String &base64Msg)
{
    ws.textAll(base64Msg); // ส่ง Text Frame
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len &&
        (info->opcode == WS_TEXT || info->opcode == WS_BINARY))
    {
        notifyClients(wsAllDataBase64());
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n",
                      client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    }
}

void initWebSocket()
{
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}

void wsPrint(const String &msg)
{
    wsSerial = msg;
    if (!wsSerial.isEmpty())
    {
        notifyClients(wsAllDataBase64());
        wsSerial = "";
        delay(100);
    }
}

void serialSent()
{
  while (Serial.available() > 0)
  {
    serialData = Serial.readStringUntil('\n');
    Serial.print("Serial Sent : ");
    Serial.println(serialData);
    wsPrint(serialData);
    len = serialData.length();
    Serial.println("len Serial: " + String(len));
    delay(10);
  }
}

/////////////////////////////////////////////////////////////////////////////////////
void wsloop()
{
    serialSent();
    if (millis() - lastTimeMonitor > MonitorDelay)
    {
        lastTimeMonitor = millis();
        notifyClients(wsAllDataBase64());
        // wsClear();
    }

    if (millis() - lastPingTime > pingInterval)
    {
        lastPingTime = millis();
        ws.pingAll();
    }
    ws.cleanupClients();
}
