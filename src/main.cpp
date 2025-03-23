// ---------------------------------------------------------
// * main.cpp *      by NoRi 2025-01-23
// *******************************************************
#include <Arduino.h>
#include <M5Unified.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#if defined(ENABLE_SD_UPDATER)
#include "SDUpdater.h"
#endif

extern const String VERSION;
extern const String PROG_NAME;
const String VERSION = "v1.02c-250323"; 
const String PROG_NAME = "m5stack-fileServer"; 

void error_stop();
void setup();
void loop();
extern bool wifiStart();
extern bool fileServerStart();

String IP_ADDR="";
String SERVER_NAME="";

void setup()
{
  // ********** M5 config ***************
  auto cfg = M5.config();
  M5.begin(cfg);
#if defined(ENABLE_SD_UPDATER)
  SDU_lobby(PROG_NAME);
#endif
  M5.Display.setBrightness(120);
  M5.Lcd.setTextSize(2);
  M5.Display.println(PROG_NAME);
  
  Serial.begin(115200);
  while (!Serial);
  Serial.println(__FILE__);
  Serial.println(PROG_NAME);
  // *************************************

  if (!wifiStart())    error_stop();
  if (!fileServerStart())   error_stop();

  M5.Display.println("\nSUCCESS: System started\n");
  IP_ADDR = WiFi.localIP().toString();
  M5.Display.println("IP Addr: " + IP_ADDR );

}

void error_stop()
{
  M5.Display.println("\nERROR: fail to start server");
  Serial.println("ERROR: fail to start server");
  delay(10000);

  while (true)
    ;
}

void loop()
{
  // Nothing to do here yet
  // ... add your requirements to do things!
}
