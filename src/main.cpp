// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-15
// -------------------------------------------------------
// main.cpp
// *******************************************************
#include "fileServer/fileServer.h"
#include "SDUpdater.h"
#if defined(CARDPUTER)
#include <M5Cardputer.h>
SPIClass SPI2;
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
// const String YOUR_HOST_NAME = "m5fileServer"; //change if you need
const String YOUR_HOST_NAME = "stackchan";
//---------------------------------------------------------------------------

void setup()
{
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;

#if defined(CARDPUTER)
  // ---- CARDPUTER ---------------
  M5Cardputer.begin(cfg, true);
  SPI2.begin(
      M5.getPin(m5::pin_name_t::sd_spi_sclk),
      M5.getPin(m5::pin_name_t::sd_spi_miso),
      M5.getPin(m5::pin_name_t::sd_spi_mosi),
      M5.getPin(m5::pin_name_t::sd_spi_ss));
#if defined(ENABLE_SD_UPDATER)
  SDU_lobby_cardputer();
#endif
#else
  // ---- Core2 CoreS3 -------------
  M5.begin(cfg);
#if defined(ENABLE_SD_UPDATER)
  SDU_lobby(PROG_NAME);
#endif
#endif // end of CARDPUTER

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
