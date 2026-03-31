#include "config.h"

// --- Global Objects ---

AsyncWebServer server(80);
File fsUploadFile;

// --- Device Info ---
char D_SoftwareVersion[15] = "1.2.8";
char D_Mfac[15]            = "ARTTECH";
char D_Model[15]           = "IoT Solar";

// --- Network Settings ---
char DEVICE_NAME[28]       = "Arttech_Device";
char DEVICE_PASS[28]       = "12345678";
char WIFI_SSID[30]         = "";
char WIFI_PASS[25]         = "";
char HOSTNAME[30]          = "arttech-iot";

// --- Static IP Settings ---
char IP_ADDR[16]           = "0.0.0.0";
char SUBNET_MASK[16]       = "255.255.255.0";
char GATEWAY[16]           = "0.0.0.0";

// --- MQTT Settings ---
char MQTT_ADDR[16]         = "192.168.1.100";
char MQTT_USER[28]         = "admin";
char MQTT_PASS[28]         = "admin1234";
char MQTT_PORT[6]          = "1883";

// --- Auth & Status ---
char AUTH_USER[10]         = "admin";
char AUTH_PASS[10]         = "12345678";
uint8_t Mac[6];
String MacAddr             = "";

bool isWifiApMode          = true;
bool isIpConfigStatic      = false;
String serialData          = "";