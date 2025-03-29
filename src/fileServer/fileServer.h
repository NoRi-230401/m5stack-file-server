// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-01
// -------------------------------------------------------
// fileServer.h
// *******************************************************
#ifndef _M5_FILE_SERVER_H
#define _M5_FILE_SERVER_H
// -------------------------------------------------------
#include <Arduino.h>

typedef struct
{
  String filename;
  String ftype;
  String fsize;
} fileinfo;

// --- Status Report File System ------
#define STREP_SPIFFS_TOTALBYTES 11
#define STREP_SPIFFS_USEDBYTES 12
#define STREP_SPIFFS_FREESPACE 13
#define STREP_SD_TOTALBYTES 21
#define STREP_SD_USEDBYTES 22
#define STREP_SD_FREESPACE 23
#define STREP_SD_CARDTYPE 24
#define STREP_SPIFFS_START STREP_SPIFFS_TOTALBYTES
#define STREP_SPIFFS_END STREP_SPIFFS_FREESPACE
#define STREP_SD_START STREP_SD_TOTALBYTES
#define STREP_SD_END STREP_SD_CARDTYPE

extern bool SD_Start();
extern bool SPIFFS_Start();
extern bool SD_SettingRd(const String filename);
extern bool SPIFFS_SettingRd(const String filename);
extern bool wifiStart();
extern bool mdnsStart(void);
extern bool fileServerStart();
extern String SSID, SSID_PASS, SERVER_NAME, IP_ADDR;
extern bool SD_ENABLE, SPIFFS_ENABLE;

extern const String VERSION;
extern const String PROG_NAME;
extern const String YOUR_SSID;
extern const String YOUR_SSID_PASS;
extern const String YOUR_SERVER_NAME;
extern const String ICON_FILE;

// -------------------------------------------------------
#endif  // _M5_FILE_SERVER_H
