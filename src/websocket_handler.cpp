#include "websocket_handler.h"

AsyncWebSocket ws("/ws");
String wsSerial;

static unsigned long lastMonitorTime = 0;
static unsigned long lastPingTime = 0;
const unsigned long MONITOR_INTERVAL = 3000;
const unsigned long PING_INTERVAL = 25000;

// ฟังก์ชันรวมข้อมูลและแปลงเป็น Base64
String getSerializedDataBase64() {
    JsonDocument doc;
    
    // ใส่ข้อมูลที่ต้องการส่งไปหน้าเว็บที่นี่
    doc["Serial"] = wsSerial;
    doc["data1"]  = "data1"; // เปลี่ยนเป็นตัวแปรจริงของคุณ
    doc["data2"]  = "data2";

    String jsonOut;
    serializeJson(doc, jsonOut);
    return base64::encode(jsonOut);
}

// ส่งข้อมูลหา Client ทั้งหมด
void notifyClients() {
    if (ws.count() > 0) { // ส่งเฉพาะเมื่อมีคนเชื่อมต่ออยู่
        ws.textAll(getSerializedDataBase64());
    }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && (info->opcode == WS_TEXT)) {
        // เมื่อได้รับข้อความจาก Client ให้ส่งข้อมูลปัจจุบันกลับไปอัปเดต
        notifyClients();
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("[WS] Client #%u connected (%s)\n", client->id(), client->remoteIP().toString().c_str());
            // ส่งข้อมูลให้ทันทีที่ต่อติด
            client->text(getSerializedDataBase64());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("[WS] Client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
            // รับการตอบกลับจาก Ping
            break;
        case WS_EVT_ERROR:
            Serial.printf("[WS] Client #%u error\n", client->id());
            break;
    }
}

void initWebSocket() {
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    Serial.println(F("[WS] WebSocket Server Initialized"));
}

// ส่งข้อความ Serial ไปยังหน้าเว็บ
void wsPrint(const String &msg) {
    wsSerial = msg;
    if (!wsSerial.isEmpty()) {
        notifyClients();
        wsSerial = ""; // Clear ข้อมูลหลังส่งเสร็จ
    }
}

// อ่านข้อมูลจาก Hardware Serial และส่งเข้า WebSocket
void processSerialInput() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim(); // ตัดช่องว่าง/ขึ้นบรรทัดใหม่
        if (input.length() > 0) {
            Serial.printf("[Serial In] %s (len: %d)\n", input.c_str(), input.length());
            wsPrint(input);
        }
    }
}

void wsLoop() {
    processSerialInput();

    unsigned long now = millis();

    // 1. ส่งข้อมูล Monitor ตามรอบเวลา
    if (now - lastMonitorTime > MONITOR_INTERVAL) {
        lastMonitorTime = now;
        notifyClients();
    }

    // 2. ส่ง Ping เพื่อรักษาการเชื่อมต่อ (Keep-alive)
    if (now - lastPingTime > PING_INTERVAL) {
        lastPingTime = now;
        ws.pingAll();
    }

    // 3. จัดการ Cleanup memory ของ client ที่หลุดไปแล้ว
    ws.cleanupClients();
}