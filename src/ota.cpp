#include "ota.h"

File fsUploadFile;

void setupOTAUpload()
{
  firmwareUpload();
  littleUpload();
  dirLittle();
  deleteFile();
}

void firmwareUpload()
{
  server.on("/otafirmware", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      bool success = Update.end(true);
      Serial.printf("[OTA] Update %s\n", success ? "SUCCESS" : "FAILED");

      if (!success) {
        Serial.printf("[OTA] Error: %s\n", Update.errorString());
        request->send(500, "text/plain", "Update failed: " + String(Update.errorString()));
      } else {
        request->send(200, "text/plain", "OK");
        delay(1000);
        ESP.restart();
      }
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      static int updateCmd = U_FLASH;

      if (index == 0) {
        String updateType = "otafirmware"; 
        if (request->hasParam("type", true)) {
          updateType = request->getParam("type", true)->value();
        }
        ledMode = LED_BUSY;
        Serial.printf("[OTA] Begin update: %s, filename: %s\n", updateType.c_str(), filename.c_str());

        if (updateType != "otafirmware") {
          Serial.println("Invalid update type provided! Aborting OTA.");
          request->send(400, "text/plain", "Invalid update type");
          Update.abort();
          return;
        }

        if (!Update.begin(UPDATE_SIZE_UNKNOWN, updateCmd)) {
          Serial.printf("[OTA] Error begin: %s\n", Update.errorString());
          return;
        }
      }
      if (len) {
        if (Update.write(data, len) != len) {
          Serial.printf("[OTA] Write error: %s\n", Update.errorString());
        } else {
          Serial.printf("[OTA] Written %u bytes\n", len);
        }
      }
      if (final) {
        Serial.println("[OTA] Finalizing update...");
      }
    }
  );
}

void littleUpload()
{
  server.on("/otalittlefs", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "OK");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {

      if (!filename.startsWith("/")) filename = "/" + filename;

      String path = filename;
      int lastSlash = path.lastIndexOf('/');
      if (lastSlash > 0) {
        String dirPath = path.substring(0, lastSlash);
        if (!FILESYSTEM.exists(dirPath)) {
          Serial.printf("Creating directory: %s\n", dirPath.c_str());
          FILESYSTEM.mkdir(dirPath);
        }
      }

      if (index == 0) {
        Serial.printf("Upload start: %s\n", filename.c_str());
        fsUploadFile = FILESYSTEM.open(filename, "w");
      }

      if (fsUploadFile) {
        fsUploadFile.write(data, len);
      }

      if (final) {
        if (fsUploadFile) {
          fsUploadFile.close();
        }
        Serial.printf("Upload complete: %s, size: %u\n", filename.c_str(), index + len);
      }
    }
  );
}

void dirLittle()
{
  server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("dir")) {
      request->send(500, "text/plain", "BAD ARGS");
      return;
    }

    String path = request->getParam("dir")->value();
    Serial.println("handleFileList: " + path);

    File root = FILESYSTEM.open(path);
    String output = "[";

    if (root && root.isDirectory()) {
      File file = root.openNextFile();
      while (file) {
        if (output != "[") {
          output += ',';
        }
        output += "{\"type\":\"";
        output += (file.isDirectory()) ? "dir" : "file";
        output += "\",\"name\":\"";
        output += String(file.path()).substring(1); 
        output += "\"}";
        file = root.openNextFile();
      }
    }
    output += "]";
    request->send(200, "application/json", output);
  });
}

void deleteFile()
{
  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("file")) {
      request->send(500, "text/plain", "BAD ARGS");
      return;
    }

    String path = request->getParam("file")->value();
    Serial.println("handleFileDelete: " + path);

    if (path == "/") {
      request->send(500, "text/plain", "BAD PATH");
      return;
    }

    if (!FILESYSTEM.exists(path)) {
      request->send(404, "text/plain", "FileNotFound");
      return;
    }

    FILESYSTEM.remove(path);
    request->send(200, "text/plain", "File Deleted");
  });
}
