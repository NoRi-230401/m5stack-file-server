// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-01
// -------------------------------------------------------
// webApi.cpp
// *******************************************************
#include "fileServer.h"

void webApiSetup();
void handle_shutdown(AsyncWebServerRequest *request);
void wsHandleShutdown(String reboot_get_str, String time_get_str);
void STOP();
void REBOOT();
void POWER_OFF();
void serverSend(AsyncWebServerRequest *request);
String HTML_Header2();
String HTML_Footer2();
String HTML_Header2Ng();
void requestManage();
void sendReq(int reqNo);
void handle_test(AsyncWebServerRequest *request);
void wsHandleTest(String okGetStr);

extern AsyncWebServer server;
extern String webpage;

#define REQ_REBOOT 98
#define REQ_SHUTDOWN 99
#define SHUTDOWN_MIN_TM 3
uint16_t SHUTDOWN_TM_SEC;
int REQUEST_NO = 0; // 0 : no request

void webApiSetup()
{
  server.on("/shutdown", HTTP_GET, [](AsyncWebServerRequest *request)
            { handle_shutdown(request);  serverSend(request); });

  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request)
            { handle_test(request);  serverSend(request); });
}

void handle_shutdown(AsyncWebServerRequest *request)
{
  webpage = "NG";
  String reboot_get_str = request->arg("reboot");
  String time_get_str = request->arg("time");
  wsHandleShutdown(reboot_get_str, time_get_str);
}

void wsHandleShutdown(String reboot_get_str, String time_get_str)
{
  uint16_t time_sec = SHUTDOWN_MIN_TM;

  if (time_get_str != "")
  {
    time_sec = time_get_str.toInt();

    if (time_sec < SHUTDOWN_MIN_TM)
      time_sec = SHUTDOWN_MIN_TM;

    if (time_sec > 60)
      time_sec = 60;
  }

  if (reboot_get_str.equalsIgnoreCase("ON"))
  {
    SHUTDOWN_TM_SEC = time_sec;
    sendReq(REQ_REBOOT);

    webpage = "reboot : after " + String(time_sec, DEC) + "sec";
    Serial.println(webpage);
    return;
  }

  // --- shutdown
  SHUTDOWN_TM_SEC = time_sec;
  sendReq(REQ_SHUTDOWN);
  webpage = "shutdown : after " + String(time_sec, DEC) + "sec";
  Serial.println(webpage);
  return;
}

void STOP()
{
  Serial.println(" *** Stop *** fatal error");
  SD.end();
  SPIFFS.end();
  delay(5000);

  for (;;)
  {
    delay(1000);
  }
}

void REBOOT()
{
  Serial.println(" *** Reboot ***");
  SD.end();
  SPIFFS.end();
  delay(5000);
  ESP.restart();

  for (;;)
  { // never
    delay(1000);
  }
}

void POWER_OFF()
{
  Serial.println(" *** POWER OFF ***");

  SD.end();
  SPIFFS.end();
  delay(5000);
  M5.Power.powerOff();

  for (;;)
  { // never
    delay(1000);
  }
}

void serverSend(AsyncWebServerRequest *request)
{
  if (webpage.equalsIgnoreCase("NG"))
  {
    webpage = HTML_Header2Ng() + webpage + HTML_Footer2();
    request->send(400, "text/html", webpage);
  }
  else if (webpage.equalsIgnoreCase("OK"))
  {
    Serial.println("send -> OK");
    request->send(200, "text/plain", String("OK"));
  }
  else
  {
    webpage = HTML_Header2() + webpage + HTML_Footer2();
    request->send(200, "text/html", webpage);
  }
}

String HTML_Header2()
{
  String page;
  page = "<!DOCTYPE html>";
  page += "<html lang = 'ja'>";
  page += "<head>";
  page += "<meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width,initial-scale=1.0'>";
  page += "<title>webApi</title>";
  page += "<base target='webApi'>";
  page += "<style>";
  page += "html {font-size: 62.5%;}";
  page += "body {font-size:1.6rem;background-color:#fffde7;text-align:left;}";
  page += "div {font-size:1.6rem;text-align:center;}";
  page += "@media screen and (max-width: 480px) {body{font-size:1.4rem;} img{width:100%;height:auto;}}";
  page += "</style>";
  page += "</head>";
  page += "<body><pre>";
  return page;
}

String HTML_Footer2()
{
  String page;
  page += "</pre><br><br>";
  page += "<div><form><input type='button' name='button' value='close' onclick='window.close();'></form><div>";
  page += "</body></html>";
  return page;
}

String HTML_Header2Ng()
{
  String page;
  page = "<!DOCTYPE html>";
  page += "<html lang = 'ja'>";
  page += "<head>";
  page += "<meta charset='UTF-8'>";
  page += "<title>webApi</title>";
  page += "<base target='webApi'>";
  page += "<meta name='viewport' content='width=device-width,initial-scale=1.0'>";
  page += "<style>";
  page += "html {font-size: 62.5%;}";
  page += "body {font-size:1.6rem;background-color:#ffccff;text-align:left;}";
  page += "div {font-size:1.6rem;text-align:center;}";
  page += "@media screen and (max-width: 480px) {body{font-size:1.4rem;} img{width:100%;height:auto;}}";
  page += "</style>";
  page += "</head>";
  page += "<body><pre>";
  return page;
}

void requestManage()
{
  if (REQUEST_NO == 0)
    return;

  int req = REQUEST_NO;
  switch (req)
  {
  case REQ_REBOOT:
    REQUEST_NO = 0;
    REBOOT();
    return;

  case REQ_SHUTDOWN:
    REQUEST_NO = 0;
    POWER_OFF();
    return;

  default:
    REQUEST_NO = 0;
    Serial.println("requeestManage : invalid request get ");
  }
  return;
}

void sendReq(int reqNo)
{
  REQUEST_NO = reqNo;
}

// ---- test for webApi -----
void handle_test(AsyncWebServerRequest *request)
{
  webpage = "NG";
  String ok_str = request->arg("ok");
  wsHandleTest(ok_str);
}

void wsHandleTest(String okGetStr)
{
  // API TEST -> "/test?OK=true"
  //     return OK=true  else return NG   
  
  if (okGetStr.equalsIgnoreCase("true"))
  {
    webpage = "OK = true";
    Serial.println(webpage);
    return;
  }
  Serial.println(webpage);
  return;
}
