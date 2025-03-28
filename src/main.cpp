// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-01
// -------------------------------------------------------
// main.cpp
// *******************************************************
#include <M5Unified.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "fileServer/fileServer.h"

#if defined(ENABLE_SD_UPDATER)
#include "SDUpdater.h"
#endif

void setup();
void loop();
bool setupFileServer();
void prt(String message);
void error_stop();

const String PROG_NAME = "m5stack-fileServer";
const String VERSION = "v1.04a-250327";
//----------------------------------------------------------
// *** SETTINGS ***
const String SETTING_FILE = "/wifi.txt";
// Write the settings in the above file(SD or SPIFFS).
// If those are no present, change the settings in the 3-lines below.
const String YOUR_SSID = "YOUR_SSID";
const String YOUR_SSID_PASS = "YOUR_SSID_PASSWORD";
const String YOUR_SERVER_NAME = "m5fileServer";
//----------------------------------------------------------
const bool SD_USE = true;     // 'false' if not use SD
const bool SPIFFS_USE = true; // 'false' if not use SPIFFS
const bool DISP_ON = true;    // 'false' if not print message on display
//----------------------------------------------------------

void setup()
{
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  M5.begin(cfg);

#if defined(ENABLE_SD_UPDATER)
  SDU_lobby(PROG_NAME);
#endif

  M5.Display.setBrightness(120);
  M5.Lcd.setTextSize(2);
  Serial.println(__FILE__);
  prt("-   " + PROG_NAME + "   -\n");

  if (!setupFileServer())
    error_stop();

  prt("SUCCESS: System started");
  prt("\nIP Addr: " + IP_ADDR);
  prt("\nServerName: " + SERVER_NAME);
}

void loop()
{
  delay(1);
}

bool setupFileServer()
{
  // --- SD and SPIFFS start ------
  SD_ENABLE = false;
  if (SD_USE)
  {
    SD_ENABLE = SD_Start();
    if (SD_ENABLE)
      prt("SD      .....  OK");
    else
      prt("SD      .....  NG");
  }

  SPIFFS_ENABLE = false;
  if (SPIFFS_USE)
  {
    SPIFFS_ENABLE = SPIFFS_Start();
    if (SPIFFS_ENABLE)
      prt("SPIFFS  .....  OK");
    else
      prt("SPIFFS  .....  NG");
  }

  if (!SPIFFS_ENABLE && !SD_ENABLE)
  {
    prt("Both SD and SPIFFS are not available");
    return false;
  }

  // ------- Network Settings Read ---------
  SSID = "";
  SSID_PASS = "";
  SERVER_NAME = "";

  if (SD_ENABLE && SD_SettingRd(SETTING_FILE))
    prt(" Settings read from SD");
  else if (SPIFFS_ENABLE && SPIFFS_SettingRd(SETTING_FILE))
    prt(" Settings read from SPIFFS");

  if (SSID == "")
    SSID = YOUR_SSID;
  prt(" SSID: " + SSID);

  if (SSID_PASS == "")
    SSID_PASS = YOUR_SSID_PASS;

  if (SERVER_NAME == "")
    SERVER_NAME = YOUR_SERVER_NAME;

  // --- wifi and Server Start --------------
  if (!wifiStart())
  {  
    prt("WiFi    .....  NG");
    return false;
  }
  prt("WiFi    .....  OK");

  if (!mdnsStart())
  {  
    prt("mDNS    .....  NG");
    return false;
  }
  prt("mDNS    .....  OK");

  if (!fileServerStart())
  {
    prt("fileServer ..  NG");
    return false;
  }
  prt("fileServer ..  OK");

  return true;
}

void prt(String message)
{
  Serial.println(message);

  if (DISP_ON)
    M5.Display.println(message);
}

void error_stop()
{
  prt("\nERR: fail to start server");
  delay(10000);

  while (true)
    ;
}
