#include "wifiConfig.h"

char MacAddr[18];
static bool wifiConfigured = false;

const uint8_t AP_PIN = 0;           // ใช้ IO0
const unsigned long HOLD_MS = 5000; // 5 วินาที

unsigned long pressStart = 0;
bool pressed = false;
bool apModeActive = false;

unsigned long last = 0;
float power = 0.0;
float lastPower = 0.0;
unsigned long lastChangeTime = 0;            // เวลาเปลี่ยนแปลงล่าสุด (ms)
const unsigned long timeout = 5 * 60 * 1000; // 5 นาที (300,000 ms)

void restart()
{
    // if (!wifiConfigured && (inv.wifi_config || inv.ip_config))
    // {
    //     wifiConfigured = true;
    //     setupWiFiMode();
    // }
}

void APmode()
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
        pressStart = 0;
    }

    if (pressed && !apModeActive)
    {
        if (millis() - pressStart >= HOLD_MS)
        {
            Serial.println("Long press on IO0 → Entering AP Mode");
            wifimode = false; // AP Mode
            saveApSetting();
            Serial.println("AP mode setting saved.");
            vTaskDelay(pdMS_TO_TICKS(2000));
            ESP.restart();
        }
    }
}

void wifi_Setup()
{
    readNetwork();
    delay(500);
    if (wifimode == 1)
    {
        // Station mode: ต้อง config IP ก่อน connect
        setupIPConfig();
        setupWiFiMode();
    }
    else
    {
        // AP mode: ทำตามปกติ
        setupWiFiMode();
    }
    lastChangeTime = millis();
}

void mac_config()
{
    // อ่าน MAC Address ของ ESP32 (byte[6])
    uint8_t mac[6];
    WiFi.macAddress(mac);

    // กำหนด Unique ID ให้ HADevice (ใช้ MAC โดยตรง)
    device.setUniqueId(mac, sizeof(mac));

    snprintf(MacAddr, sizeof(MacAddr),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2],
             mac[3], mac[4], mac[5]);

    Serial.println("========= MAC CONFIG =========");
    Serial.print("ESP32 Raw MAC Bytes: ");
    for (int i = 0; i < 6; i++)
    {
        Serial.printf("%02X", mac[i]);
        if (i < 5)
            Serial.print(":");
    }
    Serial.println();

    Serial.print("ESP32 MAC String   : ");
    Serial.println(MacAddr);
    Serial.println("UniqueId assigned to HADevice (using raw bytes)");
    Serial.println("================================");
}

void readNetwork()
{
    if (!LittleFS.exists("/networkconfig.json"))
    {
        Serial.println("networkconfig.json not found\"}");
        return;
    }

    File file = LittleFS.open("/networkconfig.json", "r");
    if (!file)
    {
        Serial.println("Failed to open file\"}");
        return;
    }

    JsonDocument doc; // ขนาดปรับตามไฟล์จริง
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error)
    {
        Serial.printf("Failed to parse JSON: %s\n", error.c_str());
        return;
    }

    // คัดลอก string จาก JSON ลง buffer
    wifimode = atoi(doc["wifi_mode"] | "0"); // 0 = AP, 1 = STA
    ipconfig = atoi(doc["ip_config"] | "0"); // 0 = DHCP, 1 = Static

    strncpy(WIFI_NAME, doc["wifi_name"] | "", sizeof(WIFI_NAME));
    strncpy(PASSWORD, doc["wifi_password"] | "", sizeof(PASSWORD));

    strncpy(IP_ADDR, doc["ip_address"] | "", sizeof(IP_ADDR));
    strncpy(SUBNET_MASK, doc["subnet_mask"] | "", sizeof(SUBNET_MASK));
    strncpy(DEFAULT_GATEWAY, doc["default_gateway"] | "", sizeof(DEFAULT_GATEWAY));

    strncpy(MQTT_USERNAME, doc["mqtt_user"] | "", sizeof(MQTT_USERNAME));
    strncpy(MQTT_PASSWORD, doc["mqtt_password"] | "", sizeof(MQTT_PASSWORD));
    strncpy(MQTT_ADDR, doc["mqtt_server"] | "", sizeof(MQTT_ADDR));
    strncpy(MQTT_PORT, doc["mqtt_port"] | "", sizeof(MQTT_PORT));

    Serial.printf("WIFI MODE: %s\n", (wifimode == 0 ? "AP" : "STA"));
    Serial.printf("IP CONFIG: %s\n", (ipconfig == 0 ? "DHCP" : "STATIC"));

    Serial.printf("WIFI_NAME: %s\n", WIFI_NAME);
    Serial.printf("PASSWORD: %s\n", PASSWORD);

    if (ipconfig)
    {
        Serial.printf("IP_ADDR: %s\n", IP_ADDR);
        Serial.printf("SUBNET_MASK: %s\n", SUBNET_MASK);
        Serial.printf("DEFAULT_GATEWAY: %s\n", DEFAULT_GATEWAY);
    }
    Serial.printf("MQTT_ADDR: %s\n", MQTT_ADDR);
    Serial.printf("MQTT_USERNAME: %s\n", MQTT_USERNAME);
    Serial.printf("MQTT_PASSWORD: %s\n", MQTT_PASSWORD);
    Serial.printf("MQTT_PORT: %s\n", MQTT_PORT);
}

void setupWiFiMode()
{
    if (wifimode == 0)
    {
        Serial.println("📡 Setting WiFi to ACCESS POINT mode");

        // ตั้งค่าโหมด AP
        ledMode = LED_AP_MODE;
        WiFi.mode(WIFI_AP);

        // กำหนดค่า IP ของ AP
        IPAddress local_IP(192, 168, 4, 1);
        IPAddress gateway(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);

        if (!WiFi.softAPConfig(local_IP, gateway, subnet))
        {
            Serial.println("❌ Failed to configure AP");
        }
        if (WiFi.softAP(DEVICE_NAME, DEVICE_PASS))
        {
            Serial.println("✅ AP Started");
            Serial.print("AP IP address: ");
            Serial.println(WiFi.softAPIP());
        }
        else
        {
            Serial.println("❌ Failed to start AP");
        }
        esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        if (ap_netif)
        {
            esp_netif_dhcps_stop(ap_netif);
            esp_netif_ip_info_t ip_info;
            ip_info.ip.addr = (uint32_t)local_IP;
            ip_info.gw.addr = (uint32_t)gateway;
            ip_info.netmask.addr = (uint32_t)subnet;
            esp_netif_set_ip_info(ap_netif, &ip_info);
            if (esp_netif_dhcps_start(ap_netif) == ESP_OK)
            {
                Serial.println("✅ DHCP server started (esp_netif)");
            }
            else
            {
                Serial.println("❌ Failed to start DHCP server");
            }
        }
        else
        {
            Serial.println("❌ Failed to get AP netif handle");
        }
    }

    if (wifimode == 1)
    {
        Serial.println("📡 Setting WiFi to STATION mode");
        ledMode = LED_BUSY;
        WiFi.mode(WIFI_STA);
        WiFi.setSleep(false); // ⭐ สำคัญ
        WiFi.begin(WIFI_NAME, PASSWORD);
    }

    delay(1000);
}

void setupIPConfig()
{
    IPAddress local_IP = parseIP(IP_ADDR);
    IPAddress subnet = parseIP(SUBNET_MASK);
    IPAddress gateway = parseIP(DEFAULT_GATEWAY);

    if (ipconfig == 1)
    {
        Serial.println("🌐 Using STATIC IP");
        if (!WiFi.config(local_IP, gateway, subnet))
        {
            Serial.println("❌ Failed to configure Static IP");
        }
    }
    else
    {
        Serial.println("🌐 Using DHCP (Obtain IP Automatically)");
    }
    delay(500);
    // Serial.println("Debug ipconfig" + String(ipconfig));
}

IPAddress parseIP(const char *ipStr)
{
    int o1, o2, o3, o4;
    if (sscanf(ipStr, "%d.%d.%d.%d", &o1, &o2, &o3, &o4) == 4)
    {
        return IPAddress(o1, o2, o3, o4);
    }
    return IPAddress(0, 0, 0, 0); // ถ้า error
}

void showAPClients()
{
    if (wifimode == 0)
    {
        wifi_sta_list_t wifi_sta_list;
        esp_netif_sta_list_t netif_sta_list;

        if (esp_wifi_ap_get_sta_list(&wifi_sta_list) == ESP_OK)
        {
            if (esp_netif_get_sta_list(&wifi_sta_list, &netif_sta_list) == ESP_OK)
            {
                Serial.printf("👥 Connected Clients: %d\n", netif_sta_list.num);

                for (int i = 0; i < netif_sta_list.num; i++)
                {
                    esp_netif_sta_info_t station = netif_sta_list.sta[i];
                    Serial.printf(
                        " - Client %d: MAC %02X:%02X:%02X:%02X:%02X:%02X, IP %s\n",
                        i + 1,
                        station.mac[0], station.mac[1], station.mac[2],
                        station.mac[3], station.mac[4], station.mac[5],
                        ip4addr_ntoa((ip4_addr_t *)&station.ip.addr) // แปลง IP เป็น string
                    );
                }
            }
            else
            {
                Serial.println("❌ Failed to get netif station list");
            }
        }
        else
        {
            Serial.println("❌ Failed to get AP sta list");
        }
    }
}

void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_START:
        Serial.println("📡 WiFi STA started");
        break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.print("✅ WiFi connected, IP: ");
        Serial.println(WiFi.localIP());

        ledMode = LED_CONNECTED;

        // ===== NTP init =====
        Serial.println("⏱ Init NTP");

        configTime(
            7 * 3600,
            0,
            "time.google.com",
            "time1.google.com",
            "time2.google.com");

        ntpSynced = false;

        if (ntpTaskHandle == NULL)
        {
            xTaskCreatePinnedToCore(
                TaskNTP,
                "TaskNTP",
                4096,
                NULL,
                1,
                &ntpTaskHandle,
                1);
        }

        // ===== MQTT / Service start =====
        // mqtt.connect(); (ถ้ามี)

        break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.println("⚠️ WiFi disconnected");

        ledMode = LED_DISCONNECTED;

        // ป้องกัน reconnect ถี่
        static unsigned long lastRetry = 0;
        if (millis() - lastRetry > 5*60*1000)
        {
            lastRetry = millis();
            Serial.println("🔄 Reconnecting WiFi...");
            if (WiFi.getMode() == WIFI_STA)
            {
                ESP.restart();
            }
        }
        break;

    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
        Serial.println("⚠️ WiFi lost IP");
        break;

    case ARDUINO_EVENT_WIFI_STA_STOP:
        Serial.println("🛑 WiFi stopped");
        break;

    default:
        break;
    }
}
