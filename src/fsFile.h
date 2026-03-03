#ifndef FS_FILE_H
#define FS_FILE_H
#include "globals.h"

void fileManage();
void listAllFilesAndFolders(const char *dirname);
// bool loadFile();
// bool saveFile();
// bool clearFile();
bool loadJsonFile(const char *filename, JsonDocument &doc);
bool saveJsonFile(const char *filename, const JsonDocument &doc);
bool loadSetting();
bool saveApSetting();

#endif