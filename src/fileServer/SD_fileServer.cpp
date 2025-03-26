// *** Modified by NoRi 2025-03-18 ***
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <algorithm>
#include <vector>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// -------------------------------------------------------
void SD_flServerSetup();
void SD_Dir(AsyncWebServerRequest *request);
void SD_Directory();
void SD_UploadFileSelect();
void SD_handleFileUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);
void SD_File_Stream();
void SD_File_Delete();
void SD_Handle_File_Delete(String filename);
void SD_File_Rename();
void SD_Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args);
bool SD_notFound(AsyncWebServerRequest *request);
void SD_Handle_File_Download();
void SD_Select_File_For_Function(String title, String function);
int SD_GetFileSize(String filename);
bool SD_isExists(const String filename);
// -------------------------------------------------------
void SDdir_flserverSetup();
void SDdir_handle_chTop();
void SDdir_handle_chUp();
void SDdir_Select_Dir_For_Function(String title, String function);
void SDdir_Handle_chdir(String filename);
void SDdir_Handle_rmdir(String filename);
void SDdir_Handle_mkdir(AsyncWebServerRequest *request);
void SDdir_DirMake();
void SDdir_SelectInputDirName(String Heading, String Command, String Arg_name);
void SDdir_DirList();
void SDdir_FilesList();
bool SDdir_notFound(AsyncWebServerRequest *request);
String SD_StatusReport(int reportNo, int dp);
bool SD_Start();
bool SD_cardInfo(void);
// -------------------------------------------------------
extern void SelectInput(String Heading, String Command, String Arg_name);
extern String getContentType(String filenametype);
extern void Home();
// extern String ConvBinUnits(int bytes, int resolution);
extern String ConvBinUnits(uint64_t bytes, int resolution);
extern String EncryptionType(wifi_auth_mode_t encryptionType);
extern String HTML_Header();
extern String HTML_Footer();
// -------------------------------------------------------
extern AsyncWebServer server;
extern String webpage;
// extern bool StartupErrors;

typedef struct
{
  String filename;
  String ftype;
  String fsize;
} fileinfo;
extern bool compareFileinfo(const fileinfo &a, const fileinfo &b);
std::vector<fileinfo> SD_Filenames;

String SD_MessageLine;
int SD_start, SD_downloadtime = 1, SD_uploadtime = 1, SD_downloadsize, SD_uploadsize, SD_downloadrate, SD_uploadrate, SD_numfiles;
String SdPath = "/";
// bool SD_ENABLE = false;

bool SD_Start()
{
  if (!SD.begin())
  {
    Serial.println("ERR: SD_Start...");
    // SD_ENABLE = false;
    return false;
  }

  if (!SD_cardInfo())
  {
    // SD_ENABLE = false;
    return false;
  }

  // SD_ENABLE = true;
  return true;
}

void SD_flServerSetup()
{
  Serial.println(__FILE__);

  // ##################### DOWNLOAD HANDLER ##########################
  server.on("/SD_download", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SD_Downloading file...");
    SD_Select_File_For_Function("[DOWNLOAD]", "SD_downloadhandler"); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  // ##################### UPLOAD HANDLERS ###########################
  server.on("/SD_upload", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SD_Uploading file...");
    SD_UploadFileSelect(); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  // Set handler for '/handleupload'
  server.on("/SD_handleupload", HTTP_POST, [](AsyncWebServerRequest *request) {}, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
            { SD_handleFileUpload(request, filename, index, data, len, final); });

  // ##################### STREAM HANDLER ############################
  server.on("/SD_stream", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SD_Streaming file...");
    SD_Select_File_For_Function("[STREAM]", "SD_streamhandler"); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  // ##################### RENAME HANDLER ############################
  server.on("/SD_rename", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SD_Renaming file...");
    SD_File_Rename(); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  // ##################### DIR HANDLER ###############################
  server.on("/SD_dir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SD_File Directory...");
    SD_Dir(request); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  // ##################### DELETE HANDLER ############################
  server.on("/SD_delete", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SD_Deleting file...");
    SD_Select_File_For_Function("[DELETE]", "SD_deletehandler");
    request->send(200, "text/html", webpage); });

  // ##################### IMAGE HANDLER ############################
  server.on("/SD_icon", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SD, "/icon.gif", "image/gif"); });
}

// #############################################################################################
void SD_Dir(AsyncWebServerRequest *request)
{
  String Fname1, Fname2;
  int index = 0;
  SD_Directory();

  webpage = HTML_Header();
  webpage += "<h3>SD:　Filing System Content</h3><br>";
  if (SD_numfiles > 0)
  {
    webpage += "<table class='center'>";
    webpage += "<tr><th>Type</th><th>File Name</th><th>File Size</th><th class='sp'></th><th>Type</th><th>File Name</th><th>File Size</th></tr>";
    while (index < SD_numfiles)
    {
      Fname1 = SD_Filenames[index].filename;
      // Fname2 = SD_Filenames[index + 1].filename;
      Fname2 = (index + 1 < SD_numfiles) ? SD_Filenames[index + 1].filename : "";
      webpage += "<tr>";
      webpage += "<td style = 'width:5%'>" + SD_Filenames[index].ftype + "</td><td style = 'width:25%'>" + Fname1 + "</td><td style = 'width:10%'>" + SD_Filenames[index].fsize + "</td>";
      webpage += "<td class='sp'></td>";
      if (index < SD_numfiles - 1)
      {
        webpage += "<td style = 'width:5%'>" + SD_Filenames[index + 1].ftype + "</td><td style = 'width:25%'>" + Fname2 + "</td><td style = 'width:10%'>" + SD_Filenames[index + 1].fsize + "</td>";
      }
      webpage += "</tr>";
      index = index + 2;
    }
    webpage += "</table>";
    webpage += "<p style='background-color:yellow;'><b>" + SD_MessageLine + "</b></p>";
    SD_MessageLine = "";
  }
  else
  {
    webpage += "<h2>No Files Found</h2>";
  }
  webpage += HTML_Footer();
  request->send(200, "text/html", webpage);
}

const String SD_SYSTEM_FILE = "System Volume Information";
// #############################################################################################
void SD_Directory()
{
  SD_numfiles = 0;
  SD_Filenames.clear(); // vectorをクリア
  if (SdPath == "")
    SdPath = "/";
  Serial.println("SdPath = " + SdPath);
  File root = SD.open(SdPath, "r");

  if (root)
  {
    root.rewindDirectory();
    File file = root.openNextFile();

    while (file)
    {
      String tmp_filename = (String(file.name()).startsWith("/") ? String(file.name()).substring(1) : file.name());

      if (tmp_filename != SD_SYSTEM_FILE)
      {
        fileinfo tmp;
        tmp.filename = tmp_filename;
        tmp.ftype = (file.isDirectory() ? "Dir" : "File");
        if (tmp.ftype == "File")
          tmp.fsize = ConvBinUnits(file.size(), 1);
        else
          tmp.fsize = "";

        SD_Filenames.push_back(tmp);
        SD_numfiles++;
      }
      file = root.openNextFile();
    }
    root.close();
  }
  // ファイル名でソート
  std::sort(SD_Filenames.begin(), SD_Filenames.end(), compareFileinfo);
}

// #############################################################################################
void SD_UploadFileSelect()
{
  webpage = HTML_Header();
  webpage += "<h3>SD:　Select a File to [UPLOAD] to this device</h3>";
  webpage += "<form method = 'POST' action = '/SD_handleupload' enctype='multipart/form-data'>";
  webpage += "<input type='file' name='filename'><br><br>";
  webpage += "<input type='submit' value='Upload'>";
  webpage += "</form>";
  webpage += HTML_Footer();
}

// u64_t SD_tSize=0;
// #############################################################################################
void SD_handleFileUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
  String file = filename;
  if (!index)
  {
    if (!filename.startsWith("/"))
      file = "/" + filename;

    if (SdPath != "/")
      file = SdPath + file;

    Serial.println("filename = " + file);
    request->_tempFile = SD.open(file, "w");

    if (!request->_tempFile)
      Serial.println("Error creating file for upload...");

    SD_uploadsize = 0;
    SD_start = millis();
  }

  if (request->_tempFile)
  {
    if (len)
    {
      request->_tempFile.write(data, len); // Chunked data
      Serial.println("Transferred : " + String(len) + " Bytes");
      SD_uploadsize = SD_uploadsize + len;
    }

    if (final)
    {
      // SD_uploadsize = request->_tempFile.size();
      request->_tempFile.close();
      SD_uploadtime = millis() - SD_start;
      Serial.println("FileName = " + file);
      Serial.println("SD_uploadsize = " + String(SD_uploadsize) + " Bytes");
      Serial.println("SD_uploadtime = " + String(SD_uploadtime) + " mSEC");
      request->redirect("/SD_dir");
    }
  }
}

// #############################################################################################
void SD_File_Stream()
{
  SelectInput("[SD] Select a File to Stream", "SD_handlestream", "filename");
}

// #############################################################################################
void SD_File_Delete()
{
  SelectInput("[SD] Select a File to Delete", "SD_handledelete", "filename");
}

// #############################################################################################
void SD_Handle_File_Delete(String filename)
{ // Delete the file
  webpage = HTML_Header();
  if (!filename.startsWith("/"))
    filename = "/" + filename;

  if (SdPath != "/")
    filename = SdPath + filename;

  Serial.println("filename = " + filename);
  File dataFile = SD.open(filename, "r"); // Now read FS to see if file exists

  if (dataFile)
  { // It does so delete it
    SD.remove(filename);
    webpage += "<h3>SD:　File '" + filename.substring(1) + "' has been deleted</h3>";
    webpage += "<a href='/SD_dir'>[Enter]</a><br><br>";
  }
  else
  {
    webpage += "<h3>SD:　File [ " + filename + " ] does not exist</h3>";
    webpage += "<a href='/SD_dir'>[Enter]</a><br><br>";
  }
  webpage += HTML_Footer();
}
// #############################################################################################
void SD_File_Rename()
{ // Rename the file
  SD_Directory();
  // SDdir_FilesList();
  webpage = HTML_Header();
  webpage += "<h3>SD:　Select a Dir/File to [RENAME] on this device</h3>";
  webpage += "<FORM action='/SD_renamehandler'>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>File name</th><th>New Filename</th><th>Select</th></tr>";
  int index = 0;
  while (index < SD_numfiles)
  {
    webpage += "<tr><td><input type='text' name='oldfile' style='color:blue;' value = '" + SD_Filenames[index].filename + "' readonly></td>";
    webpage += "<td><input type='text' name='newfile'></td><td><input type='radio' name='choice'></tr>";
    index++;
  }
  webpage += "</table><br>";
  webpage += "<input type='submit' value='Enter'>";
  webpage += "</form>";
  webpage += HTML_Footer();
}
// #############################################################################################
void SD_Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args)
{ // Rename the file
  String newfilename;
  webpage = HTML_Header();

  // ---  2025-03-20 bugfix by NoRi -----
  // for (int i = 0; i < Args; i++)
  // {
  //   if (request->arg(i) != "" && request->arg(i + 1) == "on")
  //   {
  //     filename = request->arg(i - 1);
  //     newfilename = request->arg(i);
  //   }
  // }
  // ---------------------------------------
  filename = "";
  newfilename = "";
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
  //------------------------------------------------------

  if (!filename.startsWith("/"))
    filename = "/" + filename;

  if (!newfilename.startsWith("/"))
    newfilename = "/" + newfilename;

  if (SdPath != "/")
  {
    filename = SdPath + filename;
    newfilename = SdPath + newfilename;
  }
  Serial.println("filename = " + filename);
  Serial.println("newfilename = " + newfilename);
  File CurrentFile = SD.open(filename, "r");

  if (CurrentFile && filename != "/" && newfilename != "/" && (filename != newfilename))
  { // It does so rename it, ignore if no entry made, or Newfile name exists already
    if (SD.rename(filename, newfilename))
    {
      filename = filename.substring(1);
      newfilename = newfilename.substring(1);
      webpage += "<h3>SD:　File '" + filename + "' has been renamed to '" + newfilename + "'</h3>";
      webpage += "<a href='/SD_dir'>[Enter]</a><br><br>";
    }
  }
  else
  {
    if (filename == "/" && newfilename == "/")
      webpage += "<h3>SD:　File was not renamed</h3>";
    else
      webpage += "<h3>SD:　New filename exists, cannot rename</h3>";
    webpage += "<a href='/SD_rename'>[Enter]</a><br><br>";
  }
  CurrentFile.close();
  webpage += HTML_Footer();
}

// #############################################################################################
bool SD_notFound(AsyncWebServerRequest *request)
{ // Serial.println("SD_notFund func ... : " + request->url());

  String filename;
  if (request->url().startsWith("/SD_downloadhandler") ||
      request->url().startsWith("/SD_streamhandler") ||
      request->url().startsWith("/SD_deletehandler") ||
      request->url().startsWith("/SD_renamehandler"))
  {
    // Now get the filename and handle the request for 'delete' or 'download' or 'stream' functions
    if (!request->url().startsWith("/SD_renamehandler"))
      filename = request->url().substring(request->url().indexOf("~/") + 1);

    SD_start = millis();

    if (request->url().startsWith("/SD_downloadhandler"))
    {
      Serial.println("SD_Download handler started...");
      SD_MessageLine = "";

      String filename_tmp = filename;
      if (SdPath != "/")
        filename_tmp = SdPath + filename_tmp;

      Serial.println("filename_tmp = " + filename_tmp);
      File file = SD.open(filename_tmp, "r");
      String contentType = getContentType("download");
      AsyncWebServerResponse *response = request->beginResponse(contentType, file.size(), [file](uint8_t *buffer, size_t maxLen, size_t total) mutable -> size_t
                                                                { return file.read(buffer, maxLen); });
      response->addHeader("Server", "ESP Async Web Server");
      request->send(response);
      SD_downloadtime = millis() - SD_start;
      SD_downloadsize = SD_GetFileSize(filename);
      // request->redirect("/SD_dir");
    }

    if (request->url().startsWith("/SD_streamhandler"))
    {
      Serial.println("SD_Stream handler started...");
      String filename_tmp = filename;
      if (SdPath != "/")
        filename_tmp = SdPath + filename_tmp;

      Serial.println("filename_tmp = " + filename_tmp);
      String ContentType = getContentType(filename);
      AsyncWebServerResponse *response = request->beginResponse(SD, filename_tmp, ContentType);
      request->send(response);
      SD_downloadsize = SD_GetFileSize(filename);
      SD_downloadtime = millis() - SD_start;
      // request->redirect("/SD_dir");
    }
    if (request->url().startsWith("/SD_deletehandler"))
    {
      Serial.println("SD_Delete handler started...");
      // Serial.println("filename = " + filename);
      SD_Handle_File_Delete(filename);
      request->send(200, "text/html", webpage);
    }
    if (request->url().startsWith("/SD_renamehandler"))
    {
      SD_Handle_File_Rename(request, filename, request->args());
      request->send(200, "text/html", webpage);
    }

    return true;
  }
  return false;
}

// #############################################################################################
void SD_Handle_File_Download()
{
  String filename = "";
  int index = 0;
  // SD_Directory();
  SDdir_FilesList();

  webpage = HTML_Header();
  webpage += "<h3>SD:　Select a File to Download</h3>";
  webpage += "<table>";
  webpage += "<tr><th>File Name</th><th>File Size</th></tr>";
  while (index < SD_numfiles)
  {
    webpage += "<tr><td><a href='" + SD_Filenames[index].filename + "'></a><td>" + SD_Filenames[index].fsize + "</td></tr>";
    index++;
  }
  webpage += "</table>";
  webpage += "<p>" + SD_MessageLine + "</p>";
  webpage += HTML_Footer();
}

// #############################################################################################
void SD_Select_File_For_Function(String title, String function)
{
  String Fname1, Fname2;
  int index = 0;
  SDdir_FilesList();
  webpage = HTML_Header();
  webpage += "<h3>SD:　Select a File to " + title + " from this device</h3>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>File Name</th><th>File Size</th><th class='sp'></th><th>File Name</th><th>File Size</th></tr>";
  while (index < SD_numfiles)
  {
    Fname1 = SD_Filenames[index].filename;
    Fname2 = (index + 1 < SD_numfiles) ? SD_Filenames[index + 1].filename : "";

    if (Fname1.startsWith("/"))
      Fname1 = Fname1.substring(1);

    // if (Fname2.startsWith("/"))
    if (!Fname2.isEmpty() && Fname2.startsWith("/"))
      Fname2 = Fname2.substring(1);

    webpage += "<tr>";
    webpage += "<td style='width:25%'><button><a href='" + function + "~/" + Fname1 + "'>" + Fname1 + "</a></button></td><td style = 'width:10%'>" + SD_Filenames[index].fsize + "</td>";
    webpage += "<td class='sp'></td>";

    if (index < SD_numfiles - 1)
    {
      webpage += "<td style='width:25%'><button><a href='" + function + "~/" + Fname2 + "'>" + Fname2 + "</a></button></td><td style = 'width:10%'>" + SD_Filenames[index + 1].fsize + "</td>";
    }
    webpage += "</tr>";
    index = index + 2;
  }
  webpage += "</table>";
  webpage += HTML_Footer();
}

// #############################################################################################
int SD_GetFileSize(String filename)
{
  int filesize;
  if (SdPath != "/")
    filename = SdPath + filename;

  // Serial.println("filename = " + filename);
  File CheckFile = SD.open(filename, "r");
  filesize = CheckFile.size();
  CheckFile.close();
  return filesize;
}

void SDdir_flserverSetup()
{

  // ##################### CHDIR HANDLER ############################
  server.on("/SDdir_chdir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SDdir_chdir...");
    SDdir_Select_Dir_For_Function("[CHDIR]", "SDdir_chdirhandler");
    request->send(200, "text/html", webpage); });

  // ##################### MKDIR HANDLER ############################
  server.on("/SDdir_mkdir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SDdir_mkdir ...");
    SDdir_DirMake();
    request->send(200, "text/html", webpage); });

  // ##################### RMDIR HANDLER #############################
  server.on("/SDdir_rmdir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SDdir_rmdir...");
    SDdir_Select_Dir_For_Function("[RMDIR]", "SDdir_rmdirhandler");
    request->send(200, "text/html", webpage); });

  // ######################  chTop ################################
  server.on("/SDdir_chTop", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("change SdPath to Root directory...");
    SDdir_handle_chTop(); 
    request->send(200, "text/html", webpage); });

  // ######################  chUp ################################
  server.on("/SDdir_chUp", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("change SdPath to Up directory...");
    SDdir_handle_chUp(); 
    request->send(200, "text/html", webpage); });
}

// #############################################################################################
void SDdir_handle_chTop()
{ // change SD Path to Root directory

  SdPath = String("/");
  // Serial.println("change SdPath to Root Directory");

  webpage = HTML_Header();
  webpage += "<h3>Change SD Path to Root Directory</h3>";
  webpage += "<a href='/SD_dir'>[Enter]</a><br><br>";
  webpage += HTML_Footer();
}

// #############################################################################################
void SDdir_handle_chUp()
{ // change SD Path to Up directory

  String upPath = String("/");

  if (SdPath != "/")
  {
    int i = String(SdPath).lastIndexOf("/");
    upPath = SdPath.substring(0, i);
  }
  SdPath = upPath;
  if (SdPath == "")
    SdPath = String("/");

  Serial.println("SdPath = " + SdPath);
  // Serial.println("change SdPath to Up Directory");

  webpage = HTML_Header();
  webpage += "<h3>Change SD Path to Up Directory</h3>";
  webpage += "<a href='/SD_dir'>[Enter]</a><br><br>";
  webpage += HTML_Footer();
}

// #############################################################################################
void SDdir_Handle_chdir(String filename)
{ // chdri
  Serial.println("filename = " + filename);

  webpage = HTML_Header();
  if (!filename.startsWith("/"))
    filename = "/" + filename;

  if (SdPath != "/")
    filename = SdPath + filename;
  Serial.println("filename = " + filename);

  if (SD.exists(filename))
  { // success
    SdPath = filename;
    Serial.println("SdPath = " + SdPath);
    webpage += "<h3>Directory '" + SdPath + "' has been changed</h3>";
    webpage += "<a href='/SD_dir'>[Enter]</a><br><br>";
  }
  else
  {
    webpage += "<h3>Directory [ " + filename + " ] is NOT changed successfully</h3>";
    webpage += "<a href='/SD_dir'>[Enter]</a><br><br>";
  }
  webpage += HTML_Footer();
}

// #############################################################################################
void SDdir_Handle_rmdir(String filename)
{ // rmdir
  webpage = HTML_Header();
  if (!filename.startsWith("/"))
    filename = "/" + filename;

  if (SdPath != "/")
    filename = SdPath + filename;
  Serial.println("filename = " + filename);

  if (SD.rmdir(filename))
  { // success
    webpage += "<h3>Directory '" + filename.substring(1) + "' has been removed</h3>";
    webpage += "<a href='/SD_dir'>[Enter]</a><br><br>";
  }
  else
  {
    webpage += "<h3>Directory [ " + filename + " ] is NOT removed successfully</h3>";
    webpage += "<a href='/SD_dir'>[Enter]</a><br><br>";
  }
  webpage += HTML_Footer();
}

// #############################################################################################
void SDdir_Handle_mkdir(AsyncWebServerRequest *request)
{ // Dir Make

  webpage = HTML_Header();
  String filename = request->arg("filename");

  if (!filename.startsWith("/"))
    filename = "/" + filename;

  if (SdPath != "/")
    filename = SdPath + filename;
  Serial.println("filename = " + filename);

  if (SD.mkdir(filename))
  { // success
    webpage += "<h3>Directory '" + filename + "' has been created</h3>";
    webpage += "<a href='/SD_dir'>[Enter]</a><br><br>";
  }
  else
  {
    webpage += "<h3>Directory [ " + filename + " ] is NOT created successfully</h3>";
    webpage += "<a href='/SD_dir'>[Enter]</a><br><br>";
  }
  webpage += HTML_Footer();
}

// #############################################################################################
void SDdir_DirMake()
{
  SDdir_SelectInputDirName("Make New Directory", "SDdir_mkdirhandler", "filename");
}

// #############################################################################################
void SDdir_SelectInputDirName(String Heading, String Command, String Arg_name)
{
  webpage = HTML_Header();
  webpage += "<h3>" + Heading + "</h3>";
  webpage += "<form  action='/" + Command + "'>";
  webpage += "DirName: <input type='text' name='" + Arg_name + "'><br><br>";
  webpage += "<input type='submit' value='Enter'>";
  webpage += "</form>";
  webpage += HTML_Footer();
}

// #############################################################################################
void SDdir_Select_Dir_For_Function(String title, String function)
{
  String Fname1, Fname2;
  int index = 0;
  SDdir_DirList(); // Get a Dir list
  webpage = HTML_Header();
  webpage += "<h3>Select a Directory to " + title + " from this device</h3>";
  webpage += "<table class='center'>";
  webpage += "<tr> <th>Directory Name</th> <th>Directory Name</th> </tr>";

  while (index < SD_numfiles)
  {
    Fname1 = SD_Filenames[index].filename;
    // Fname2 = SD_Filenames[index + 1].filename;
    Fname2 = (index + 1 < SD_numfiles) ? SD_Filenames[index + 1].filename : ""; // 10. 変更点
    if (Fname1.startsWith("/"))
      Fname1 = Fname1.substring(1);
    // if (Fname2.startsWith("/"))
    //   Fname2 = Fname2.substring(1);

    webpage += "<tr>";
    webpage += "<td style='width:25%'><button><a href='" + function + "~/" + Fname1 + "'>" + Fname1 + "</a></button></td>";

    if (index < SD_numfiles - 1)
    {
      webpage += "<td style='width:25%'><button><a href='" + function + "~/" + Fname2 + "'>" + Fname2 + "</a></button></td>";
    }
    webpage += "</tr>";
    index = index + 2;
  }
  webpage += "</table>";
  webpage += HTML_Footer();
}

// #############################################################################################
//  File含まない。Dirのみの表示   .. by NoRi
void SDdir_DirList()
{
  SD_numfiles = 0;
  SD_Filenames.clear(); // vectorをクリア

  if (SdPath == "")
    SdPath = "/";
  Serial.println("SdPath = " + SdPath);
  File root = SD.open(SdPath, "r");

  if (root)
  {
    root.rewindDirectory();
    File file = root.openNextFile();
    while (file)
    {
      String tmp_filename = (String(file.name()).startsWith("/") ? String(file.name()).substring(1) : file.name());

      if (file.isDirectory() && (tmp_filename != SD_SYSTEM_FILE))
      {
        fileinfo tmp;
        tmp.filename = tmp_filename;
        // tmp.ftype = (file.isDirectory() ? "Dir" : "File");
        // tmp.fsize = ConvBinUnits(file.size(), 1);
        tmp.ftype = "Dir";
        tmp.fsize = "";

        SD_Filenames.push_back(tmp);
        SD_numfiles++;
      }
      file = root.openNextFile();
    }
    root.close();
  }
  // ファイル名でソート
  std::sort(SD_Filenames.begin(), SD_Filenames.end(), compareFileinfo);
}

// #############################################################################################
// //  Dirを含まない。fileのみ  .. by NoRi
void SDdir_FilesList()
{
  SD_numfiles = 0;
  SD_Filenames.clear(); // vectorをクリア
  if (SdPath == "")
    SdPath = "/";
  // Serial.println("SdPath = " + SdPath);
  File root = SD.open(SdPath, "r");

  if (root)
  {
    root.rewindDirectory();
    File file = root.openNextFile();

    while (file)
    {
      if (!file.isDirectory())
      {
        fileinfo tmp;
        tmp.filename = (String(file.name()).startsWith("/") ? String(file.name()).substring(1) : file.name());
        tmp.ftype = "File";
        tmp.fsize = ConvBinUnits(file.size(), 1);

        SD_Filenames.push_back(tmp);
        SD_numfiles++;
      }
      file = root.openNextFile();
    }
    root.close();
  }
  // ファイル名でソート
  std::sort(SD_Filenames.begin(), SD_Filenames.end(), compareFileinfo);
}

// #############################################################################################
bool SDdir_notFound(AsyncWebServerRequest *request)
{
  String filename;
  if (request->url().startsWith("/SDdir_chdirhandler") ||
      request->url().startsWith("/SDdir_mkdirhandler") ||
      request->url().startsWith("/SDdir_rmdirhandler"))
  {
    filename = request->url().substring(request->url().indexOf("~/") + 1);

    if (request->url().startsWith("/SDdir_chdirhandler"))
    {
      Serial.println("SDdir_chdir handler started...");
      SDdir_Handle_chdir(filename);
      request->send(200, "text/html", webpage);
      return true;
    }

    if (request->url().startsWith("/SDdir_mkdirhandler"))
    {
      Serial.println("SDdir_mkdir handler started...");
      SDdir_Handle_mkdir(request);
      request->send(200, "text/html", webpage);
      return true;
    }

    if (request->url().startsWith("/SDdir_rmdirhandler"))
    {
      Serial.println("SDdir_rmdir handler started...");
      SDdir_Handle_rmdir(filename);
      request->send(200, "text/html", webpage);
      return true;
    }
  }
  return false;
}

// --------------------------------------------------------------------
#define STREP_SD_TOTALBYTES 21
#define STREP_SD_USEDBYTES 22
#define STREP_SD_FREESPACE 23
#define STREP_SD_CARDTYPE 24

String SD_StatusReport(int reportNo, int dp)
{ // dp:deciamlPoint小数点以下の桁数
  sdcard_type_t cardType = SD.cardType();
  const String cType[] = {"NONE", "MMC", "SD", "SDHC", "UNKNOWN"};

  switch (reportNo)
  {
  case STREP_SD_TOTALBYTES:
    return ConvBinUnits(SD.totalBytes(), dp);
  case STREP_SD_USEDBYTES:
    return ConvBinUnits(SD.usedBytes(), dp);
  case STREP_SD_FREESPACE:
    return ConvBinUnits(SD.totalBytes() - SD.usedBytes(), dp);
  case STREP_SD_CARDTYPE:
    return cType[cardType];
  default:
    return String("");
  }
}

bool SD_cardInfo(void)
{
  sdcard_type_t cardType = SD.cardType();
  switch (cardType)
  {
  case CARD_MMC:
    Serial.println("MMC detected");
    break;
  case CARD_SD:
    Serial.println("SD detected");
    break;
  case CARD_SDHC:
    Serial.println("SDHC detected");
    break;
  case CARD_NONE:
    Serial.println("ERR: No SD card attached");
    // SD_ENABLE = false;
    return false;
  case CARD_UNKNOWN:
    Serial.println("ERR: SD card unknown Type");
    // SD_ENABLE = false;
    return false;
  default:
    Serial.println("ERR: SD cardType is default Type");
    // SD_ENABLE = false;
    return false;
  }

  Serial.println("SD_totalbytes = " + SD_StatusReport(STREP_SD_TOTALBYTES, 1));
  Serial.println("SD_usedbytes  = " + SD_StatusReport(STREP_SD_USEDBYTES, 1));
  Serial.println("SD_freespace  = " + SD_StatusReport(STREP_SD_FREESPACE, 1));
  Serial.println("SD_CardType   = " + SD_StatusReport(STREP_SD_CARDTYPE, 1));

  // SD_ENABLE = true;
  return true;
}

bool SD_isExists(const String filename)
{
  return (SD.exists(filename));
}

bool SD_SettingRd(const String filename);
const String WIFI_TXT = "/wifi.txt";
extern String SERVER_NAME;
extern String SSID;
extern String SSID_PASS;

bool SD_SettingRd(const String filename)
{
  if (!SD.exists(filename))
    return false;

  File fs = SD.open(filename, FILE_READ);
  if (!fs)
    return false;

  size_t sz = fs.size();
  if (sz <= 3)  // at least 3bytes size 
    return false;

  char buf[sz + 1];
  fs.read((uint8_t *)buf, sz);
  buf[sz] = 0;
  fs.close();

  int y = 0;
  int z = 0;
  for (int x = 0; x < sz; x++)
  {
    if (buf[x] == 0x0a || buf[x] == 0x0d)
      buf[x] = 0;
    else if (!y && x > 0 && !buf[x - 1] && buf[x])
      y = x;
    else if (!z && x > 0 && !buf[x - 1] && buf[x])
      z = x;
  }

  SSID = String(buf);
  SSID_PASS = String(&buf[y]);
  SERVER_NAME = String(&buf[z]);
  if (SSID == "" || SSID_PASS == "" || SERVER_NAME == "")
    return false;

  Serial.println("SSID        = " + SSID);
  Serial.println("SSID_PASS   = " + SSID_PASS);
  Serial.println("SERVER_NAME = " + SERVER_NAME);
  return true;
}
