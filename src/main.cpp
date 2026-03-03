// main.cpp
#include "globals.h"

WiFiClient client;           // home assistant
HADevice device;             // home assistant
HAMqtt mqtt(client, device); // home assistant ต้องประกาศ Object ใน main.cpp เท่านั้น ไม่งั้น mqtt broker หาไม่เจอ

void TaskMain(void *pvParameters);
void TaskSub(void *pvParameters);
void TaskLED(void *pvParameters);
void TaskNTP(void *pvParameters); // Task สำหรับ sync NTP

void setup()
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  pinMode(AP_PIN, INPUT_PULLUP);
  // === Serial Setup ===
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.print("Booting");
  for (int i = 0; i < 5; i++)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.println("Serial Setup Completed");
  delay(300);

  // === File System Setup ===
  if (!LittleFS.begin(true))
  {
    Serial.println("LittleFS Mount Failed");
    return;
  }
  Serial.println("LittleFS Mounted Successfully");
  delay(300);

  // === Device Configuration ===
  mac_config();
  WiFi.onEvent(WiFiEvent);
  wifi_Setup();
  /////////////////////////////////////////////////////

  iotHAsetup();
  Serial.println("Home Assistant Initialized");
  delay(300);

  ////////////////////////////////////////////////////////////////////////////

  // === mDNS & Web Setup ===
  MDNS.begin(HOST);
  Serial.println("mDNS Responder Started");
  delay(300);

  webHandle();
  initWebSocket();
  server.begin();
  Serial.println("Web Server Started");
  delay(300);

  // === NTP Setup ===
  NTPbegin();
  Serial.println("NTP Server Configured");
  delay(300);

  // === Watchdog Setup ===
  esp_task_wdt_init(WDT_TIMEOUT, true); // Enable WDT with system reset
  esp_task_wdt_delete(NULL);            // Remove loopTask from WDT
  // esp_task_wdt_add(NULL);            // (Don't add loopTask if it's unused)

  // === Task Creation ===
  TaskHandle_t xHandleMain = NULL;
  TaskHandle_t xHandleSub = NULL;
  TaskHandle_t xHandleLED = NULL;
  TaskHandle_t xHandleNTP = NULL;

  xTaskCreatePinnedToCore(
      TaskMain, "TaskMain", 4096, NULL, 1, &xHandleMain, 0);
  xTaskCreatePinnedToCore(
      TaskSub, "TaskSub", 3000, NULL, 1, &xHandleSub, 1);
  xTaskCreatePinnedToCore(
      TaskLED, "TaskLED", 2048, NULL, 1, &xHandleLED, 1);

  // === Add tasks to Watchdog ===
  esp_task_wdt_add(xHandleMain);
  esp_task_wdt_add(xHandleSub);

  Serial.println("Setup Complete");
}

void loop()
{
  esp_task_wdt_reset();
  delay(100);
}

/////////////////////////////////////////////////////////////////////////////////////////////
void TaskMain(void *pvParameters) // CPU Core0
{
  while (1)
  {
    unsigned long last = 0;
    unsigned long lastDiagnostic = 0;
    // Main function
    operation();
    // Home Assistant Diagnostic update every 60 seconds
    if ((millis() - lastDiagnostic) > 60 * 1000)
    {
      lastDiagnostic = millis();
      HA_Diagnostic();
    }

    if ((millis() - last) > 10000)
    {
      last = millis();
      UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
    }
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
void TaskSub(void *pvParameters) // CPU Core1
{
  unsigned long lastStack = 0;
  while (1)
  {
    fileManage();
    mqtt.loop();
    wsloop();
    restart();
    APmode();

    if ((millis() - lastStack) > 10000)
    {
      
      lastStack = millis();
      UBaseType_t stackRemaining = uxTaskGetStackHighWaterMark(NULL);
      showAPClients();
    }
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
void TaskLED(void *pvParameters)
{
  pinMode(LED, OUTPUT);

  while (1)
  {
    ledPatternSelect();
    vTaskDelay(pdMS_TO_TICKS(20)); // 50 Hz
  }
}
