// *** Modified by NoRi 2025-03-18 ***
#include <SPIFFS.h> // Built-in
// #include <WiFi.h>              // Built-in
// #include <ESPmDNS.h>           // Built-in
#include <AsyncTCP.h>          // https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer
// #include "esp_system.h"        // Built-in
// #include "esp_spi_flash.h"     // Built-in
// #include "esp_wifi_types.h"    // Built-in
// #include "esp_bt.h"            // Built-in
#define FS SPIFFS //

// -------------------------------------------------------
void Home();
void Display_System_Info();
String HTML_Header();
String HTML_Footer();
extern String ConvBinUnits(int bytes, int resolution);
extern String EncryptionType(wifi_auth_mode_t encryptionType);
// -------------------------------------------------------

extern AsyncWebServer server;
extern String VERSION;
extern String webpage;
extern int start, downloadtime, uploadtime, downloadsize, uploadsize, downloadrate, uploadrate, numfiles;
extern float Temperature; // for example new page, amend in a sensor function if required
extern String Name;


extern String IP_ADDR;
extern String SERVER_NAME;

void Home()
{
  webpage = HTML_Header();
  webpage += "<br>";
  webpage += "<img src='icon.gif' alt='icon'>";
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
  webpage += "<h4>Transfer Statistics</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>Last Upload</th><th>Last Download/Stream</th><th>Units</th></tr>";
  webpage += "<tr><td>" + ConvBinUnits(uploadsize, 1) + "</td><td>" + ConvBinUnits(downloadsize, 1) + "</td><td>File Size</td></tr> ";
  webpage += "<tr><td>" + ConvBinUnits((float)uploadsize / uploadtime * 1024.0, 1) + "/Sec</td>";
  webpage += "<td>" + ConvBinUnits((float)downloadsize / downloadtime * 1024.0, 1) + "/Sec</td><td>Transfer Rate</td></tr>";
  webpage += "</table>";
  webpage += "<h4>Filing System</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>Total Space</th><th>Used Space</th><th>Free Space</th><th>Number of Files</th></tr>";
  webpage += "<tr><td>" + ConvBinUnits(FS.totalBytes(), 1) + "</td>";
  webpage += "<td>" + ConvBinUnits(FS.usedBytes(), 1) + "</td>";
  webpage += "<td>" + ConvBinUnits(FS.totalBytes() - FS.usedBytes(), 1) + "</td>";
  webpage += "<td>" + (numfiles == 0 ? "Pending Dir or Empty" : String(numfiles)) + "</td></tr>";
  webpage += "</table>";
  webpage += "<h4>CPU Information</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>Parameter</th><th>Value</th></tr>";
  webpage += "<tr><td>Number of Cores</td><td>" + String(chip_info.cores) + "</td></tr>";
  webpage += "<tr><td>Chip revision</td><td>" + String(chip_info.revision) + "</td></tr>";
  webpage += "<tr><td>Internal or External Flash Memory</td><td>" + String(((chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "Embedded" : "External")) + "</td></tr>";
  webpage += "<tr><td>Flash Memory Size</td><td>" + String((spi_flash_get_chip_size() / (1024 * 1024))) + " MB</td></tr>";
  webpage += "<tr><td>Current Free RAM</td><td>" + ConvBinUnits(ESP.getFreeHeap(), 1) + "</td></tr>";
  webpage += "</table>";
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
  page += ".topnav {overflow: visible;background-color:lightPink;}";
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
  page += ".topnav3 {overflow: visible;background-color:lightcyan;}";
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

  page += "<div class = 'topnav'>";
  page += "SPIFFS:　<a href='/dir'>Files</a>";
  page += "<a href='/upload'>Upload</a> ";
  page += "<a href='/download'>Download</a>";
  page += "<a href='/stream'>Stream</a>";
  page += "<a href='/delete'>Delete</a>";
  page += "<a href='/rename'>Rename</a>";
  page += "</div>";

  page += "<br><br>";
  page += "<div class = 'topnav2'>";
  page += "SD:　<a href='/SD_dir'>Files</a>";
  page += "<a href='/SD_upload'>Upload</a> ";
  page += "<a href='/SD_download'>Download</a>";
  page += "<a href='/SD_stream'>Stream</a>";
  page += "<a href='/SD_delete'>Delete</a>";
  page += "<a href='/SD_rename'>Rename</a>";
  page += "</div>";

  // ---- dir function ---------
  String SdPath = "/";
  page += "<div class = 'topnav2'>";
  page += "<a href='/root_sd'>Root</a>CurrentDir = " + SdPath;
  page += "<a href='/chdir'>Chdir</a> ";
  page += "<a href='/mkdir'>Mkdir</a> ";
  page += "<a href='/rmdir'>Rmdir</a>";
  page += "</div>";
  // --------------------------

  page += "<br><br>";
  page += "<div class = 'topnav'>";
  page += "<a href='/'>Home</a>";
  page += "<a href='/system'>StatusA</a>";
  page += "<a href='/SD_system'>StatusB</a>";
  // page += "<a href='/newpage'>New Page</a>";
  // page += "<a href='/logout'>[Log-out]</a>";
  page += "</div>";
  
  return page;
}


// #############################################################################################
// String HTML_Footer()
// {
//   String page;
//   page += "<br><br><footer>";
//   page += "<p class='medium'>m5stack file server</p>";
//   page += "<p class='ps'><i>Version " + Version + "</i></p>";
//   page += "</footer>";
//   page += "</body>";
//   page += "</html>";
//   return page;
// }

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
