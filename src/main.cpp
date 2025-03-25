// ---------------------------------------------------------
// * main.cpp *      by NoRi 2025-01-23
// *******************************************************
// #include <Arduino.h>
#include <M5Unified.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#if defined(ENABLE_SD_UPDATER)
#include "SDUpdater.h"
#endif

void error_stop();
void setup();
void loop();
void prt(String msg);
extern bool wifiStart();
extern bool mdnsStart(void);
extern bool SD_Start();
extern bool SPIFFS_Start();
extern bool fileServerStart();

extern const String VERSION;
extern const String PROG_NAME;
const String VERSION = "v1.03a-250324";
const String PROG_NAME = "m5stack-fileServer";
String IP_ADDR = "";
String SERVER_NAME = "stackchan";

void setup()
{
  // ********** M5stack start ***********
  auto cfg = M5.config();
  M5.begin(cfg);
#if defined(ENABLE_SD_UPDATER)
  SDU_lobby(PROG_NAME);
#endif
  M5.Display.setBrightness(120);
  M5.Lcd.setTextSize(2);
  Serial.begin(115200);
  while (!Serial)  ;
  Serial.println(__FILE__);
  prt("--  " + PROG_NAME + "  --\n\n");
  // *************************************

  if (!wifiStart())    error_stop();
  prt("WiFi    .....  OK");

  if (!mdnsStart())    error_stop();
  prt("mDNS    .....  OK");

  if (!SPIFFS_Start())    error_stop();
  prt("SPIFFS  .....  OK");

  if (!SD_Start())    error_stop();
  prt("SD      .....  OK");

  if (!fileServerStart())    error_stop();
  prt("fileServer ..  OK");

  prt("SUCCESS: System started");
  IP_ADDR = WiFi.localIP().toString();
  prt("\n\nIP Addr: " + IP_ADDR);
  prt("\nServerName: " + SERVER_NAME);
}

void loop()
{
  // Nothing to do here yet
  // ... add your requirements to do things!
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
