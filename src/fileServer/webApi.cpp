// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-15
// -------------------------------------------------------
// webApi.cpp
// *******************************************************
#include "fileServer.h"

void webApiSetup();
void handle_shutdown(AsyncWebServerRequest *request);
void wsHandleShutdown(String reboot_get_str, String time_get_str);
void serverSend(AsyncWebServerRequest *request);
String HTML_Header2();
String HTML_Footer2();
String HTML_Header2Ng();
void wsHandleTest(String okGetStr);
// -------------------------------------------------------

extern AsyncWebServer server;
extern String webpage;


// --- test for webApi  ---
// #define TEST_EXECUTE
#ifdef TEST_EXECUTE
void handle_test(AsyncWebServerRequest *request);
void wsHandleTest(String okGetStr);
#endif

void webApiSetup()
{
  server.on("/shutdown", HTTP_GET, [](AsyncWebServerRequest *request)
            { handle_shutdown(request);  serverSend(request); });

#ifdef TEST_EXECUTE
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request)
            { handle_test(request);  serverSend(request); });
#endif
}

void handle_shutdown(AsyncWebServerRequest *request)
{
  webpage = "NG";
  String reboot_get_str = request->arg("reboot");
  String time_get_str = request->arg("time");
  wsHandleShutdown(reboot_get_str, time_get_str);
}

#define SHUTDOWN_MIN_TM 3L    // minimum shutdown wait time
void wsHandleShutdown(String reboot_get_str, String time_get_str)
{
  uint32_t time_sec = SHUTDOWN_MIN_TM;

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

#ifdef TEST_EXECUTE
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
#endif
