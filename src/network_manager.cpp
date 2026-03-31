#include "network_manager.h"

const uint8_t AP_PIN = 0;
const unsigned long HOLD_MS = 5000;

// State variables
static unsigned long pressStart = 0;
static bool pressed = false;
static unsigned long lastChangeTime = 0;

void APmode_Check()
{
    bool isPressed = (digitalRead(AP_PIN) == LOW);

    if (isPressed && !pressed)
    {
        pressed = true;
        pressStart = millis();
    }
    else if (!isPressed && pressed)
    {
        pressed = false;
    }

    if (pressed && (millis() - pressStart >= HOLD_MS))
    {
        Serial.println(F("⚠️ Long press detected! Switching to AP Mode..."));
        isWifiApMode = 0;      // Set to AP
        saveWifiModeSetting(); // สมมติว่ามี function นี้ใน globals
        delay(1000);
        ESP.restart();
    }
}

void wifi_Setup()
{
    mac_config();
    readNetworkConfig();

    if (isWifiApMode == 1)
    { // STA Mode
        setupIPConfig();
    }

    setupWiFiMode();
    lastChangeTime = millis();
}

void mac_config()
{
    uint8_t mac[6];
    WiFi.macAddress(mac);

    // กำหนด Unique ID ให้ HADevice
    device.setUniqueId(mac, sizeof(mac));

    // แก้จาก snprintf(MacAddr, ... ) เป็น:
    char tempMac[18];
    snprintf(tempMac, sizeof(tempMac), "%02X:%02X:%02X:%02X:%02X:%02X",
             Mac[0], Mac[1], Mac[2], Mac[3], Mac[4], Mac[5]);
    MacAddr = String(tempMac); // เก็บลง String ใน globals
}

void readNetworkConfig()
{
    if (!LittleFS.exists("/networkconfig.json"))
    {
        Serial.println(F("❌ Config file not found"));
        return;
    }

    File file = LittleFS.open("/networkconfig.json", "r");
    if (!file)
        return;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        Serial.printf("❌ JSON Parse Error: %s\n", error.c_str());
        return;
    }

    // Mapping values safely
    isWifiApMode = doc["wifi_mode"] | 0;
    isIpConfigStatic = doc["ip_config"] | 0;

    strlcpy(WIFI_SSID, doc["wifi_ssid"] | "", sizeof(WIFI_SSID));
    strlcpy(WIFI_PASS, doc["wifi_pass"] | "", sizeof(WIFI_PASS));
    strlcpy(IP_ADDR, doc["ip_address"] | "", sizeof(IP_ADDR));
    strlcpy(SUBNET_MASK, doc["subnet_mask"] | "", sizeof(SUBNET_MASK));
    strlcpy(GATEWAY, doc["gateway"] | "", sizeof(GATEWAY));

    strlcpy(MQTT_ADDR, doc["mqtt_server"] | "", sizeof(MQTT_ADDR));
    strlcpy(MQTT_USER, doc["mqtt_user"] | "", sizeof(MQTT_USER));
    strlcpy(MQTT_PASS, doc["mqtt_pass"] | "", sizeof(MQTT_PASS));
    strlcpy(MQTT_PORT, doc["mqtt_port"] | "1883", sizeof(MQTT_PORT));

    Serial.println(F("--- Network Config Loaded ---"));
    Serial.printf("Mode: %s | IP: %s\n", (isWifiApMode ? "STA" : "AP"), (isIpConfigStatic ? "Static" : "DHCP"));
}

void setupWiFiMode()
{
    if (isWifiApMode == 0)
    { // AP MODE
        Serial.println(F("📡 Starting AP Mode..."));
        ledMode = LED_AP_MODE;
        WiFi.mode(WIFI_AP);

        IPAddress local_IP(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);
        WiFi.softAPConfig(local_IP, local_IP, subnet);
        WiFi.softAP(DEVICE_NAME, DEVICE_PASS);

        Serial.print(F("AP IP: "));
        Serial.println(WiFi.softAPIP());
    }
    else
    { // STA MODE
        Serial.printf("📡 Connecting to: %s\n", WIFI_SSID);
        ledMode = LED_DISCONNECTED;
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
}

void setupIPConfig()
{
    if (isIpConfigStatic == 1)
    {
        Serial.println(F("🌐 Setting Static IP..."));
        IPAddress local_IP = parseIP(IP_ADDR);
        IPAddress subnet = parseIP(SUBNET_MASK);
        IPAddress gateway = parseIP(GATEWAY);

        if (!WiFi.config(local_IP, gateway, subnet))
        {
            Serial.println(F("❌ Static IP Failed"));
        }
    }
}

IPAddress parseIP(const char *ipStr)
{
    IPAddress ip;
    if (ip.fromString(ipStr))
        return ip; // ใช้ built-in method ของ Arduino จะคลีนกว่า sscanf
    return IPAddress(0, 0, 0, 0);
}

void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.printf("✅ Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        ledMode = LED_CONNECTED;

        // NTP Sync
        configTime(7 * 3600, 0, "time.google.com", "pool.ntp.org");

        if (ntpTaskHandle == NULL)
        {
            xTaskCreatePinnedToCore(TaskNTP, "NTP", 4096, NULL, 1, &ntpTaskHandle, 1);
        }
        break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        ledMode = LED_DISCONNECTED;
        Serial.println(F("⚠️ WiFi Lost Connection"));
        // ระบบ Reconnect อัตโนมัติมักจะทำงานเองในพื้นหลังอยู่แล้ว
        // แต่ถ้าจะ Restart ให้เช็คเงื่อนไขให้ดีเพื่อป้องกัน Infinite Boot Loop
        break;

    default:
        break;
    }
}

void showAPClients()
{
    if (isWifiApMode != 0)
        return;

    wifi_sta_list_t wifi_sta_list;
    esp_netif_sta_list_t netif_sta_list;

    if (esp_wifi_ap_get_sta_list(&wifi_sta_list) == ESP_OK)
    {
        if (esp_netif_get_sta_list(&wifi_sta_list, &netif_sta_list) == ESP_OK)
        {
            Serial.printf("👥 Clients: %d\n", netif_sta_list.num);
            for (int i = 0; i < netif_sta_list.num; i++)
            {
                esp_netif_sta_info_t station = netif_sta_list.sta[i];
                Serial.printf(" - [%d] MAC: %02X:%02X:%02X... IP: %s\n",
                              i + 1, station.mac[0], station.mac[1], station.mac[2],
                              ip4addr_ntoa((ip4_addr_t *)&station.ip.addr));
            }
        }
    }
}