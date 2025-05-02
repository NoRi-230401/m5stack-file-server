#if defined(ENABLE_SD_UPDATER)

#include <Arduino.h>
#include <M5Unified.h>
#include <M5StackUpdater.h>
#include "SDUpdater.h"


#define SDU_SKIP_TMR 5000 // skip timer : ms

void SDU_lobby(String PROG_NAME)
{
  SDUCfg.setAppName(PROG_NAME.c_str()); // lobby screen label: application name
  SDUCfg.setLabelMenu("< Menu");        // BtnA label: load menu.bin

  checkSDUpdater(
      SD,           // filesystem (default=SD)
      MENU_BIN,     // path to binary (default=/menu.bin, empty string=rollback only)
      SDU_SKIP_TMR, // wait delay, (default=0, will be forced to 2000 upon ESP.restart() )
      TFCARD_CS_PIN // usually default=4 but your mileage may vary
  );

  Serial.println("SDU_lobby done");
}

#if defined(CARDPUTER)
#include <M5Cardputer.h>
extern SPIClass SPI2;
void SDU_lobby_cardputer()
{
  while (false == SD.begin(M5.getPin(m5::pin_name_t::sd_spi_ss), SPI2))
  {
      delay(500);
  }
  M5Cardputer.update();
  
  if (M5Cardputer.Keyboard.isKeyPressed('a'))
  {
      updateFromFS(SD, "/menu.bin");
      ESP.restart();
      
  }

  // delay(5000);
  Serial.println("\nSDU_lobby_cardputer done");
}
#endif   // end of CARDPUTER

#endif   // end of ENABLE_SD_UPDATER