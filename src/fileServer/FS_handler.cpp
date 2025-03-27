// *******************************************************
//  m5stack-fileServer          by NoRi 2025-01-23
// -------------------------------------------------------
// FS_handler.cpp
// *******************************************************
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <algorithm>
#include <vector>
#define FS SPIFFS
#include "fileServer.h"

bool FS_Start();
void FS_flServerSetup();
void FS_Dir(AsyncWebServerRequest *request);
void FS_Directory();
void FS_UploadFileSelect();
void FS_handleFileUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);
void FS_File_Stream();
void FS_File_Delete();
void FS_Handle_File_Delete(String filename);
void FS_File_Rename();
void FS_Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args);
bool FS_notFound(AsyncWebServerRequest *request);
void FS_Handle_File_Download();
void FS_Select_File_For_Function(String title, String function);
int FS_GetFileSize(String filename);
String FS_StatusReport(int reportNo, int decimalPlaces);
bool FS_isExists(const String filename);
bool FS_SettingRd(const String filename);
extern String SSID, SSID_PASS, SERVER_NAME;

// -------------------------------------------------------
extern bool compareFileinfo(const fileinfo &a, const fileinfo &b);
extern void SelectInput(String Heading, String Command, String Arg_name);
extern String getContentType(String filenametype);
extern String ConvBinUnits(uint64_t bytes, int resolution);
extern String EncryptionType(wifi_auth_mode_t encryptionType);
extern String HTML_Header();
extern String HTML_Footer();
extern AsyncWebServer server;
extern String webpage;
// -------------------------------------------------------
std::vector<fileinfo> FS_Filenames;
String FS_MessageLine;
int FS_start, FS_downloadtime = 1, FS_uploadtime = 1, FS_downloadsize, FS_uploadsize, FS_downloadrate, FS_uploadrate, FS_numfiles;

bool FS_Start()
{
  if (!FS.begin(true))
  {
    Serial.println("ERR: SPIFFS begin erro...");
    return false;
  }
  return true;
}

void FS_flServerSetup()
{
  Serial.println(__FILE__);

  server.on("/FS_download", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("FS Downloading file...");
    FS_Select_File_For_Function("[DOWNLOAD]", "FS_downloadhandler");
    request->send(200, "text/html", webpage); });

  server.on("/FS_upload", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("FS Uploading file...");
    FS_UploadFileSelect(); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  server.on("/FS_handleupload", HTTP_POST, [](AsyncWebServerRequest *request) {}, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
            { FS_handleFileUpload(request, filename, index, data, len, final); });

  server.on("/FS_stream", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("FS Streaming file...");
    FS_Select_File_For_Function("[STREAM]", "FS_streamhandler"); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  server.on("/FS_rename", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("FS Renaming file...");
    FS_File_Rename(); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  server.on("/FS_dir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("FS File Directory...");
    FS_Dir(request); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  server.on("/FS_delete", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("FS Deleting file...");
    FS_Select_File_For_Function("[DELETE]", "FS_deletehandler"); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  server.on("/FS_icon", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(FS, "/icon.gif", "image/gif"); });
}

void FS_Directory()
{
  FS_numfiles = 0;
  FS_Filenames.clear();
  File root = FS.open("/");

  if (root)
  {
    root.rewindDirectory();
    File file = root.openNextFile();

    while (file)
    {
      fileinfo tmp;
      tmp.filename = (String(file.name()).startsWith("/") ? String(file.name()).substring(1) : file.name());
      tmp.ftype = (file.isDirectory() ? "Dir" : "File");
      if (tmp.ftype == "File")
        tmp.fsize = ConvBinUnits(file.size(), 1);
      else
        tmp.fsize = "";

      FS_Filenames.push_back(tmp);
      file = root.openNextFile();
      FS_numfiles++;
    }
    root.close();
  }
  std::sort(FS_Filenames.begin(), FS_Filenames.end(), compareFileinfo);
}

void FS_Dir(AsyncWebServerRequest *request)
{
  String Fname1, Fname2;
  int index = 0;
  FS_Directory();
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS:　Filing System Content</h3><br>";
  if (FS_numfiles > 0)
  {
    webpage += "<table class='center'>";
    webpage += "<tr><th>Type</th><th>File Name</th><th>File Size</th><th class='sp'></th><th>Type</th><th>File Name</th><th>File Size</th></tr>";
    while (index < FS_numfiles)
    {
      Fname1 = FS_Filenames[index].filename;
      Fname2 = (index + 1 < FS_numfiles) ? FS_Filenames[index + 1].filename : "";
      webpage += "<tr>";
      webpage += "<td style = 'width:5%'>" + FS_Filenames[index].ftype + "</td><td style = 'width:25%'>" + Fname1 + "</td><td style = 'width:10%'>" + FS_Filenames[index].fsize + "</td>";
      webpage += "<td class='sp'></td>";
      if (index < FS_numfiles - 1)
      {
        webpage += "<td style = 'width:5%'>" + FS_Filenames[index + 1].ftype + "</td><td style = 'width:25%'>" + Fname2 + "</td><td style = 'width:10%'>" + FS_Filenames[index + 1].fsize + "</td>";
      }
      webpage += "</tr>";
      index = index + 2;
    }
    webpage += "</table>";
    webpage += "<p style='background-color:yellow;'><b>" + FS_MessageLine + "</b></p>";
    FS_MessageLine = "";
  }
  else
  {
    webpage += "<h2>No Files Found</h2>";
  }
  webpage += HTML_Footer();
  request->send(200, "text/html", webpage);
}

void FS_UploadFileSelect()
{
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS:　Select a File to [UPLOAD] to this device</h3>";
  webpage += "<form method = 'POST' action = '/FS_handleupload' enctype='multipart/form-data'>";
  webpage += "<input type='file' name='filename'><br><br>";
  webpage += "<input type='submit' value='Upload'>";
  webpage += "</form>";
  webpage += HTML_Footer();
}

void FS_handleFileUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
  String file = filename;
  if (!index)
  {
    if (!filename.startsWith("/"))
      file = "/" + filename;

    request->_tempFile = FS.open(file, "w");

    if (!request->_tempFile)
      Serial.println("Error creating file for FS upload...");

    FS_uploadsize = 0;
    FS_start = millis();
  }

  if (request->_tempFile)
  {
    if (len)
    {
      request->_tempFile.write(data, len);
      Serial.println("Transferred : " + String(len) + " Bytes");
      FS_uploadsize = FS_uploadsize + len;
    }

    if (final)
    {
      request->_tempFile.close();
      FS_uploadtime = millis() - FS_start;
      Serial.println("FileName = " + file);
      Serial.println("FS_uploadsize = " + String(FS_uploadsize) + " Bytes");
      Serial.println("FS_uploadtime = " + String(FS_uploadtime) + " mSEC");
      request->redirect("/FS_dir");
    }
  }
}

void FS_File_Stream()
{
  SelectInput("Select a File to Stream", "FS_handlestream", "filename");
}

void FS_File_Delete()
{
  SelectInput("Select a File to Delete", "FS_handledelete", "filename");
}

void FS_Handle_File_Delete(String filename)
{ // Delete the file
  webpage = HTML_Header();
  if (!filename.startsWith("/"))
    filename = "/" + filename;
  File dataFile = FS.open(filename, "r");
  if (dataFile)
  {
    FS.remove(filename);
    webpage += "<h3>SPIFFS:　File '" + filename.substring(1) + "' has been deleted</h3>";
    webpage += "<a href='/FS_dir'>[Enter]</a><br><br>";
  }
  else
  {
    webpage += "<h3>SPIFFS:　File [ " + filename + " ] does not exist</h3>";
    webpage += "<a href='/FS_dir'>[Enter]</a><br><br>";
  }
  webpage += HTML_Footer();
}

void FS_File_Rename()
{
  FS_Directory();
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS:　Select a File to [RENAME] on this device</h3>";
  webpage += "<FORM action='/FS_renamehandler'>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>File name</th><th>New Filename</th><th>Select</th></tr>";
  int index = 0;
  while (index < FS_numfiles)
  {
    webpage += "<tr><td><input type='text' name='oldfile' style='color:blue;' value = '" + FS_Filenames[index].filename + "' readonly></td>";
    webpage += "<td><input type='text' name='newfile'></td><td><input type='radio' name='choice'></tr>";
    index++;
  }
  webpage += "</table><br>";
  webpage += "<input type='submit' value='Enter'>";
  webpage += "</form>";
  webpage += HTML_Footer();
}

void FS_Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args)
{
  String newfilename;
  webpage = HTML_Header();

  newfilename = "";
  filename = "";
  if (Args >= 3)
  {
    for (int i = 2; i < Args; i++)
    {
      if (request->arg(i - 1) != "" && request->arg(i) == "on")
      {
        filename = request->arg(i - 2);
        newfilename = request->arg(i - 1);
        break;
      }
    }
  }
  Serial.println("old filename = " + filename);
  Serial.println("new filename = " + newfilename);

  if (!filename.startsWith("/"))
    filename = "/" + filename;

  if (!newfilename.startsWith("/"))
    newfilename = "/" + newfilename;

  File CurrentFile = FS.open(filename, "r");

  if (CurrentFile && filename != "/" && newfilename != "/" && (filename != newfilename))
  {
    if (FS.rename(filename, newfilename))
    {
      filename = filename.substring(1);
      newfilename = newfilename.substring(1);
      webpage += "<h3>SPIFFS:　File '" + filename + "' has been renamed to '" + newfilename + "'</h3>";
      webpage += "<a href='/FS_dir'>[Enter]</a><br><br>";
    }
  }
  else
  {
    if (filename == "/" && newfilename == "/")
      webpage += "<h3>SPIFFS:　File was not renamed</h3>";
    else
      webpage += "<h3>SPIFFS:　New filename exists, cannot rename</h3>";
    webpage += "<a href='/FS_rename'>[Enter]</a><br><br>";
  }
  CurrentFile.close();
  webpage += HTML_Footer();
}

bool FS_notFound(AsyncWebServerRequest *request)
{ // Serial.println("FS_notFund func ... : " + request->url());

  String filename;
  if (request->url().startsWith("/FS_downloadhandler") ||
      request->url().startsWith("/FS_streamhandler") ||
      request->url().startsWith("/FS_deletehandler") ||
      request->url().startsWith("/FS_renamehandler"))
  {
    if (!request->url().startsWith("/FS_renamehandler"))
      filename = request->url().substring(request->url().indexOf("~/") + 1);

    FS_start = millis();

    if (request->url().startsWith("/FS_downloadhandler"))
    {
      Serial.println("FS Download handler started...");
      FS_MessageLine = "";
      File file = FS.open(filename, "r");
      String contentType = getContentType("download");
      AsyncWebServerResponse *response = request->beginResponse(contentType, file.size(), [file](uint8_t *buffer, size_t maxLen, size_t total) mutable -> size_t
                                                                { return file.read(buffer, maxLen); });
      response->addHeader("Server", "ESP Async Web Server");
      request->send(response);
      FS_downloadtime = millis() - FS_start;
      FS_downloadsize = FS_GetFileSize(filename);
      // request->redirect("/FS_dir");
    }

    if (request->url().startsWith("/FS_streamhandler"))
    {
      Serial.println("FS Stream handler started...");
      String ContentType = getContentType(filename);
      AsyncWebServerResponse *response = request->beginResponse(FS, filename, ContentType);
      request->send(response);
      FS_downloadsize = FS_GetFileSize(filename);
      FS_downloadtime = millis() - FS_start;
      // request->redirect("/FS_dir");
    }

    if (request->url().startsWith("/FS_deletehandler"))
    {
      Serial.println("FS Delete handler started...");
      FS_Handle_File_Delete(filename);
      request->send(200, "text/html", webpage);
    }

    if (request->url().startsWith("/FS_renamehandler"))
    {
      Serial.println("FS Rename handler started...");
      FS_Handle_File_Rename(request, filename, request->args());
      request->send(200, "text/html", webpage);
    }
    return true;
  }
  return false;
}

void FS_Handle_File_Download()
{
  String filename = "";
  int index = 0;
  FS_Directory();
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS:　Select a File to Download</h3>";
  webpage += "<table>";
  webpage += "<tr><th>File Name</th><th>File Size</th></tr>";
  while (index < FS_numfiles)
  {
    webpage += "<tr><td><a href='" + FS_Filenames[index].filename + "'></a><td>" + FS_Filenames[index].fsize + "</td></tr>";
    index++;
  }
  webpage += "</table>";
  webpage += "<p>" + FS_MessageLine + "</p>";
  webpage += HTML_Footer();
}

void FS_Select_File_For_Function(String title, String function)
{
  String Fname1, Fname2;
  int index = 0;
  FS_Directory();
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS:　Select a File to " + title + " from this device</h3>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>File Name</th><th>File Size</th><th class='sp'></th><th>File Name</th><th>File Size</th></tr>";
  while (index < FS_numfiles)
  {
    Fname1 = FS_Filenames[index].filename;
    Fname2 = (index + 1 < FS_numfiles) ? FS_Filenames[index + 1].filename : "";

    if (Fname1.startsWith("/"))
      Fname1 = Fname1.substring(1);

    if (!Fname2.isEmpty() && Fname2.startsWith("/"))
      Fname2 = Fname2.substring(1);

    webpage += "<tr>";
    webpage += "<td style='width:25%'><button><a href='" + function + "~/" + Fname1 + "'>" + Fname1 + "</a></button></td><td style = 'width:10%'>" + FS_Filenames[index].fsize + "</td>";
    webpage += "<td class='sp'></td>";

    if (index < FS_numfiles - 1)
    {
      webpage += "<td style='width:25%'><button><a href='" + function + "~/" + Fname2 + "'>" + Fname2 + "</a></button></td><td style = 'width:10%'>" + FS_Filenames[index + 1].fsize + "</td>";
    }
    webpage += "</tr>";
    index = index + 2;
  }
  webpage += "</table>";
  webpage += HTML_Footer();
}

int FS_GetFileSize(String filename)
{
  int filesize;
  File CheckFile = FS.open(filename, "r");
  filesize = CheckFile.size();
  CheckFile.close();
  return filesize;
}

String FS_StatusReport(int reportNo, int decimalPlaces)
{
  switch (reportNo)
  {
  case STREP_FS_TOTALBYTES:
    return ConvBinUnits(FS.totalBytes(), decimalPlaces);
  case STREP_FS_USEDBYTES:
    return ConvBinUnits(FS.usedBytes(), decimalPlaces);
  case STREP_FS_FREESPACE:
    return ConvBinUnits(FS.totalBytes() - FS.usedBytes(), decimalPlaces);
  }
  return String("");
}

bool FS_isExists(const String filename)
{
  return (FS.exists(filename));
}

bool FS_SettingRd(const String filename)
{
  if (!FS.exists(filename))
    return false;

  File fs = FS.open(filename, FILE_READ);
  if (!fs)
    return false;

  size_t length = fs.size();
  if (length <= 3) // at least 3bytes size
    return false;

  char buf[length + 1];
  fs.read((uint8_t *)buf, length);
  buf[length] = 0;
  fs.close();

  int x;
  int y = 0;
  int z = 0;
  for (x = 0; x < length; x++)
  {
    if (buf[x] == 0x0a || buf[x] == 0x0d)
      buf[x] = 0;
    else if (!y && x > 0 && !buf[x - 1] && buf[x])
      y = x;
    else if (!z && x > 0 && !buf[x - 1] && buf[x])
      z = x;
  }

  if (y == 0)
    return false;
  SSID = String(buf);
  SSID_PASS = String(&buf[y]);
  Serial.println("SSID        = " + SSID);
  Serial.println("SSID_PASS   = " + SSID_PASS);

  if (z == 0)
    return false;
  SERVER_NAME = String(&buf[z]);
  Serial.println("SERVER_NAME = " + SERVER_NAME);

  if (SSID == "" || SSID_PASS == "" || SERVER_NAME == "")
    return false;

  return true;
}
