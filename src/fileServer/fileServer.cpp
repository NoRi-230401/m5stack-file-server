// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-01
// -------------------------------------------------------
// fileServer.cpp
// *******************************************************
#include "fileServer.h"

void Display_System_Info();
bool fileServerStart();
void notFound(AsyncWebServerRequest *request);
void Page_Not_Found();
void Home();
String HTML_Header();
String HTML_Footer();
String EncryptionType(wifi_auth_mode_t encryptionType);
String getContentType(String filenametype);
bool compareFileinfo(const fileinfo &a, const fileinfo &b);
// -------------------------------------------------------
extern bool SPIFFS_notFound(AsyncWebServerRequest *request);
extern void SPIFFS_flServerSetup();
extern void SPIFFS_Directory();
extern uint32_t SPIFFS_startTime, SPIFFS_downloadTime, SPIFFS_uploadTime;
extern uint64_t SPIFFS_downloadSize, SPIFFS_uploadSize;
extern uint32_t SPIFFS_numfiles;
// -------------------------------------------------------
extern bool SD_notFound(AsyncWebServerRequest *request);
extern void SD_flServerSetup();
extern void SD_Directory();
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

void Display_System_Info()
{
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  // if (WiFi.scanComplete() == -2)
  //   WiFi.scanNetworks(true, false);
  // Scan parameters are (async, show_hidden)
  // if async = true, don't wait for the result
  webpage = HTML_Header();
  webpage += "<h3>Status and System Information</h3>";
  webpage += "<br>";

  if (SPIFFS_ENABLE)
  {
    // - SPIFFS trx Statistics
    webpage += "<h4>SPIFFS:　Transfer Statistics</h4>";
    webpage += "<table class='center'>";
    webpage += "<tr><th>last upload</th><th>last download/stream</th><th>units</th></tr>";
    webpage += "<tr><td>" + ConvBytesUnits(SPIFFS_uploadSize, 1) + "</td><td>" + ConvBytesUnits(SPIFFS_downloadSize, 1) + "</td><td>File Size</td></tr> ";
    webpage += "<tr><td>" + ConvBytesUnits((float)SPIFFS_uploadSize / SPIFFS_uploadTime * 1024.0, 1) + "/Sec</td>";
    webpage += "<td>" + ConvBytesUnits((float)SPIFFS_downloadSize / SPIFFS_downloadTime * 1024.0, 1) + "/Sec</td><td>Transfer Rate</td></tr>";
    webpage += "</table>";
    webpage += "<br><br>";

    // - SPIFFS Filing-Sys
    webpage += "<h4>SPIFFS:　Filing System</h4>";
    webpage += "<table class='center'>";
    webpage += "<tr><th>total space</th><th>used space</th><th>free space</th><th>number of files</th></tr>";
    webpage += "<tr>";
    //-----------------------------------
    uint64_t SPIFFS_total = (uint64_t)SPIFFS.totalBytes();
    uint64_t SPIFFS_used = (uint64_t)SPIFFS.usedBytes();
    uint64_t SPIFFS_free = SPIFFS_total - SPIFFS_used;
    webpage += "<td>" + ConvBytesUnits(SPIFFS_total, 1, UNIT_KIRO) + "</td>";
    webpage += "<td>" + ConvBytesUnits(SPIFFS_used, 1, UNIT_KIRO) + "</td>";
    webpage += "<td>" + ConvBytesUnits(SPIFFS_free, 1, UNIT_KIRO) + "</td>";

    webpage += "<td>" + (SPIFFS_numfiles == 0 ? "Pending Dir or Empty" : String(SPIFFS_numfiles)) + "</td>";
    //-----------------------------------
    webpage += "</tr>";
    webpage += "</table>";
    webpage += "<br><br>";
  }

  if (SD_ENABLE)
  {
    // - SD trx Statistics
    webpage += "<h4>SD:　Transfer Statistics</h4>";
    webpage += "<table class='center'>";
    webpage += "<tr><th>last upload</th><th>last download/stream</th><th>units</th></tr>";
    webpage += "<tr><td>" + ConvBytesUnits(SD_uploadSize, 1) + "</td><td>" + ConvBytesUnits(SD_downloadSize, 1) + "</td><td>File Size</td></tr> ";
    webpage += "<tr><td>" + ConvBytesUnits((float)SD_uploadSize / SD_uploadTime * 1024.0, 1) + "/Sec</td>";
    webpage += "<td>" + ConvBytesUnits((float)SD_downloadSize / SD_downloadTime * 1024.0, 1) + "/Sec</td><td>Transfer Rate</td></tr>";
    webpage += "</table>";
    webpage += "<br>";

    // - SD Filing-Sys
    webpage += "<h4>SD:　Filing System</h4>";
    webpage += "<table class='center'>";
    webpage += "<tr><th>total space</th><th>used space</th><th>free space</th><th>card type</th></tr>";
    webpage += "<tr>";
    //-----------------------------------
    uint64_t SD_total = (uint64_t)SD.totalBytes();
    uint64_t SD_used = (uint64_t)SD.usedBytes();
    uint64_t SD_free = SD_total - SD_used;
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

  // - program size
  webpage += "<h4>Program size in FLASH</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>total space</th><th>used program size</th><th>free space</th></tr>";
  webpage += "<tr>";
  //-----------------------------------
  uint64_t prog_max = (uint64_t)ESP.getFreeSketchSpace();
  uint64_t prog_used = (uint64_t)ESP.getSketchSize();
  uint64_t prog_available = prog_max - prog_used;
  webpage += "<td>" + ConvBytesUnits(prog_max, 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(prog_used, 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(prog_available, 1, UNIT_KIRO) + "</td>";
  //-----------------------------------
  webpage += "</tr>";
  webpage += "</table>";
  webpage += "<br><br>";

  //-------------------
  // - SRAM: Internal RAM
  webpage += "<h4>Internal RAM (SRAM)</h4><table class='center'>";
  webpage += "<tr><th>total heap size</th><th>free heap</th><th>min free heap<br>since boot</th><th>available max<br>allocate block</th></tr><tr>";
  webpage += "<td>" + ConvBytesUnits(ESP.getHeapSize(), 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(ESP.getFreeHeap(), 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(ESP.getMinFreeHeap(), 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(ESP.getMaxAllocHeap(), 1, UNIT_KIRO) + "</td>";
  webpage += "</tr></table>";
  //-------------------
  webpage += "<br><br>";

  // - PSRAM : External RAM
  webpage += "<h4>External RAM (PSRAM)</h4><table class='center'>";
  webpage += "<tr><th>total heap size</th><th>free heap</th><th>min free heap<br>since boot</th><th>available max<br>allocate block</th></tr><tr>";
  webpage += "<td>" + ConvBytesUnits(ESP.getPsramSize(), 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(ESP.getFreePsram(), 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(ESP.getMinFreePsram(), 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(ESP.getMaxAllocPsram(), 1, UNIT_KIRO) + "</td>";
  webpage += "</tr></table>";
  webpage += "<br><br>";
  //-------------------

  // - NVS
  nvs_stats_t nvsStats;
  if (ESP_OK == nvs_get_stats("nvs", &nvsStats))
  {
    webpage += "<h4>NVS : Non-Volatile Storage</h4>";
    webpage += "<table class='center'>";
    webpage += "<tr><th>available entries</th><th>used entries</th><th>free entries</th><th>name space</th></tr><tr>";

    size_t total_ent = nvsStats.total_entries;
    size_t used_ent = nvsStats.used_entries;
    size_t free_ent = nvsStats.free_entries;
    size_t namespace_cnt = nvsStats.namespace_count;
    webpage += "<td>" + String(total_ent) + "</td>";
    webpage += "<td>" + String(used_ent) + "</td>";
    webpage += "<td>" + String(free_ent) + "</td>";
    webpage += "<td>" + String(namespace_cnt) + "</td>";

    webpage += "</tr></table>";
    webpage += "<br><br>";
  }

  // - CPU information
  webpage += "<h4>CPU Information</h4><table class='center'>";
  webpage += "<tr><th>parameter</th><th>value</th></tr>";
  //-------------------
  webpage += "<tr><td>CPU Model</td><td>" + String(ESP.getChipModel()) + "</td></tr>";
  webpage += "<tr><td>Chip revision</td><td>" + String(ESP.getChipRevision()) + "</td></tr>";
  webpage += "<tr><td>SDK Version</td><td>" + String(ESP.getSdkVersion()) + "</td></tr>";

  webpage += "<tr><td>Number of Cores</td><td>" + String(ESP.getChipCores()) + "</td></tr>";
  // webpage += "<tr><td>Cycle Count</td><td>" + String(ESP.getCycleCount()) + "</td></tr>";

  webpage += "<tr><td>CPU Freq</td><td>" + String(ESP.getCpuFreqMHz()) + " MHz" + "</td></tr>";
  webpage += "<tr><td>Flash Memory Size</td><td>" + ConvBytesUnits(ESP.getFlashChipSize(), 0, UNIT_AUTO) + "</td></tr>";
  webpage += "<tr><td>Flash Freq</td><td>" + String(ESP.getFlashChipSpeed() / 1000000) + " MHz" + "</td></tr>";
  // webpage += "<tr><td>Flash Chip Mode</td><td>" + String(ESP.getFlashChipMode()) + "</td></tr>";
  //-------------------
  webpage += "</table>";
  webpage += "<br><br>";

  // - MAC Address
  webpage += "<h4>MAC Address</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>parameter</th><th>value</th></tr>";
  //-------------------
  char buf[256];
  uint8_t mac0[6];
  uint64_t chipid;
  esp_read_mac(mac0, ESP_MAC_WIFI_STA);
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac0[0], mac0[1], mac0[2], mac0[3], mac0[4], mac0[5]);
  webpage += "<tr><td>WiFi STAtion MAC (default)</td><td>" + String(buf) + "</td></tr>";
  esp_read_mac(mac0, ESP_MAC_WIFI_SOFTAP);
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac0[0], mac0[1], mac0[2], mac0[3], mac0[4], mac0[5]);
  webpage += "<tr><td>WiFi softAP MAC</td><td>" + String(buf) + "</td></tr>";
  esp_read_mac(mac0, ESP_MAC_BT);
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac0[0], mac0[1], mac0[2], mac0[3], mac0[4], mac0[5]);
  webpage += "<tr><td>Bluetooth MAC</td><td>" + String(buf) + "</td></tr>";
  esp_read_mac(mac0, ESP_MAC_ETH);
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac0[0], mac0[1], mac0[2], mac0[3], mac0[4], mac0[5]);
  webpage += "<tr><td>Ethernet MAC</td><td>" + String(buf) + "</td></tr>";
  //-------------------
  webpage += "</table>";
  webpage += "<br><br>";

  // - Network Info
  webpage += "<h4>Network Information</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>parameter</th><th>value</th></tr>";
  webpage += "<tr><td>IP Address</td><td>" + String(WiFi.localIP().toString()) + "</td></tr>";
  webpage += "<tr><td>Server Name (hostName)</td><td>" + SERVER_NAME + "</td></tr>";
  webpage += "<tr><td>WiFi SSID</td><td>" + String(WiFi.SSID()) + "</td></tr>";
  webpage += "<tr><td>WiFi BSSID</td><td>" + String(WiFi.BSSIDstr()) + "</td></tr>";
  // webpage += "<tr><td>WiFi RSSI</td><td>" + String(WiFi.RSSI()) + " dB</td></tr>";
  // webpage += "<tr><td>WiFi Channel</td><td>" + String(WiFi.channel()) + "</td></tr>";
  webpage += "<tr><td>WiFi Encryption Type</td><td>" + String(EncryptionType(WiFi.encryptionType(0))) + "</td></tr>";
  webpage += "</table> ";
  webpage += "<br><br>";

  // - clock
  webpage += "<h4>clock</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>parameter</th><th>value</th></tr>";
  
  if(RTC_ENABLE)
    webpage += "<tr><td>Real Time Clock (RTC)</td><td>" + getTmRTC() + "</td></tr>";
  else
    webpage += "<tr><td>Real Time Clock (RTC)</td><td>　**　disable　**　</td></tr>";
  
  webpage += "<tr><td>Sync with NTP server</td><td>" + getTmNTP() + "</td></tr>";
  webpage += "</table> ";
  webpage += "<br><br>";

  // ------------------------------------------------------
  webpage += HTML_Footer();
}

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
  {
    SPIFFS_flServerSetup();
    server.on("/SPIFFS_icon", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, ICON_FILE, "image/gif"); });
  }

  if (SD_ENABLE)
  {
    SD_flServerSetup();
    SDdir_flserverSetup();

    server.on("/SD_icon", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SD, ICON_FILE, "image/gif"); });
  }

  // favicon.ico
  if (SD_ENABLE && SD.exists("/favicon.ico"))
  {
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SD, "/favicon.ico", "image/x-icon"); });
  }
  else if (SPIFFS_ENABLE && SPIFFS.exists("/favicon.ico"))
  {
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/favicon.ico", "image/x-icon"); });
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

  // if (SD_ENABLE && SD_isExists(ICON_FILE))
  if (SD_ENABLE && SD.exists(ICON_FILE))
  {
    webpage += "<img src = 'SD_icon' alt='icon'>";
  }
  // else if (SPIFFS_ENABLE && SPIFFS_isExists(ICON_FILE))
  else if (SPIFFS_ENABLE && SPIFFS.exists(ICON_FILE))
  {
    webpage += "<img src = 'SPIFFS_icon' alt='icon'>";
  }

  webpage += "<h3>[&nbsp;Home&nbsp;]　" + SERVER_NAME + "　IP=" + IP_ADDR + "</h3>";
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
  // page += "h3 {color:#6ecf12;font-size:1.8rem;font-style:normal;text-align:center;}";
  page += "h3 {color:#6ecf12;font-size:1.7rem;font-style:normal;text-align:center;}";
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
  page += "　　";
  // page += "<a href='/reboot'>Reboot</a>";
  // page += "<a href='/shutdown'>Shutdown</a>";

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
  page += "<br><br><br>";
  page += "<footer>";
  page += "<p class='ps'><i>" + getTmNTP() + "　　" + PROG_NAME + "　" + VERSION + "</i></p>";
  page += "</footer>";
  page += "<br>";
  page += "</body>";
  page += "</html>";
  return page;
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
