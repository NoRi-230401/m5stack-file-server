// ---------------------------------------------------------
// * main.cpp *      by NoRi 2025-01-23
// *******************************************************
#include <Arduino.h>
#include <M5Unified.h>

#include <AsyncTCP.h>          // https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer
#if defined(ENABLE_SD_UPDATER)
#include "SDUpdater.h"
#endif

#include "credentials.h"
#include <SPIFFS.h> // Built-in
#include <WiFi.h>   // Built-in

extern void SPIFFS_setupServerV11();
extern void SD_setupServerV20();
extern bool StartMDNSservice(const char *Name);
extern bool StartupErrors;
extern void Directory();
extern void SD_Directory();
AsyncWebServer server(80);

bool wifiStart();
bool serverStart();
void error_stop();
String IP_ADDR="";
String SERVER_NAME="";
String VERSION = "1.02a"; 

void setup()
{
  // ********** M5 config ***************
  auto cfg = M5.config();
  M5.begin(cfg);
#if defined(ENABLE_SD_UPDATER)
  SDU_lobby("m5_flServer");
#endif
  M5.Display.setBrightness(120);
  M5.Lcd.setTextSize(2);
  String msg ="Hello,m5_flServer!";
  M5.Display.println(msg);
  
  Serial.begin(115200);
  while (!Serial);
  Serial.println(__FILE__);
  Serial.println(msg);
  // *************************************

  if (!wifiStart())    error_stop();
  if (!serverStart())   error_stop();

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

bool wifiStart()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int count = 1;
  const int COUNT_MAX = 10;
  while(WiFi.waitForConnectResult() != WL_CONNECTED )
  {
    count++;
    M5.Display.printf(".");
    Serial.printf(".");
    // Serial.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(500);
    WiFi.begin(ssid, password);
    if (count >= COUNT_MAX)
    {
      Serial.printf("\nSTA: Failed!\n");
      return false;
    }
  }

  Serial.println("\nIP Address: " + WiFi.localIP().toString());
  if (WiFi.scanComplete() == -2)
    WiFi.scanNetworks(true); // Complete an initial scan for WiFi networks, otherwise = 0 on first display!

  return true;
}

bool serverStart()
{
  SERVER_NAME = String(ServerName);

  if (!StartMDNSservice(SERVER_NAME.c_str()))
  {
    Serial.println("Error starting mDNS Service...");
    ;
    StartupErrors = true;
    return false;
  }

  SPIFFS_setupServerV11();
  SD_setupServerV20();

  server.begin(); // Start the server
  if (!StartupErrors)
  {
    Serial.println("System started successfully...");
    Directory();    // Update the SPIFFS file list
    SD_Directory(); // Update the SD file list
    return true;
  }
  else
  {
    Serial.println("There were problems starting all services...");
    return false;
  }
}

void loop()
{
  // Nothing to do here yet ... add your requirements to do things!
}
