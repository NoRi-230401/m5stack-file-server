// *** Modified by NoRi 2025-03-18 ***
#include <Arduino.h>
#include <M5Unified.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi_types.h"
#include "esp_bt.h"
#define FS SPIFFS
#include "credentials.h"

// -------------------------------------------------------
bool wifiStart();
bool fileServerStart();
void notFound(AsyncWebServerRequest *request);
void Page_Not_Found();
void Home();
void Display_System_Info();
String HTML_Header();
String HTML_Footer();
String ConvBinUnits(int bytes, int resolution);
String EncryptionType(wifi_auth_mode_t encryptionType);
bool StartMDNSservice(const char *Name);
String getContentType(String filenametype);
void SelectInput(String Heading, String Command, String Arg_name);
// -------------------------------------------------------
extern bool SD_notFound(AsyncWebServerRequest *request);
extern bool SPIFFS_notFound(AsyncWebServerRequest *request);
extern void SPIFFS_flServerSetup();
extern void SD_flServerSetup();
extern bool StartMDNSservice(const char *Name);
extern bool StartupErrors;
extern void Directory();
extern void SD_Directory();
extern String SD_totalBytes(int res);
extern String SD_usedBytes(int res);
extern String SD_numFiles(int res);
extern String webpage;
extern int start, downloadtime, uploadtime, downloadsize, uploadsize, downloadrate, uploadrate, numfiles;
extern int SD_start, SD_downloadtime, SD_uploadtime, SD_downloadsize, SD_uploadsize, SD_downloadrate, SD_uploadrate, SD_numfiles;
extern String VERSION;
extern String IP_ADDR;
extern String SERVER_NAME;
// -------------------------------------------------------

AsyncWebServer server(80);

bool wifiStart()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

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

bool fileServerStart()
{
  if (!StartMDNSservice(ServerName))
  {
    Serial.println("Error starting mDNS Service...");
    ;
    StartupErrors = true;
    return false;
  }

  SPIFFS_flServerSetup();
  SD_flServerSetup();
  server.onNotFound(notFound);

  server.begin(); // Start the server
  if (StartupErrors)
  {
    Serial.println("There were problems starting all services...");
    return false;
  }
  Directory();    // Update the SPIFFS file list
  SD_Directory(); // Update the SD file list
  Serial.println("System started successfully...");
  return true;
}

// #############################################################################################
String getContentType(String filenametype)
{ // Tell the browser what file type is being sent
  if (filenametype == "download")
  {
    return "application/octet-stream";
  }
  else if (filenametype.endsWith(".txt"))
  {
    return "text/plainn";
  }
  else if (filenametype.endsWith(".htm"))
  {
    return "text/html";
  }
  else if (filenametype.endsWith(".html"))
  {
    return "text/html";
  }
  else if (filenametype.endsWith(".css"))
  {
    return "text/css";
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
    return "text/xml";
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
  return "text/plain";
}

// #############################################################################################
void notFound(AsyncWebServerRequest *request)
{
  Serial.println("notFound func : " + request->url() );

  if (SD_notFound(request))
    return;

  if (SPIFFS_notFound(request))
    return;

  Page_Not_Found();
  request->send(200, "text/html", webpage);
}

// #############################################################################################
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

// #############################################################################################
void Home()
{
  webpage = HTML_Header();
  webpage += "<br>";
  webpage += "<img src = 'icon' alt='icon'>";
  webpage += "<h3>[&nbsp;Home&nbsp;]　" + SERVER_NAME + "　IP=" + IP_ADDR + "</h3>";
  webpage += HTML_Footer();
}

// #############################################################################################
void Display_System_Info()
{
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  if (WiFi.scanComplete() == -2)
    WiFi.scanNetworks(true, false); // Scan parameters are (async, show_hidden) if async = true, don't wait for the result
  webpage = HTML_Header();
  webpage += "<h3>System Information</h3>";
  
  webpage += "<br>";
  webpage += "<h4>SPIFFS:　Transfer Statistics</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>Last Upload</th><th>Last Download/Stream</th><th>Units</th></tr>";
  webpage += "<tr><td>" + ConvBinUnits(uploadsize, 1) + "</td><td>" + ConvBinUnits(downloadsize, 1) + "</td><td>File Size</td></tr> ";
  webpage += "<tr><td>" + ConvBinUnits((float)uploadsize / uploadtime * 1024.0, 1) + "/Sec</td>";
  webpage += "<td>" + ConvBinUnits((float)downloadsize / downloadtime * 1024.0, 1) + "/Sec</td><td>Transfer Rate</td></tr>";
  webpage += "</table>";

  webpage += "<br><br>";
  webpage += "<h4>SD:　Transfer Statistics</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>Last Upload</th><th>Last Download/Stream</th><th>Units</th></tr>";
  webpage += "<tr><td>" + ConvBinUnits(SD_uploadsize, 1) + "</td><td>" + ConvBinUnits(SD_downloadsize, 1) + "</td><td>File Size</td></tr> ";
  webpage += "<tr><td>" + ConvBinUnits((float)SD_uploadsize / SD_uploadtime * 1024.0, 1) + "/Sec</td>";
  webpage += "<td>" + ConvBinUnits((float)SD_downloadsize / SD_downloadtime * 1024.0, 1) + "/Sec</td><td>Transfer Rate</td></tr>";
  webpage += "</table>";

  webpage += "<br><br>";
  webpage += "<h4>SPIFFS:　Filing System</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>Total Space</th><th>Used Space</th><th>Free Space</th><th>Number of Files</th></tr>";
  webpage += "<tr><td>" + ConvBinUnits(FS.totalBytes(), 1) + "</td>";
  webpage += "<td>" + ConvBinUnits(FS.usedBytes(), 1) + "</td>";
  webpage += "<td>" + ConvBinUnits(FS.totalBytes() - FS.usedBytes(), 1) + "</td>";
  webpage += "<td>" + (numfiles == 0 ? "Pending Dir or Empty" : String(numfiles)) + "</td></tr>";
  webpage += "</table>";
  
  webpage += "<br><br>";
  webpage += "<h4>SD:　Filing System</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>Total Space</th><th>Used Space</th><th>Free Space</th><th>Number of Files</th></tr>";
  webpage += "<tr><td>" + SD_totalBytes(1) + "</td>";
  webpage += "<td>" + SD_usedBytes(1) + "</td>";
  webpage += "<td>" + SD_numFiles(1) + "</td>";
  webpage += "<td>" + (SD_numfiles == 0 ? "Pending Dir or Empty" : String(SD_numfiles)) + "</td></tr>";
  webpage += "</table>";
  
  webpage += "<br><br>";
  webpage += "<h4>CPU Information</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>Parameter</th><th>Value</th></tr>";
  webpage += "<tr><td>Number of Cores</td><td>" + String(chip_info.cores) + "</td></tr>";
  webpage += "<tr><td>Chip revision</td><td>" + String(chip_info.revision) + "</td></tr>";
  webpage += "<tr><td>Internal or External Flash Memory</td><td>" + String(((chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "Embedded" : "External")) + "</td></tr>";
  webpage += "<tr><td>Flash Memory Size</td><td>" + String((spi_flash_get_chip_size() / (1024 * 1024))) + " MB</td></tr>";
  webpage += "<tr><td>Current Free RAM</td><td>" + ConvBinUnits(ESP.getFreeHeap(), 1) + "</td></tr>";
  webpage += "</table>";

  webpage += "<br><br>";
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

  webpage += HTML_Footer();
}

// #############################################################################################
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
  page += ".topnav a {float:center;color:blue;text-align:center;padding:1.0rem 1.0rem;text-decoration:none;font-size:1.5rem;}";
  page += ".topnav a:hover {background-color:deepskyblue;color:white;}";
  page += ".topnav a.active {background-color:lightblue;color:blue;}";

  // TOPNAV2
  page += ".topnav2 {overflow: visible;background-color:lightcyan;}";
  page += ".topnav2 a {float:center;color:blue;text-align:center;padding:1.0rem 1.0rem;text-decoration:none;font-size:1.5rem;}";
  page += ".topnav2 a:hover {background-color:deepskyblue;color:white;}";
  page += ".topnav2 a.active {background-color:lightblue;color:blue;}";
  page += ".notfound {padding:0.8rem;text-align:center;font-size:1.3rem;}";
  page += ".left {text-align:left;}";
  page += ".medium {font-size:1.9rem;padding:0;margin:0}";
  page += ".ps {font-size:1.4rem;padding:0;margin:0}";
  page += ".sp {background-color:silver;white-space:nowrap;width:2%;}";

  // TOPNAV3
  page += ".topnav3 {overflow: visible;background-color:lightPink;}";
  // page += ".topnav3 {overflow: visible;background-color:lightcyan;}";
  page += ".topnav3 a {float:center;color:blue;text-align:center;padding:1.0rem 1.0rem;text-decoration:none;font-size:1.5rem;}";
  page += ".topnav3 a:hover {background-color:deepskyblue;color:white;}";
  page += ".topnav3 a.active {background-color:lightblue;color:blue;}";
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
  page += "<a href='/system'>Status</a>";
  page += "</div>";

  // -- 2 -- 
  // page += "<br><br>";
  page += "<br>";
  page += "<div class = 'topnav2'>";
  page += "SPIFFS:　<a href='/dir'>Files</a>";
  page += "<a href='/upload'>Upload</a> ";
  page += "<a href='/download'>Download</a>";
  page += "<a href='/stream'>Stream</a>";
  page += "<a href='/delete'>Delete</a>";
  page += "<a href='/rename'>Rename</a>";
  page += "</div>";

  // -- 3 -- 
  page += "<br>";
  page += "<div class = 'topnav2'>";
  page += "SD:　<a href='/SD_dir'>Files</a>";
  page += "<a href='/SD_upload'>Upload</a> ";
  page += "<a href='/SD_download'>Download</a>";
  page += "<a href='/SD_stream'>Stream</a>";
  page += "<a href='/SD_delete'>Delete</a>";
  page += "<a href='/SD_rename'>Rename</a>";
  page += "</div>";

  // -- 4 -- 
  // page += "<br>";
  String SdPath = "/";
  page += "<div class = 'topnav2'>";
  page += "<a href='/root_sd'>Root</a>CurrentDir = " + SdPath;
  page += "<a href='/chdir'>Chdir</a> ";
  page += "<a href='/mkdir'>Mkdir</a> ";
  page += "<a href='/rmdir'>Rmdir</a>";
  page += "</div>";
  page += "<br><br>";

  return page;
}

// #############################################################################################
String HTML_Footer()
{
  String page;
  page += "<br>";
  page += "<footer>";
  page += "<p class='medium'>m5stack file server</p>";
  // page += "<p class='medium'> Server Name : " + SERVER_NAME + "</p>";
  page += "<p class='ps'><i> " + VERSION + "</i></p>";
  page += "</footer>";
  page += "<br>";
  page += "</body>";
  page += "</html>";
  return page;
}

// #############################################################################################
String ConvBinUnits(int bytes, int resolution)
{// int resolution : 小数点以下の桁数、decimal places
  if (bytes < 1024)
  {
    return String(bytes) + " B";
  }
  else if (bytes < 1024 * 1024)
  {
    return String((bytes / 1024.0), resolution) + " KB";
  }
  else if (bytes < (1024 * 1024 * 1024))
  {
    return String((bytes / 1024.0 / 1024.0), resolution) + " MB";
  }
  else
    return "";
}

// #############################################################################################
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

// #############################################################################################
bool StartMDNSservice(const char *Name)
{
  esp_err_t err = mdns_init(); // Initialise mDNS service
  if (err)
  {
    printf("MDNS Init failed: %d\n", err);
    SERVER_NAME = "";
    return false;
  }
  mdns_hostname_set(Name); // Set hostname
  SERVER_NAME = String(Name);
  return true;
}

// #############################################################################################
void SelectInput(String Heading, String Command, String Arg_name)
{
  webpage = HTML_Header();
  webpage += "<h3>" + Heading + "</h3>";
  webpage += "<form  action='/" + Command + "'>";
  webpage += "Filename: <input type='text' name='" + Arg_name + "'><br><br>";
  webpage += "<input type='submit' value='Enter'>";
  webpage += "</form>";
  webpage += HTML_Footer();
}
