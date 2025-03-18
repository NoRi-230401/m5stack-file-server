#if defined(ENABLE_SD_UPDATER)

#include <Arduino.h>
#include <M5Unified.h>
#include <ESP32-targz.h>
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

#endif