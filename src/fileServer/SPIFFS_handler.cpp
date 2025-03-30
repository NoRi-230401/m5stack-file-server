// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-01
// -------------------------------------------------------
// SPIFFS_handler.cpp
// *******************************************************
#include "fileServer.h"
// -------------------------------------------------------
void SPIFFS_flServerSetup();
void SPIFFS_Dir(AsyncWebServerRequest *request);
void SPIFFS_Directory();
void SPIFFS_UploadFileSelect();
void SPIFFS_handleFileUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);
void SPIFFS_Handle_File_Delete(String filename);
void SPIFFS_File_Rename();
void SPIFFS_Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args);
bool SPIFFS_notFound(AsyncWebServerRequest *request);
void SPIFFS_Handle_File_Download();
void SPIFFS_Select_File_For_Function(String title, String function);
uint64_t SPIFFS_GetFileSize(String filename);
// -------------------------------------------------------
// String SPIFFS_StatusReport(int reportNo, int decimalPlaces);
bool SPIFFS_isExists(const String filename);
bool SPIFFS_Start();
bool SPIFFS_SettingRd(const String filename);
// -------------------------------------------------------
extern AsyncWebServer server;
extern String webpage;
std::vector<fileinfo> SPIFFS_Filenames;
uint32_t SPIFFS_startTime, SPIFFS_downloadTime = 1, SPIFFS_uploadTime = 1;
uint64_t SPIFFS_downloadSize, SPIFFS_uploadSize;
uint32_t SPIFFS_numfiles;

bool SPIFFS_Start()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("ERR: SPIFFS begin erro...");
    return false;
  }
  return true;
}

void SPIFFS_flServerSetup()
{
  Serial.println(__FILE__);

  server.on("/SPIFFS_download", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SPIFFS Downloading file...");
    SPIFFS_Select_File_For_Function("[DOWNLOAD]", "SPIFFS_downloadhandler");
    request->send(200, "text/html", webpage); });

  server.on("/SPIFFS_upload", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SPIFFS Uploading file...");
    SPIFFS_UploadFileSelect();
    request->send(200, "text/html", webpage); });

  server.on("/SPIFFS_handleupload", HTTP_POST, [](AsyncWebServerRequest *request) {}, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
            { SPIFFS_handleFileUpload(request, filename, index, data, len, final); });

  server.on("/SPIFFS_stream", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SPIFFS Streaming file...");
    SPIFFS_Select_File_For_Function("[STREAM]", "SPIFFS_streamhandler");
    request->send(200, "text/html", webpage); });

  server.on("/SPIFFS_rename", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SPIFFS Renaming file...");
    SPIFFS_File_Rename();
    request->send(200, "text/html", webpage); });

  server.on("/SPIFFS_dir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SPIFFS File Directory...");
    SPIFFS_Dir(request);
    request->send(200, "text/html", webpage); });

  server.on("/SPIFFS_delete", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SPIFFS Deleting file...");
    SPIFFS_Select_File_For_Function("[DELETE]", "SPIFFS_deletehandler");
    request->send(200, "text/html", webpage); });

  server.on("/SPIFFS_icon", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, ICON_FILE, "image/gif"); });
}

void SPIFFS_Directory()
{
  SPIFFS_numfiles = 0;
  SPIFFS_Filenames.clear();
  File root = SPIFFS.open("/");

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
        tmp.fsize = ConvBytesUnits(file.size(), 1);
      else
        tmp.fsize = "";

      SPIFFS_Filenames.push_back(tmp);
      file = root.openNextFile();
      SPIFFS_numfiles++;
    }
    root.close();
  }
  std::sort(SPIFFS_Filenames.begin(), SPIFFS_Filenames.end(), compareFileinfo);
}

void SPIFFS_Dir(AsyncWebServerRequest *request)
{
  String Fname1, Fname2;
  int index = 0;
  SPIFFS_Directory();
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS:　Filing System Content</h3><br>";
  if (SPIFFS_numfiles > 0)
  {
    webpage += "<table class='center'>";
    webpage += "<tr><th>Type</th><th>File Name</th><th>File Size</th><th class='sp'></th><th>Type</th><th>File Name</th><th>File Size</th></tr>";
    while (index < SPIFFS_numfiles)
    {
      Fname1 = SPIFFS_Filenames[index].filename;
      Fname2 = (index + 1 < SPIFFS_numfiles) ? SPIFFS_Filenames[index + 1].filename : "";
      webpage += "<tr>";
      webpage += "<td style = 'width:5%'>" + SPIFFS_Filenames[index].ftype + "</td><td style = 'width:25%'>" + Fname1 + "</td><td style = 'width:10%'>" + SPIFFS_Filenames[index].fsize + "</td>";
      webpage += "<td class='sp'></td>";
      if (index < SPIFFS_numfiles - 1)
      {
        webpage += "<td style = 'width:5%'>" + SPIFFS_Filenames[index + 1].ftype + "</td><td style = 'width:25%'>" + Fname2 + "</td><td style = 'width:10%'>" + SPIFFS_Filenames[index + 1].fsize + "</td>";
      }
      webpage += "</tr>";
      index = index + 2;
    }
    webpage += "</table>";
  }
  else
  {
    webpage += "<h2>No Files Found</h2>";
  }
  webpage += HTML_Footer();
  request->send(200, "text/html", webpage);
}

void SPIFFS_UploadFileSelect()
{
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS:　Select a File to [UPLOAD] to this device</h3>";
  webpage += "<form method = 'POST' action = '/SPIFFS_handleupload' enctype='multipart/form-data'>";
  webpage += "<input type='file' name='filename'><br><br>";
  webpage += "<input type='submit' value='Upload'>";
  webpage += "</form>";
  webpage += HTML_Footer();
}

void SPIFFS_handleFileUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
  String file = filename;
  if (!index)
  {
    if (!filename.startsWith("/"))
      file = "/" + filename;

    request->_tempFile = SPIFFS.open(file, "w");

    if (!request->_tempFile)
      Serial.println("Error creating file for SPIFFS upload...");

    SPIFFS_uploadSize = 0;
    SPIFFS_startTime = millis();
  }

  if (request->_tempFile)
  {
    if (len)
    {
      request->_tempFile.write(data, len);
      Serial.println("Transferred : " + String(len) + " Bytes");
      SPIFFS_uploadSize = SPIFFS_uploadSize + len;
    }

    if (final)
    {
      request->_tempFile.close();
      SPIFFS_uploadTime = millis() - SPIFFS_startTime;
      Serial.println("FileName = " + file);
      Serial.println("SPIFFS_uploadSize = " + String(SPIFFS_uploadSize) + " Bytes");
      Serial.println("SPIFFS_uploadTime = " + String(SPIFFS_uploadTime) + " mSEC");
      request->redirect("/SPIFFS_dir");
    }
  }
}

void SPIFFS_Handle_File_Delete(String filename)
{ // Delete the file
  webpage = HTML_Header();
  if (!filename.startsWith("/"))
    filename = "/" + filename;
  File dataFile = SPIFFS.open(filename, "r");
  if (dataFile)
  {
    SPIFFS.remove(filename);
    webpage += "<h3>SPIFFS:　File '" + filename.substring(1) + "' has been deleted</h3>";
    webpage += "<a href='/SPIFFS_dir'>[Enter]</a><br><br>";
  }
  else
  {
    webpage += "<h3>SPIFFS:　File [ " + filename + " ] does not exist</h3>";
    webpage += "<a href='/SPIFFS_dir'>[Enter]</a><br><br>";
  }
  webpage += HTML_Footer();
}

void SPIFFS_File_Rename()
{
  SPIFFS_Directory();
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS:　Select a File to [RENAME] on this device</h3>";
  webpage += "<FORM action='/SPIFFS_renamehandler'>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>File name</th><th>New Filename</th><th>Select</th></tr>";
  int index = 0;
  while (index < SPIFFS_numfiles)
  {
    webpage += "<tr><td><input type='text' name='oldfile' style='color:blue;' value = '" + SPIFFS_Filenames[index].filename + "' readonly></td>";
    webpage += "<td><input type='text' name='newfile'></td><td><input type='radio' name='choice'></tr>";
    index++;
  }
  webpage += "</table><br>";
  webpage += "<input type='submit' value='Enter'>";
  webpage += "</form>";
  webpage += HTML_Footer();
}

void SPIFFS_Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args)
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

  File CurrentFile = SPIFFS.open(filename, "r");

  if (CurrentFile && filename != "/" && newfilename != "/" && (filename != newfilename))
  {
    if (SPIFFS.rename(filename, newfilename))
    {
      filename = filename.substring(1);
      newfilename = newfilename.substring(1);
      webpage += "<h3>SPIFFS:　File '" + filename + "' has been renamed to '" + newfilename + "'</h3>";
      webpage += "<a href='/SPIFFS_dir'>[Enter]</a><br><br>";
    }
  }
  else
  {
    if (filename == "/" && newfilename == "/")
      webpage += "<h3>SPIFFS:　File was not renamed</h3>";
    else
      webpage += "<h3>SPIFFS:　New filename exists, cannot rename</h3>";
    webpage += "<a href='/SPIFFS_rename'>[Enter]</a><br><br>";
  }
  CurrentFile.close();
  webpage += HTML_Footer();
}

bool SPIFFS_notFound(AsyncWebServerRequest *request)
{ // Serial.println("SPIFFS_notFund func ... : " + request->url());

  String filename;
  if (request->url().startsWith("/SPIFFS_downloadhandler") ||
      request->url().startsWith("/SPIFFS_streamhandler") ||
      request->url().startsWith("/SPIFFS_deletehandler") ||
      request->url().startsWith("/SPIFFS_renamehandler"))
  {
    if (!request->url().startsWith("/SPIFFS_renamehandler"))
      filename = request->url().substring(request->url().indexOf("~/") + 1);

    SPIFFS_startTime = millis();

    if (request->url().startsWith("/SPIFFS_downloadhandler"))
    {
      Serial.println("SPIFFS Download handler started...");
      File file = SPIFFS.open(filename, "r");
      String contentType = getContentType("download");
      AsyncWebServerResponse *response = request->beginResponse(contentType, file.size(), [file](uint8_t *buffer, size_t maxLen, size_t total) mutable -> size_t
                                                                { return file.read(buffer, maxLen); });
      response->addHeader("Server", "ESP Async Web Server");
      request->send(response);
      SPIFFS_downloadTime = millis() - SPIFFS_startTime;
      SPIFFS_downloadSize = SPIFFS_GetFileSize(filename);
      Serial.println("SPIFFS download handler done...");
    }

    if (request->url().startsWith("/SPIFFS_streamhandler"))
    {
      Serial.println("SPIFFS Stream handler started...");
      String ContentType = getContentType(filename);
      AsyncWebServerResponse *response = request->beginResponse(SPIFFS, filename, ContentType);
      request->send(response);
      SPIFFS_downloadSize = SPIFFS_GetFileSize(filename);
      SPIFFS_downloadTime = millis() - SPIFFS_startTime;
    }

    if (request->url().startsWith("/SPIFFS_deletehandler"))
    {
      Serial.println("SPIFFS Delete handler started...");
      SPIFFS_Handle_File_Delete(filename);
      request->send(200, "text/html", webpage);
    }

    if (request->url().startsWith("/SPIFFS_renamehandler"))
    {
      Serial.println("SPIFFS Rename handler started...");
      SPIFFS_Handle_File_Rename(request, filename, request->args());
      request->send(200, "text/html", webpage);
    }
    return true;
  }
  return false;
}

void SPIFFS_Handle_File_Download()
{
  String filename = "";
  int index = 0;
  SPIFFS_Directory();
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS:　Select a File to Download</h3>";
  webpage += "<table>";
  webpage += "<tr><th>File Name</th><th>File Size</th></tr>";
  while (index < SPIFFS_numfiles)
  {
    webpage += "<tr><td><a href='" + SPIFFS_Filenames[index].filename + "'></a><td>" + SPIFFS_Filenames[index].fsize + "</td></tr>";
    index++;
  }
  webpage += "</table>";
  webpage += HTML_Footer();
}

void SPIFFS_Select_File_For_Function(String title, String function)
{
  String Fname1, Fname2;
  int index = 0;
  SPIFFS_Directory();
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS:　Select a File to " + title + " from this device</h3>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>File Name</th><th>File Size</th><th class='sp'></th><th>File Name</th><th>File Size</th></tr>";
  while (index < SPIFFS_numfiles)
  {
    Fname1 = SPIFFS_Filenames[index].filename;
    Fname2 = (index + 1 < SPIFFS_numfiles) ? SPIFFS_Filenames[index + 1].filename : "";

    if (Fname1.startsWith("/"))
      Fname1 = Fname1.substring(1);

    if (!Fname2.isEmpty() && Fname2.startsWith("/"))
      Fname2 = Fname2.substring(1);

    webpage += "<tr>";
    webpage += "<td style='width:25%'><button><a href='" + function + "~/" + Fname1 + "'>" + Fname1 + "</a></button></td><td style = 'width:10%'>" + SPIFFS_Filenames[index].fsize + "</td>";
    webpage += "<td class='sp'></td>";

    if (index < SPIFFS_numfiles - 1)
    {
      webpage += "<td style='width:25%'><button><a href='" + function + "~/" + Fname2 + "'>" + Fname2 + "</a></button></td><td style = 'width:10%'>" + SPIFFS_Filenames[index + 1].fsize + "</td>";
    }
    webpage += "</tr>";
    index = index + 2;
  }
  webpage += "</table>";
  webpage += HTML_Footer();
}

uint64_t SPIFFS_GetFileSize(String filename)
{
  uint64_t filesize;
  File CheckFile = SPIFFS.open(filename, "r");
  filesize = (uint64_t)CheckFile.size();
  CheckFile.close();
  return filesize;
}

bool SPIFFS_isExists(const String filename)
{
  return (SPIFFS.exists(filename));
}

bool SPIFFS_SettingRd(const String filename)
{
  if (!SPIFFS.exists(filename))
    return false;

  File fs = SPIFFS.open(filename, FILE_READ);
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
