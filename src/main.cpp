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
const String PROG_NAME = "m5fileServer";
const String VERSION = "v1.05b";
//---------------------------------------------------------------------------
// **  SETTINGS  **
//---------------------------------------------------------------------------
const bool SD_USE = true;     // 'false' if don't use SD
const bool SPIFFS_USE = true; // 'false' if don't use SPIFFS
const bool DISP_ON = true;    // 'false' if don't disp message on the display
bool RTC_ADJUST_REQ = true;   // 'false' if don't adjust RTC
//---------------------------------------------------------------------------
const String WIFI_TXT = "/wifi.txt";
// ---- write the network settings in the above file(SD or SPIFFS)  ---------
//           if those are no present, use in the 3-lines below.
const String YOUR_SSID = "YOUR_WIFI_SSID";
const String YOUR_SSID_PASS = "YOUR_WIFI_SSID_PASSWORD";
// const String YOUR_SERVER_NAME = "m5fileServer";  //change these if you need
const String YOUR_SERVER_NAME = "stackchan";
//---------------------------------------------------------------------------

// NTP connection information.
#define NTP_SVR1 "ntp.nict.jp"         // NTP server1
#define NTP_SVR2 "ntp.jst.mfeed.ad.jp" // NTP server2
#define NTP_GMT_OFFSET 9 * 3600L       // Sec  : GMT offset
#define NTP_DAYLIGHT_OFFSET 0          // Sec  : daylight offset
// RTC adjust
#define TM_RTC_ADJUST 10 * 1000L // mSec : adjust after setup()
unsigned long TM_SETUP_DONE = 0;
bool RTC_ENABLE = false;
// #define HEAP_INF

void setup()
{
#ifdef HEAP_INF
  getHeapInf();
#endif

  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  M5.begin(cfg);

#if defined(ENABLE_SD_UPDATER)
  SDU_lobby(PROG_NAME);
#else
  delay(1000); // Wait until the serial setup is complete
#endif

  M5.Display.setBrightness(120);
  M5.Lcd.setTextSize(2);
  Serial.println(__FILE__);
  prt("-   " + PROG_NAME + "   -\n");

  if (!setupServer())
    STOP();

  prt("SUCCESS: System started");
  prt("\nIP Addr: " + IP_ADDR);
  prt("\nServerName: " + SERVER_NAME);

#ifdef HEAP_INF
  // ---- Heap Information -----
  prtHeapInf("-- SetupStart HeapInf --");
  getHeapInf();
  prtHeapInf("-- SetupDone  HeapInf --");
#endif

  TM_SETUP_DONE = millis();
}

void loop()
{
  requestManage();

  if (RTC_ADJUST_REQ && RTC_ENABLE && (millis() - TM_SETUP_DONE > TM_RTC_ADJUST))
  {
    adjustRTC();
    RTC_ADJUST_REQ = false;
  }

  delay(1);
}

bool setupServer()
{
  // --- SD and SPIFFS start ---
  SD_ENABLE = false;
  if (SD_USE)
  {
    SD_ENABLE = FS_start(FS_SD);
    if (SD_ENABLE)
      prt("SD      .....  OK");
    else
      prt("SD      .....  NG");
  }

  SPIFFS_ENABLE = false;
  if (SPIFFS_USE)
  {
    SPIFFS_ENABLE = FS_start(FS_SPIFFS);
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

  if (SD_ENABLE && getSetting(FS_SD, WIFI_TXT))
    prt(" Settings read from SD");
  else if (SPIFFS_ENABLE && getSetting(FS_SPIFFS, WIFI_TXT))
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

  // NTP Server config
  configTime(NTP_GMT_OFFSET, NTP_DAYLIGHT_OFFSET, NTP_SVR1, NTP_SVR2);

  // check RTC enable
  if (RTC_ENABLE = M5.Rtc.isEnabled())
  {
    Serial.println("RTC is enable");
  }
  else
  {
    Serial.println("RTC is disable");
    RTC_ADJUST_REQ = false;
  }

  if (!fileServerStart())
  {
    prt("fileServer ..  NG");
    return false;
  }
  prt("fileServer ..  OK");

  return true;
}
