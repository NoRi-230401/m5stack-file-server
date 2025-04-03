// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-01
// -------------------------------------------------------
// main.cpp
// *******************************************************
#include "fileServer/fileServer.h"

#if defined(ENABLE_SD_UPDATER)
#include "SDUpdater.h"
#endif

bool setupServer();
const String PROG_NAME = "m5stack-fileServer";
const String VERSION = "v1.05a-250402b";
//---------------------------------------------------------------------------
// **  SETTINGS  **
//---------------------------------------------------------------------------
const bool SD_USE = true;     // 'false' if not use SD
const bool SPIFFS_USE = true; // 'false' if not use SPIFFS
const bool DISP_ON = true;    // 'false' if not print message on the display
//---------------------------------------------------------------------------
const String NETWORK_SETTING_FILE = "/wifi.txt";
// Write the network settings in the above file(SD or SPIFFS).
// If those are no present, use in the 3-lines below.
const String YOUR_SSID = "YOUR_SSID";
const String YOUR_SSID_PASS = "YOUR_SSID_PASSWORD";
const String YOUR_SERVER_NAME = "m5fileServer";
//---------------------------------------------------------------------------
// NTP connection information.
#define NTP_JST 9 * 3600L              // JST=GMT+9h
#define NTP_GMT_OFFSET NTP_JST         // GMT_OFFSET
#define NTP_DAYLIGHT_OFFSET 0          // daylight_offset
#define NTP_SVR1 "ntp.nict.jp"         // NTP server
#define NTP_SVR2 "ntp.jst.mfeed.ad.jp" // NTP server

void setup()
{
  getHeapInf();

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

  if (!setupServer())
    error_stop();

  if (timeSyncNTP(NTP_GMT_OFFSET, NTP_DAYLIGHT_OFFSET, NTP_SVR1, NTP_SVR2))
  {
    Serial.println("Success : NTP time sync");
    Serial.println("NTP: " + getTmNTP());
    setRTC();
  }
  else
  {
    prt("ERROR: NTP time sync");
  }
  Serial.println("RTC: " + getTmRTC());

  prt("SUCCESS: System started");
  prt("\nIP Addr: " + IP_ADDR);
  prt("\nServerName: " + SERVER_NAME);

  // ---- Heap Information -----
  prtHeapInf("-- SetupStart HeapInf --");
  getHeapInf();
  prtHeapInf("-- SetupDone  HeapInf --");
}

void loop()
{
  delay(1);
}

bool setupServer()
{
  // --- SD and SPIFFS start ---
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

  if (SD_ENABLE && SD_SettingRd(NETWORK_SETTING_FILE))
    prt(" Settings read from SD");
  else if (SPIFFS_ENABLE && SPIFFS_SettingRd(NETWORK_SETTING_FILE))
    prt(" Settings read from SPIFFS");

  if (SSID == "")
    SSID = YOUR_SSID;
  prt(" SSID: " + SSID);

  if (SSID_PASS == "")
    SSID_PASS = YOUR_SSID_PASS;

  if (SERVER_NAME == "")
    SERVER_NAME = YOUR_SERVER_NAME;

  if (SSID == "" || SSID_PASS == "" || SERVER_NAME == "")
  {
    prt("SETTINGS.....  NG");
    return false;
  }

  // --- wifi and Server Start -------
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
