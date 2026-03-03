#include "fsFile.h"

void fileManage()
{
  // if (file.dir)
  // {
  //   listAllFilesAndFolders(targetDirectory);
  //   inv.dir = false;
  // }
  // if (file.format)
  // {
  //   LittleFS.format();
  //   inv.format = false;
  // }
}

void listAllFilesAndFolders(const char *dirname)
{
  // แสดงรายชื่อไฟล์ทั้งหมดใน LittleFS
  Serial.println("\nListing files in LittleFS:");
  Serial.printf("Listing directory: %s\n", dirname);

  File root = LittleFS.open(dirname);
  if (!root)
  {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      // ถ้าต้องการดูไฟล์ใน sub-directory ด้วย ให้เรียกซ้ำ:
      listAllFilesAndFolders(file.name()); // ระวัง recursive depth ถ้ามี subfolder เยอะ
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    file = root.openNextFile();
  }
}

// // ======================================================
// // 🔹 ฟังก์ชันโหลดค่าจาก LittleFS
// // ======================================================
// bool loadFile()
// {

// }

// // ======================================================
// // 🔹 ฟังก์ชันบันทึกลงใน LittleFS
// // ======================================================
// bool saveFile()
// {
  
// }

// // ======================================================
// // 🔹 ฟังก์ชันเคลียร์ค่า และรีเซ็ตไฟล์
// // ======================================================
// bool clearfsFile()
// {
  
// }

///////////////////////////////////////////////////////////////////////////////
bool loadJsonFile(const char *filename, JsonDocument &doc)
{
  Serial.printf("📂 Loading JSON from %s ...\n", filename);
  if (!LittleFS.exists(filename))
  {
    Serial.printf("⚠️ File not found: %s\n", filename);
    return false;
  }
  File file = LittleFS.open(filename, "r");
  if (!file)
  {
    Serial.printf("Failed to open %s for reading\n", filename);
    return false;
  }
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error)
  {
    Serial.printf("Failed to parse JSON (%s): %s\n",
                  filename, error.c_str());
    return false;
  }
  Serial.printf("JSON loaded successfully from %s\n", filename);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool saveJsonFile(const char *filename, const JsonDocument &doc)
{
  Serial.printf("💾 Saving JSON to %s ...\n", filename);
  if (LittleFS.exists(filename))
  {
    LittleFS.remove(filename);
  }
  File file = LittleFS.open(filename, "w");
  if (!file)
  {
    Serial.printf("Failed to open %s for writing\n", filename);
    return false;
  }
  if (serializeJson(doc, file) == 0)
  {
    Serial.printf("Failed to write JSON to %s\n", filename);
    file.close();
    return false;
  }
  file.close();
  Serial.printf("JSON saved successfully to %s\n", filename);
  return true;
}

bool loadSetting()
{
    JsonDocument doc;
    if (loadJsonFile("/networkconfig.json", doc))
    {
        Serial.println("📂 Loaded networkconfig.json");
        wifimode = atoi(doc["wifi_mode"] | "0");
    }
    else
    {
        Serial.println("❌ Failed to load networkconfig.json");
        wifimode = 0;   // default
    }

    if (loadJsonFile("/setting.json", doc))
    {
        Serial.println("📂 Loaded setting.json");
        //fsFile = atoi(doc["file"] | "0");

    }
    else
    {
        Serial.println("❌ Failed to load setting.json");
    }

    return true;
}

bool saveApSetting()
{
  JsonDocument doc;
  doc["wifi_mode"] = wifimode;
  if (saveJsonFile("/networkconfig.json", doc))
  {
    Serial.println("💾 setting.json saved successfully");
    return true;
  }
  else
  {
    Serial.println("❌ Failed to save setting.json");
    return false;
  }
}