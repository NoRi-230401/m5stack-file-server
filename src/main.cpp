// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-15
// -------------------------------------------------------
// main.cpp
// *******************************************************
#include "fileServer/fileServer.h"

//-------------------------------------------
const String PROG_NAME = "m5fileServer";
const String VERSION = "v1.09";
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
// const String YOUR_HOST_NAME = "m5fileServer"; //change if you need
const String YOUR_HOST_NAME = "stackchan";
//---------------------------------------------------------------------------

void setup()
{
  m5stack_begin();
  SDU_lobby();

  DISP_start();
  SD_start();
  SPIFFS_start();
  if (!SPIFFS_ENABLE && !SD_ENABLE)
  {
    prt("Both SD and SPIFFS are not available");
    STOP();
  }

  if (!setupServer())
    STOP();
}

void loop()
{
  requestManage();
  delay(1);
}
