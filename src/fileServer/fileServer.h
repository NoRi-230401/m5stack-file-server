// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-01
// -------------------------------------------------------
// fileServer.h
// *******************************************************
#ifndef _M5STACK_FILE_SERVER_H
#define _M5STACK_FILE_SERVER_H
// -------------------------------------------------------
#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <algorithm>
#include <vector>
#include <SPIFFS.h>
#include <SD.h>
#include <nvs.h>
#include <time.h>

typedef struct
{
  String filename;
  String ftype;
  String fsize;
} fileinfo;

//---- units ------
#define UNIT_AUTO 1
#define UNIT_BYTE 2
#define UNIT_KIRO 3
#define UNIT_MEGA 4
#define UNIT_GIGA 5
#define UNIT_TERA 6

extern bool wifiStart();
extern bool mdnsStart(void);
extern bool fileServerStart();
extern String SSID, SSID_PASS, SERVER_NAME, IP_ADDR;
extern bool SD_ENABLE, SPIFFS_ENABLE;
extern String HTML_Header();
extern String HTML_Footer();
extern String getContentType(String filenametype);
extern bool compareFileinfo(const fileinfo &a, const fileinfo &b);
extern void prt(String message);
extern void getHeapInf();
extern void prtHeapInf(String message);
extern String ConvBytesUnits(uint64_t bytes, int dp, int unit=UNIT_AUTO);
extern void adjustRTC();
extern String getTmRTC();
extern String getTmNTP();
extern String urlEncode(const String& input);
extern String urlDecode(const String& input);
// -------------------------------------------------------

#define FS_SPIFFS 1
#define FS_SD 2
extern bool getSetting(int flType, const String filename);
extern bool FS_start(int flType);
extern uint64_t getFileSize(int flType, String filename);

extern const String VERSION;
extern const String PROG_NAME;
extern const String YOUR_SSID;
extern const String YOUR_SSID_PASS;
extern const String YOUR_SERVER_NAME;
extern const bool DISP_ON;
extern bool RTC_ENABLE;
extern String SdPath;

extern void STOP();
extern void webApiSetup();
extern void requestManage();
// -------------------------------------------------------
#endif  // _M5STACK_FILE_SERVER_H
