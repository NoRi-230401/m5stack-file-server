// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-01
// -------------------------------------------------------
// fileServer.cpp
// *******************************************************
#include <Arduino.h>
#include <M5Unified.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <algorithm>
#include <vector>
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi_types.h"
#include "esp_bt.h"
#include "fileServer.h"

bool compareFileinfo(const fileinfo &a, const fileinfo &b);
bool wifiStart();
bool mdnsStart(void);
bool fileServerStart();
void notFound(AsyncWebServerRequest *request);
void Page_Not_Found();
void Home();
void Display_System_Info();
String HTML_Header();
String HTML_Footer();
// String ConvBytesUnits(uint64_t bytes, int dp);
String ConvBytesUnits(uint64_t bytes, int dp, int unit);
String EncryptionType(wifi_auth_mode_t encryptionType);
String getContentType(String filenametype);
// -------------------------------------------------------
extern bool SPIFFS_notFound(AsyncWebServerRequest *request);
extern void SPIFFS_flServerSetup();
extern void SPIFFS_Directory();
extern bool SPIFFS_isExists(const String filename);
extern uint32_t SPIFFS_startTime, SPIFFS_downloadTime, SPIFFS_uploadTime;
extern uint64_t SPIFFS_downloadSize, SPIFFS_uploadSize;
extern uint32_t SPIFFS_numfiles;
// -------------------------------------------------------
extern bool SD_notFound(AsyncWebServerRequest *request);
extern void SD_flServerSetup();
extern void SD_Directory();
extern bool SD_isExists(const String filename);
extern uint32_t SD_start, SD_downloadTime, SD_uploadTime;
extern uint64_t SD_downloadSize, SD_uploadSize;
extern uint32_t SD_numfiles;
extern bool SDdir_notFound(AsyncWebServerRequest *request);
extern void SDdir_flserverSetup();
// -------------------------------------------------------
String SSID, SSID_PASS, SERVER_NAME, IP_ADDR;
bool SD_ENABLE, SPIFFS_ENABLE;
const String ICON_FILE = "/icon.gif";
extern String SdPath;
AsyncWebServer server(80);
String webpage;

bool compareFileinfo(const fileinfo &a, const fileinfo &b)
{ // ファイル情報を比較するための関数
  // ディレクトリをファイルより前に配置
  if (a.ftype == "Dir" && b.ftype != "Dir")
  {
    return true;
  }
  if (a.ftype != "Dir" && b.ftype == "Dir")
  {
    return false;
  }
  // 同じタイプの場合はファイル名でソート
  return a.filename < b.filename;
}

bool wifiStart()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, SSID_PASS);

  int count = 1;
  const int COUNT_MAX = 10;
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    count++;
    M5.Display.printf(".");
    Serial.printf(".");
    // Serial.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(500);
    WiFi.begin(SSID, SSID_PASS);
    if (count >= COUNT_MAX)
    {
      Serial.printf("\nSTA: Failed!\n");
      return false;
    }
  }

  IP_ADDR = WiFi.localIP().toString();
  Serial.println("\nIP Address: " + IP_ADDR);
  if (WiFi.scanComplete() == -2)
    WiFi.scanNetworks(true);
  // Complete an initial scan for WiFi networks,
  //  otherwise = 0 on first display!

  return true;
}

bool mdnsStart(void)
{
  if (!MDNS.begin(SERVER_NAME.c_str()))
  {
    Serial.println("ERR: MDNS cannot start");
    Serial.println("ERR: ServerName = " + SERVER_NAME);
    return false;
  }

  Serial.println("mDNS ServerName = " + SERVER_NAME);
  return true;
}

bool fileServerStart()
{
  Serial.println(__FILE__);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  Serial.println("Home Page...");
  Home();
  request->send(200, "text/html", webpage); });

  server.on("/system", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  Display_System_Info();
  request->send(200, "text/html", webpage); });

  if (SPIFFS_ENABLE)
    SPIFFS_flServerSetup();

  if (SD_ENABLE)
  {
    SD_flServerSetup();
    SDdir_flserverSetup();
  }

  server.onNotFound(notFound);
  server.begin();
  if (SPIFFS_ENABLE)
    SPIFFS_Directory();

  return true;
}

String getContentType(String filenametype)
{
  if (filenametype == "download")
  {
    return "application/octet-stream";
  }
  else if (filenametype.endsWith(".txt"))
  {
    return "text/plain;charset=UTF-8";
  }
  else if (filenametype.endsWith(".htm"))
  {
    return "text/html;charset=UTF-8";
  }
  else if (filenametype.endsWith(".html"))
  {
    return "text/html;charset=UTF-8";
  }
  else if (filenametype.endsWith(".css"))
  {
    return "text/css;charset=UTF-8";
  }
  else if (filenametype.endsWith(".js"))
  {
    return "application/javascript";
  }
  else if (filenametype.endsWith(".png"))
  {
    return "image/png";
  }
  else if (filenametype.endsWith(".gif"))
  {
    return "image/gif";
  }
  else if (filenametype.endsWith(".jpg"))
  {
    return "image/jpeg";
  }
  else if (filenametype.endsWith(".ico"))
  {
    return "image/x-icon";
  }
  else if (filenametype.endsWith(".xml"))
  {
    return "text/xml;charset=UTF-8";
  }
  else if (filenametype.endsWith(".pdf"))
  {
    return "application/x-pdf";
  }
  else if (filenametype.endsWith(".zip"))
  {
    return "application/x-zip";
  }
  else if (filenametype.endsWith(".gz"))
  {
    return "application/x-gzip";
  }
  // ----- Add by NoRi 2025-03-27 ------------
  else if (filenametype.endsWith(".csv"))
  {
    return "text/csv;charset=UTF-8";
  }
  else if (filenametype.endsWith(".json"))
  {
    return "application/json;charset=UTF-8";
  }
  else if (filenametype.endsWith(".bmp"))
  {
    return "image/bmp";
  }
  else if (filenametype.endsWith(".wav"))
  {
    return "audio/wav";
  }
  else if (filenametype.endsWith(".mp3"))
  {
    return "audio/mp3";
  }
  else if (filenametype.endsWith(".mp4"))
  {
    return "video/mp4";
  }

  return "text/plain;charset=UTF-8";
}

void notFound(AsyncWebServerRequest *request)
{
  Serial.println("notFound func : " + request->url());

  if (SPIFFS_ENABLE)
  {
    if (SPIFFS_notFound(request))
      return;
  }

  if (SD_ENABLE)
  {
    if (SD_notFound(request))
      return;

    if (SDdir_notFound(request))
      return;
  }

  Page_Not_Found();
  request->send(200, "text/html", webpage);
}

void Page_Not_Found()
{
  webpage = HTML_Header();
  webpage += "<div class='notfound'>";
  webpage += "<h1>Sorry</h1>";
  webpage += "<p>Error 404 - Page Not Found</p>";
  webpage += "</div><div class='left'>";
  webpage += "<p>The page you were looking for was not found, it may have been moved or is currently unavailable.</p>";
  webpage += "<p>Please check the address is spelt correctly and try again.</p>";
  webpage += "<p>Or click <b><a href='/'>[Here]</a></b> for the home page.</p></div>";
  webpage += HTML_Footer();
}

void Home()
{
  webpage = HTML_Header();
  webpage += "<br>";

  if (SD_ENABLE && SD_isExists(ICON_FILE))
  {
    webpage += "<img src = 'SD_icon' alt='icon'>";
  }
  else if (SPIFFS_ENABLE && SPIFFS_isExists(ICON_FILE))
  {
    webpage += "<img src = 'SPIFFS_icon' alt='icon'>";
  }

  webpage += "<h3>[&nbsp;Home&nbsp;]　" + SERVER_NAME + "　IP=" + IP_ADDR + "</h3>";
  webpage += HTML_Footer();
}

void Display_System_Info()
{
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  if (WiFi.scanComplete() == -2)
    WiFi.scanNetworks(true, false);
  // Scan parameters are (async, show_hidden)
  // if async = true, don't wait for the result
  webpage = HTML_Header();
  webpage += "<h3>System Information</h3>";
  webpage += "<br><br>";

  if (SPIFFS_ENABLE)
  {
    // - 1.SPIFFS trx Statistics
    webpage += "<h4>SPIFFS:　Transfer Statistics</h4>";
    webpage += "<table class='center'>";
    webpage += "<tr><th>Last Upload</th><th>Last Download/Stream</th><th>Units</th></tr>";
    webpage += "<tr><td>" + ConvBytesUnits(SPIFFS_uploadSize, 1) + "</td><td>" + ConvBytesUnits(SPIFFS_downloadSize, 1) + "</td><td>File Size</td></tr> ";
    webpage += "<tr><td>" + ConvBytesUnits((float)SPIFFS_uploadSize / SPIFFS_uploadTime * 1024.0, 1) + "/Sec</td>";
    webpage += "<td>" + ConvBytesUnits((float)SPIFFS_downloadSize / SPIFFS_downloadTime * 1024.0, 1) + "/Sec</td><td>Transfer Rate</td></tr>";
    webpage += "</table>";

    // - 2.SPIFFS Filing-Sys
    webpage += "<h4>SPIFFS:　Filing System</h4>";
    webpage += "<table class='center'>";
    webpage += "<tr><th>Total Space</th><th>Used Space</th><th>Free Space</th><th>Number of Files</th></tr>";
    webpage += "<tr>";
    //-----------------------------------        
    uint64_t SPIFFS_total = (uint64_t)SPIFFS.totalBytes();
    uint64_t SPIFFS_used = (uint64_t)SPIFFS.usedBytes();
    uint64_t SPIFFS_free = SPIFFS_total -SPIFFS_used;
    webpage += "<td>" + ConvBytesUnits(SPIFFS_total,1) + "</td>";
    webpage += "<td>" + ConvBytesUnits(SPIFFS_used,1) + "</td>";
    webpage += "<td>" + ConvBytesUnits(SPIFFS_free,1) + "</td>";

    webpage += "<td>" + (SPIFFS_numfiles == 0 ? "Pending Dir or Empty" : String(SPIFFS_numfiles)) + "</td>";
    //-----------------------------------        
    webpage += "</tr>";
    webpage += "</table>";
    webpage += "<br><br>";
  }

  if (SD_ENABLE)
  {
    // - 3.SD trx Statistics
    webpage += "<h4>SD:　Transfer Statistics</h4>";
    webpage += "<table class='center'>";
    webpage += "<tr><th>Last Upload</th><th>Last Download/Stream</th><th>Units</th></tr>";
    webpage += "<tr><td>" + ConvBytesUnits(SD_uploadSize, 1) + "</td><td>" + ConvBytesUnits(SD_downloadSize, 1) + "</td><td>File Size</td></tr> ";
    webpage += "<tr><td>" + ConvBytesUnits((float)SD_uploadSize / SD_uploadTime * 1024.0, 1) + "/Sec</td>";
    webpage += "<td>" + ConvBytesUnits((float)SD_downloadSize / SD_downloadTime * 1024.0, 1) + "/Sec</td><td>Transfer Rate</td></tr>";
    webpage += "</table>";

    // - 4.SD Filing-Sys
    webpage += "<h4>SD:　Filing System</h4>";
    webpage += "<table class='center'>";
    webpage += "<tr><th>Total Space</th><th>Used Space</th><th>Free Space</th><th>Card Type</th></tr>";
    webpage += "<tr>";
    //-----------------------------------        
    uint64_t SD_total=(uint64_t)SD.totalBytes();
    uint64_t SD_used=(uint64_t)SD.usedBytes();
    uint64_t SD_free= SD_total - SD_used;
    webpage += "<td>" + ConvBytesUnits(SD_total, 1) + "</td>";
    webpage += "<td>" + ConvBytesUnits(SD_used, 1) + "</td>";
    webpage += "<td>" + ConvBytesUnits(SD_free, 1) + "</td>";

    sdcard_type_t cardType = SD.cardType();
    const String cType[] = {"NONE", "MMC", "SD", "SDHC", "UNKNOWN"};
    webpage += "<td>" + cType[cardType] + "</td>";
    //-----------------------------------        
    webpage += "</tr>";
    webpage += "</table>";
    webpage += "<br><br>";
  }

  // - 5.Heap
  webpage += "<h4>Free Heap Space</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>Total</th><th>PSRAM</th><th>SRAM</th><th>MaxAllocate DMA</th></tr><tr>";
  
  size_t FHS_total,FHS_psram,FHS_other,FHS_maxDMA;
  FHS_total = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  FHS_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  FHS_other = FHS_total - FHS_psram ;
  FHS_maxDMA = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
  
  webpage += "<td>" + ConvBytesUnits(FHS_total, 1,UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(FHS_psram, 1,UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(FHS_other, 1,UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(FHS_maxDMA, 1,UNIT_KIRO) + "</td>";
  webpage += "</tr></table>";

  webpage += "<br><br>";
  webpage += "<h4>Free Heap RAM Space2</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>Parameter</th><th>Value</th></tr>";
  webpage += "<tr><td>Heap Size</td><td>" + ConvBytesUnits(ESP.getHeapSize(), 1) + "</td></tr>";
  webpage += "<tr><td>Free Heap</td><td>" + ConvBytesUnits(ESP.getFreeHeap(), 1) + "</td></tr>";
  webpage += "<tr><td>Min Free Heap</td><td>" + ConvBytesUnits(ESP.getMinFreeHeap(), 1) + "</td></tr>";
  webpage += "<tr><td>Max Allocate Heap</td><td>" + ConvBytesUnits(ESP.getMaxAllocHeap(), 1) + "</td></tr>";
  webpage += "</table>";
  webpage += "<br><br>";

  // - 5.CPU Info
  webpage += "<h4>CPU Information</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>Parameter</th><th>Value</th></tr>";
  webpage += "<tr><td>Number of Cores</td><td>" + String(chip_info.cores) + "</td></tr>";
  webpage += "<tr><td>Chip revision</td><td>" + String(chip_info.revision) + "</td></tr>";
  webpage += "<tr><td>Internal or External Flash Memory</td><td>" + String(((chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "Embedded" : "External")) + "</td></tr>";
  webpage += "<tr><td>Flash Memory Size</td><td>" + String((spi_flash_get_chip_size() / (1024 * 1024))) + " MB</td></tr>";
  webpage += "<tr><td>Current Free RAM</td><td>" + ConvBytesUnits(ESP.getFreeHeap(), 1) + "</td></tr>";
  webpage += "</table>";
  webpage += "<br><br>";

  // - 6.Network Info
  webpage += "<h4>Network Information</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>Parameter</th><th>Value</th></tr>";
  webpage += "<tr><td>LAN IP Address</td><td>" + String(WiFi.localIP().toString()) + "</td></tr>";
  webpage += "<tr><td>Network Adapter MAC Address</td><td>" + String(WiFi.BSSIDstr()) + "</td></tr>";
  webpage += "<tr><td>WiFi SSID</td><td>" + String(WiFi.SSID()) + "</td></tr>";
  webpage += "<tr><td>WiFi RSSI</td><td>" + String(WiFi.RSSI()) + " dB</td></tr>";
  webpage += "<tr><td>WiFi Channel</td><td>" + String(WiFi.channel()) + "</td></tr>";
  webpage += "<tr><td>WiFi Encryption Type</td><td>" + String(EncryptionType(WiFi.encryptionType(0))) + "</td></tr>";
  webpage += "</table> ";
  webpage += "<br><br>";
  // ------------------------------------------------------

  // ------------------------------------------------------
  webpage += HTML_Footer();
}

String HTML_Header()
{
  String page;
  page = "<!DOCTYPE html>";
  page += "<html lang = 'ja'>";
  page += "<head>";
  page += "<title>Home</title>";
  page += "<base target='_self'>";
  page += "<meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width,initial-scale=1.0'>";
  page += "<style>";
  page += "@media screen and (max-width: 480px) {img{width:100%;height:auto;}}";
  page += "html {font-size: 62.5%;}";
  page += "body {width:100%;margin-left:auto;margin-right:auto;font-family:Arial,Helvetica,sans-serif;font-size:1.4rem;color:#2f4f4f;background-color:#fffacd;text-align:center;}";
  page += "footer {padding:1.0rem;background-color:cyan;font-size:1.4rem;}";
  page += "table {font-family:arial,sans-serif;border-collapse:collapse;width:80%;}";
  page += "table.center {margin-left:auto;margin-right:auto;}";
  page += "td, th {border:1px solid #dddddd;text-align:left;padding:0.8rem;}";
  page += "tr:nth-child(even) {background-color:#dddddd;}";
  page += "h3 {color:#6ecf12;font-size:1.8rem;font-style:normal;text-align:center;}";
  page += "h4 {color:slateblue;font-size:1.5rem;text-align:left;font-style:oblique;text-align:center;}";
  page += ".center {margin-left:auto;margin-right:auto;}";

  // TOPNAV
  page += ".topnav {overflow: visible;background-color:cyan;}";
  page += ".topnav a {float:center;color:blue;text-align:center;padding:1.0rem 1.0rem;text-decoration:none;font-size:1.6rem;}";
  page += ".topnav a:hover {background-color:deepskyblue;color:white;}";
  page += ".topnav a.active {background-color:lightblue;color:blue;}";

  // TOPNAV2
  page += ".topnav2 {overflow: visible;background-color:lightcyan;}";
  page += ".topnav2 a {float:center;color:blue;text-align:center;padding:1.2rem 1.2rem;text-decoration:none;font-size:1.5rem;}";
  page += ".topnav2 a:hover {background-color:deepskyblue;color:white;}";
  page += ".topnav2 a.active {background-color:lightblue;color:blue;}";
  page += ".notfound {padding:0.8rem;text-align:center;font-size:1.3rem;}";
  page += ".left {text-align:left;}";
  page += ".medium {font-size:1.9rem;padding:0;margin:0}";
  page += ".ps {font-size:1.4rem;padding:0;margin:0}";
  page += ".sp {background-color:silver;white-space:nowrap;width:2%;}";
  // --- end of style ---
  page += "</style></head><body>";

  // -- 1 --
  page += "<div class = 'topnav'>";
  page += "<a href='/'>Home</a>";
  page += "　　";
  page += "<a href='/system'>Status</a>";
  page += "</div>";

  // --------------- SPIFFS ------------------------
  if (SPIFFS_ENABLE)
  {
    // -- 2 SPIFFS --
    page += "<br>";
    page += "<div class = 'topnav2'>";
    page += "SPIFFS:<a href='/SPIFFS_dir'>Dir</a>";
    page += "<a href='/SPIFFS_upload'>Upload</a> ";
    page += "<a href='/SPIFFS_download'>Download</a>";
    page += "<a href='/SPIFFS_stream'>Stream</a>";
    page += "<a href='/SPIFFS_delete'>Delete</a>";
    page += "<a href='/SPIFFS_rename'>Rename</a>";
    page += "</div>";
  }

  // ------------------ SD -------------------------
  if (SD_ENABLE)
  {
    // -- 3 SD --
    page += "<br>";
    page += "<div class = 'topnav2'>";
    page += "SD:<a href='/SD_dir'>Dir</a>";
    page += "<a href='/SD_upload'>Upload</a> ";
    page += "<a href='/SD_download'>Download</a>";
    page += "<a href='/SD_stream'>Stream</a>";
    page += "<a href='/SD_delete'>Delete</a>";
    page += "<a href='/SD_rename'>Rename</a>";
    page += "</div>";

    // -- 4 SD path --
    page += "<div class = 'topnav2'>";
    page += "path:　" + SdPath;
    page += "<a href='/SDdir_chTop'>　Top　</a>";
    page += "<a href='/SDdir_chUp'>　Up　</a>";
    page += "</div>";

    // -- 5 SD dir --
    page += "<div class = 'topnav2'>";
    page += "<a href='/SDdir_chdir'>Chdir</a>";
    page += "<a href='/SDdir_mkdir'>Mkdir</a>";
    page += "<a href='/SDdir_rmdir'>Rmdir</a>";
    page += "</div>";
  }

  page += "<br>";
  return page;
}

String HTML_Footer()
{
  String page;
  page += "<br>";
  page += "<footer>";
  page += "<p class='ps'><i> " + PROG_NAME + "　" + VERSION + "</i></p>";
  page += "</footer>";
  page += "<br>";
  page += "</body>";
  page += "</html>";
  return page;
}

// String ConvBytesUnits(uint64_t bytes, int dp)
// { // int dp : 小数点以下の桁数、decimal places
//   const uint64_t KILO = 1024ULL;
//   const uint64_t MEGA = KILO * KILO;
//   const uint64_t GIGA = MEGA * KILO;
//   const uint64_t TERA = GIGA * KILO;

//   if (bytes < KILO)
//   {
//     return (String(bytes) + " B");
//   }
//   else if (bytes < MEGA)
//   {
//     float kb = (float)bytes / (float)KILO;
//     return String(kb, dp) + " KB";
//   }
//   else if (bytes < GIGA)
//   {
//     float mb = (float)bytes / (float)MEGA;
//     return (String(mb, dp) + " MB");
//   }
//   else if (bytes < TERA)
//   {
//     float gb = (float)bytes / (float)GIGA;
//     return (String(gb, dp) + " GB");
//   }
//   else
//   {
//     float tb = (float)bytes / (float)TERA;
//     return (String(tb, dp) + " TB");
//   }
// }

String ConvBytesUnits(uint64_t bytes, int dp, int unit)
{ // int dp : 小数点以下の桁数、decimal places
  const uint64_t KILO = 1024ULL;
  const uint64_t MEGA = KILO * KILO;
  const uint64_t GIGA = MEGA * KILO;
  const uint64_t TERA = GIGA * KILO;

  if (unit == UNIT_AUTO)
  {
    if (bytes < KILO)
    {
      return (String(bytes) + " B");
    }
    else if (bytes < MEGA)
    {
      float kb = (float)bytes / (float)KILO;
      return String(kb, dp) + " KB";
    }
    else if (bytes < GIGA)
    {
      float mb = (float)bytes / (float)MEGA;
      return (String(mb, dp) + " MB");
    }
    else if (bytes < TERA)
    {
      float gb = (float)bytes / (float)GIGA;
      return (String(gb, dp) + " GB");
    }
    else
    {
      float tb = (float)bytes / (float)TERA;
      return (String(tb, dp) + " TB");
    }
  }
  else if (unit == UNIT_KIRO)
  {
    float kb = (float)bytes / (float)KILO;
    return String(kb, dp) + " KB";
  }
  else if (unit == UNIT_MEGA)
  {
    float mb = (float)bytes / (float)MEGA;
    return (String(mb, dp) + " MB");
  }
  else if (unit == UNIT_GIGA)
  {
    float gb = (float)bytes / (float)GIGA;
    return (String(gb, dp) + " GB");
  }
  else if (unit == UNIT_TERA)
  {
    float tb = (float)bytes / (float)TERA;
    return (String(tb, dp) + " TB");
  }

  // UNIT_BYTE
  return (String(bytes) + " B");
}


String EncryptionType(wifi_auth_mode_t encryptionType)
{
  switch (encryptionType)
  {
  case (WIFI_AUTH_OPEN):
    return "OPEN";
  case (WIFI_AUTH_WEP):
    return "WEP";
  case (WIFI_AUTH_WPA_PSK):
    return "WPA PSK";
  case (WIFI_AUTH_WPA2_PSK):
    return "WPA2 PSK";
  case (WIFI_AUTH_WPA_WPA2_PSK):
    return "WPA WPA2 PSK";
  case (WIFI_AUTH_WPA2_ENTERPRISE):
    return "WPA2 ENTERPRISE";
  case (WIFI_AUTH_MAX):
    return "WPA2 MAX";
  default:
    return "";
  }
}

/*
void EPS32_system_info(void)
{
  Serial.println("\r\n-----------------------------");
  uint64_t chipid;

  // The chip ID is essentially its MAC address(length: 6 bytes).
  chipid = ESP.getEfuseMac();
  // print High 2 bytes
  Serial.printf("ESP32 Chip ID = %04X\r\n", (uint16_t)(chipid >> 32));
  Serial.printf("Chip Revision %d\r\n", ESP.getChipRevision());

  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  Serial.printf("Number of Core: %d\r\n", chip_info.cores);
  Serial.printf("CPU Frequency: %d MHz\r\n", ESP.getCpuFreqMHz());
  Serial.printf("Flash Chip Size = %d byte\r\n", ESP.getFlashChipSize());
  Serial.printf("Flash Frequency = %d Hz\r\n", ESP.getFlashChipSpeed());
  Serial.printf("ESP-IDF version = %s\r\n", esp_get_idf_version());

  // 利用可能なヒープのサイズを取得
  Serial.printf("Available Heap Size= %d\r\n", esp_get_free_heap_size());

  // 利用可能な内部ヒープのサイズを取得
  Serial.printf("Available Internal Heap Size = %d\r\n", esp_get_free_internal_heap_size());

  // これまでに利用可能だった最小ヒープを取得します
  Serial.printf("Minimum Free Heap Ever Available Size = %d\r\n", esp_get_minimum_free_heap_size());
  Serial.println();

  uint8_t mac0[6];
  esp_efuse_mac_get_default(mac0);
  Serial.printf("Default Mac Address = %02X:%02X:%02X:%02X:%02X:%02X\r\n", mac0[0], mac0[1], mac0[2], mac0[3], mac0[4], mac0[5]);

  uint8_t mac3[6];
  esp_read_mac(mac3, ESP_MAC_WIFI_STA);
  Serial.printf("[Wi-Fi Station] Mac Address = %02X:%02X:%02X:%02X:%02X:%02X\r\n", mac3[0], mac3[1], mac3[2], mac3[3], mac3[4], mac3[5]);

  uint8_t mac4[7];
  esp_read_mac(mac4, ESP_MAC_WIFI_SOFTAP);
  Serial.printf("[Wi-Fi SoftAP] Mac Address = %02X:%02X:%02X:%02X:%02X:%02X\r\n", mac4[0], mac4[1], mac4[2], mac4[3], mac4[4], mac4[5]);

  uint8_t mac5[6];
  esp_read_mac(mac5, ESP_MAC_BT);
  Serial.printf("[Bluetooth] Mac Address = %02X:%02X:%02X:%02X:%02X:%02X\r\n", mac5[0], mac5[1], mac5[2], mac5[3], mac5[4], mac5[5]);

  uint8_t mac6[6];
  esp_read_mac(mac6, ESP_MAC_ETH);
  Serial.printf("[Ethernet] Mac Address = %02X:%02X:%02X:%02X:%02X:%02X\r\n", mac6[0], mac6[1], mac6[2], mac6[3], mac6[4], mac6[5]);
}

void info_spiffs()
{
  float total_mb = SPIFFS.totalBytes() / (1024.0 * 1024.0);
  float used_mb = SPIFFS.usedBytes() / (1024.0 * 1024.0);
  float free_mb = total_mb - used_mb;

  char s[200];
  sprintf(s, "Total Space = %.3f MB", total_mb);
  Serial.println(s);

  sprintf(s, "Used Space = %.3f MB", used_mb);
  Serial.println(s);

  sprintf(s, "Free Space = %.3f MB", free_mb);
  Serial.println(s);
}

// 空きメモリをシリアル出力
void log_free_size(const char *text)
{
  M5.Log.printf("%s ** free size of Memory(kB): %4d-%4d-%3d  [def-ps-dma] **\n", text,
                heap_caps_get_free_size(MALLOC_CAP_DEFAULT) / 1024,
                heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024,
                heap_caps_get_free_size(MALLOC_CAP_DMA) / 1024);
}

*/
