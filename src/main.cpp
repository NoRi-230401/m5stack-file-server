// ---------------------------------------------------------
// * main.cpp *      by NoRi 2025-01-23
// *******************************************************
#include <M5Unified.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#if defined(ENABLE_SD_UPDATER)
#include "SDUpdater.h"
#endif

void setup();
void loop();
void prt(String msg);
void error_stop();

extern bool SD_Start();
extern bool SPIFFS_Start();
extern bool SD_SettingRd(const String filename);
extern bool FS_SettingRd(const String filename);
extern bool wifiStart();
extern bool mdnsStart(void);
extern bool fileServerStart();

extern const String VERSION;
extern const String PROG_NAME;
const String VERSION = "v1.03a-250326";
const String PROG_NAME = "m5stack-fileServer";
String SSID, SSID_PASS, SERVER_NAME, IP_ADDR;

bool SD_ENABLE;
bool SPIFFS_ENABLE;
const bool SD_USE = true;      // 'false' if not use SD
const bool SPIFFS_USE = true;  // 'false' if not use SPIFFS

#define NETWORK_SETTING_FILE "/wifi.txt"
// you should change your own settings below 3-lines
//     or  settings in NETWORK_SETTING_FILE
#define YOUR_SSID        "NAME_OF_YOUR_SSID"
#define YOUR_SSID_PASS   "PASSWORD_OF_YOUR_SSID"
#define YOUR_SERVER_NAME "SERVER_NAME_OF_YOUR_DEVICE"

void setup()
{
  // ********** M5stack start ***********
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  // -- M5Unified 0.1.17からデフォルトが0になったため設定
  M5.begin(cfg);

  #if defined(ENABLE_SD_UPDATER)
  SDU_lobby(PROG_NAME);
  #endif

  Serial.println(__FILE__);
  M5.Display.setBrightness(120);
  M5.Lcd.setTextSize(2);
  prt("-   " + PROG_NAME + "   -\n");
  // *************************************

  // --- SD and SPIFFS start ------
  SD_ENABLE = false;
  SD_ENABLE = SD_Start();
  if(SD_ENABLE) prt("SD      .....  OK");
  else  prt("SD      .....  NG");

  SPIFFS_ENABLE = false;
  SPIFFS_ENABLE = SPIFFS_Start();
  if (SPIFFS_ENABLE) prt("SPIFFS  .....  OK");
  else  prt("SPIFFS  .....  NG");

  if(SD_ENABLE && SD_USE) SD_ENABLE = true;
  else SD_ENABLE=false;
  
  if(SPIFFS_ENABLE && SPIFFS_USE) SPIFFS_ENABLE = true;
  else SPIFFS_ENABLE=false;

  if (!SPIFFS_ENABLE && !SD_ENABLE)
  {
    prt("SD and SPIFFS are not available");
    error_stop();
  } 

  // ------- Network Settings Read ---------
  if(SD_ENABLE && SD_SettingRd(NETWORK_SETTING_FILE))
    prt(" <- Settings from SD");
  else if(SPIFFS_ENABLE && FS_SettingRd(NETWORK_SETTING_FILE))
    prt(" <- Settings from SPIFFS");
  else
  {
    SSID = YOUR_SSID;
    SSID_PASS = YOUR_SSID_PASS;
    SERVER_NAME = YOUR_SERVER_NAME;
    prt(" <- Settings in PROG_CODE");
  }
  
  // --- wifi and Server Start --------------
  if (!wifiStart())       error_stop();
  prt("\nWiFi    .....  OK");

  if (!mdnsStart())       error_stop();
  prt("mDNS    .....  OK");

  if (!fileServerStart()) error_stop();
  prt("fileServer ..  OK");

  prt("SUCCESS: System started");
  prt("\nIP Addr: " + IP_ADDR);
  prt("\nServerName: " + SERVER_NAME);
}

void loop()
{
  // Nothing to do here yet
  delay(1);
}

void prt(String msg)
{
  M5.Display.println(msg);
  Serial.println(msg);
}

void error_stop()
{
  prt("\nERR: fail to start server");
  delay(10000);

  while (true)
    ;
}
