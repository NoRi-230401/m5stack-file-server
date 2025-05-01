// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-15
// -------------------------------------------------------
// main.cpp
// *******************************************************
#include "fileServer/fileServer.h"

#if defined(ENABLE_SD_UPDATER)
#include "SDUpdater.h"
#endif

//-------------------------------------------
const String PROG_NAME = "m5fileServer";
const String VERSION = "v1.08";
const String GITHUB_URL = "https://github.com/NoRi-230401/m5stack-file-server";

//--------------------
// ***  SETTINGS  ***
//--------------------
const bool SD_USE = true;     // 'false' if don't use SD
const bool SPIFFS_USE = true; // 'false' if don't use SPIFFS
bool DISP_ON = true;          // 'false' if don't disp message on the display
bool RTC_ADJUST_ON = true;    // 'false' if don't adjust RTC
//---------------------------------------------------------------------------
const String WIFI_TXT = "/wifi.txt";
// -- write the network settings in the above file(SD or SPIFFS)  --
//           if those are no present, use in the 3-lines below.
const String YOUR_SSID = "your_wifi_ssid";
const String YOUR_SSID_PASS = "your_wifi_ssid_password";
// const String YOUR_SERVER_NAME = "m5fileServer"; //change if you need
const String YOUR_SERVER_NAME = "stackchan";
//---------------------------------------------------------------------------

void setup()
{
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

  if (!setupServer())
    STOP();

  // ----- setup done -----
}

void loop()
{
  requestManage();
  delay(1);
}
