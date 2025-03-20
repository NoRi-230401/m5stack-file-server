// *** Modified by NoRi 2025-03-18 ***
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
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
String SD_totalBytes(int res);
String SD_usedBytes(int res);
String SD_freeSpace(int res);

extern void SelectInput(String Heading, String Command, String Arg_name);
extern String getContentType(String filenametype);
extern void Home();
// extern void LogOut();
extern String ConvBinUnits(int bytes, int resolution);
extern String EncryptionType(wifi_auth_mode_t encryptionType);
extern String HTML_Header();
extern String HTML_Footer();
// -------------------------------------------------------

void SDdir_flserverSetup();
// void FilesDirList();
void handle_fileSystem(AsyncWebServerRequest *request);
void handle_root_sd();
void Select_Dir_For_Function(String title, String function);
void Handle_chdir(String filename);
void Handle_rmdir(String filename);
void Handle_mkdir(AsyncWebServerRequest *request);
void Dir_Make();
void SelectInputDirName(String Heading, String Command, String Arg_name);
void DirsList();
void FilesList();

const String FLS_NAME[] = {"SD", "SPIFFS"};
int isSPIFFS = 1;
int numDirs;
String SdPath = "/";
// -------------------------------------------------------

extern AsyncWebServer server;

typedef struct
{
  String filename;
  String ftype;
  String fsize;
} fileinfo;

extern String webpage;
String SD_MessageLine;
fileinfo SD_Filenames[200]; // Enough for most purposes!
extern bool StartupErrors;
int SD_start, SD_downloadtime = 1, SD_uploadtime = 1, SD_downloadsize, SD_uploadsize, SD_downloadrate, SD_uploadrate, SD_numfiles;

void SD_flServerSetup()
{
  Serial.println(__FILE__);

  // if (!SD.begin(SD_CARD_SELECT_PIN))
  if (!SD.begin())
  {
    Serial.println("Error preparing Filing System...");
    StartupErrors = true;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
  }

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
      Fname2 = SD_Filenames[index + 1].filename;
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

// #############################################################################################
void SD_Directory()
{
  SD_numfiles = 0; // Reset number of FS files counter
  File root = SD.open("/");
  if (root)
  {
    root.rewindDirectory();
    File file = root.openNextFile();
    while (file)
    { // Now get all the filenames, file types and sizes
      SD_Filenames[SD_numfiles].filename = (String(file.name()).startsWith("/") ? String(file.name()).substring(1) : file.name());
      SD_Filenames[SD_numfiles].ftype = (file.isDirectory() ? "Dir" : "File");
      SD_Filenames[SD_numfiles].fsize = ConvBinUnits(file.size(), 1);
      file = root.openNextFile();
      SD_numfiles++;
    }
    root.close();
  }
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
  webpage = HTML_Header();
  webpage += "<h3>SD:　Select a File to [RENAME] on this device</h3>";
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
//  Not found handler is also the handler for 'delete', 'download' and 'stream' functions
bool SD_notFound(AsyncWebServerRequest *request)
{ // Process selected file types
  Serial.println("SD_notFund func ... : " + request->url());

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
      File file = SD.open(filename, "r");
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
      String ContentType = getContentType(filename);
      AsyncWebServerResponse *response = request->beginResponse(SD, filename, ContentType);
      request->send(response);
      SD_downloadsize = SD_GetFileSize(filename);
      SD_downloadtime = millis() - SD_start;
      // request->redirect("/SD_dir");
    }
    if (request->url().startsWith("/SD_deletehandler"))
    {
      Serial.println("SD_Delete handler started...");
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
  SD_Directory();
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
  SD_Directory();
  webpage = HTML_Header();
  webpage += "<h3>SD:　Select a File to " + title + " from this device</h3>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>File Name</th><th>File Size</th><th class='sp'></th><th>File Name</th><th>File Size</th></tr>";
  while (index < SD_numfiles)
  {
    Fname1 = SD_Filenames[index].filename;
    Fname2 = SD_Filenames[index + 1].filename;
    if (Fname1.startsWith("/"))
      Fname1 = Fname1.substring(1);
    if (Fname2.startsWith("/"))
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
  File CheckFile = SD.open(filename, "r");
  filesize = CheckFile.size();
  CheckFile.close();
  return filesize;
}

// --------------------------------------------------------------------
String SD_totalBytes(int res)
// res : 小数点以下の桁数: decimal places
{
  return ConvBinUnits(SD.totalBytes(), res);
}

String SD_usedBytes(int res)
{
  return ConvBinUnits(SD.usedBytes(), res);
}

String SD_freeSpace(int res)
{
  return ConvBinUnits(SD.totalBytes() - SD.usedBytes(), res);
}

// -------------------------------------------------------------------
void SDdir_flserverSetup()
{
  // ############### Spiffs/Sd change  #############################
  // server.on("/fileSystem", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  //   Serial.println("change file system SPIFFS <---> SD ...");
  //   handle_fileSystem(request); // file System change  SPIFFS - SD
  //   request->send(200, "text/html", webpage); });

  // ######################  ROOT_SD ################################
  server.on("/root_sd", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("change SdPath to Root ...");
    handle_root_sd(); 
    request->send(200, "text/html", webpage); });

  // ##################### CHDIR HANDLER ############################
  server.on("/chdir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("chdir...");
    Select_Dir_For_Function("[CHDIR]", "chdirhandler");
    request->send(200, "text/html", webpage); });

  // ##################### MKDIR HANDLER ############################
  server.on("/mkdir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("mkdir ...");
    Dir_Make();
    request->send(200, "text/html", webpage); });

  // ##################### RMDIR HANDLER #############################
  server.on("/rmdir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("rmdir...");
    Select_Dir_For_Function("[RMDIR]", "rmdirhandler");
    request->send(200, "text/html", webpage); });
}

// #############################################################################################
// void handle_fileSystem(AsyncWebServerRequest *request)
// {
//   String modeS;
//   modeS = request->arg("mode");

//   if (modeS != "")
//   {
//     if (modeS == "toggle")
//       isSPIFFS ^= 1; // 反転

//     else if (modeS == "SPIFFS")
//       isSPIFFS = 1;

//     else if (modeS == "SD")
//     {
//       isSPIFFS = 0;
//     }

//     if (!isSPIFFS)
//     {
//       SdPath = String("/");
//     }
//   }

//   Serial.println("file System is " + FLS_NAME[isSPIFFS]);

//   Home();
// }

// #############################################################################################
void handle_root_sd()
{ // change SD Path to Root

  SdPath = String("/");
  Serial.println("change SdPath to Root");

  webpage = HTML_Header();
  webpage += "<h3>Change SD Path to Root</h3>";
  webpage += "<a href='/dir'>[Enter]</a><br><br>";
  webpage += HTML_Footer();
}

// #############################################################################################
void Handle_chdir(String filename)
{ // chdri
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
    webpage += "<a href='/dir'>[Enter]</a><br><br>";
  }
  else
  {
    webpage += "<h3>Directory [ " + filename + " ] is NOT changed successfully</h3>";
    webpage += "<a href='/dir'>[Enter]</a><br><br>";
  }
  webpage += HTML_Footer();
}

// #############################################################################################
void Handle_rmdir(String filename)
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
    webpage += "<a href='/dir'>[Enter]</a><br><br>";
  }
  else
  {
    webpage += "<h3>Directory [ " + filename + " ] is NOT removed successfully</h3>";
    webpage += "<a href='/dir'>[Enter]</a><br><br>";
  }
  webpage += HTML_Footer();
}

// #############################################################################################
void Handle_mkdir(AsyncWebServerRequest *request)
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
    webpage += "<a href='/dir'>[Enter]</a><br><br>";
  }
  else
  {
    webpage += "<h3>Directory [ " + filename + " ] is NOT created successfully</h3>";
    webpage += "<a href='/dir'>[Enter]</a><br><br>";
  }
  webpage += HTML_Footer();
}

// #############################################################################################
void Dir_Make()
{
  SelectInputDirName("Make New Directory", "mkdirhandler", "filename");
}

// #############################################################################################
void SelectInputDirName(String Heading, String Command, String Arg_name)
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
void Select_Dir_For_Function(String title, String function)
{
  String Fname1, Fname2;
  int index = 0;
  DirsList(); // Get a Dir list
  webpage = HTML_Header();
  webpage += "<h3>Select a Directory to " + title + " from this device</h3>";
  webpage += "<table class='center'>";
  webpage += "<tr> <th>Directory Name</th> <th>Directory Name</th> </tr>";

  while (index < numDirs)
  {
    Fname1 = SD_Filenames[index].filename;
    Fname2 = SD_Filenames[index + 1].filename;
    if (Fname1.startsWith("/"))
      Fname1 = Fname1.substring(1);
    if (Fname2.startsWith("/"))
      Fname2 = Fname2.substring(1);

    webpage += "<tr>";
    webpage += "<td style='width:25%'><button><a href='" + function + "~/" + Fname1 + "'>" + Fname1 + "</a></button></td>";

    if (index < numDirs - 1)
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
//  File含まない。Dirのみの表示 for SD Only  .. by NoRi
void DirsList()
{
  numDirs = 0; // Reset number of dirs counter
  File root = SD.open(SdPath, "r");

  if (root)
  {
    root.rewindDirectory();
    File file = root.openNextFile();

    while (file)
    {
      String tmp_filename = (String(file.name()).startsWith("/") ? String(file.name()).substring(1) : file.name());

      if (!file.isDirectory() || tmp_filename == "System Volume Information")
      {
        file = root.openNextFile();
      }
      else
      {
        SD_Filenames[numDirs].filename = tmp_filename;
        SD_Filenames[numDirs].ftype = (file.isDirectory() ? "Dir" : "File");
        SD_Filenames[numDirs].fsize = ConvBinUnits(file.size(), 1);
        file = root.openNextFile();
        numDirs++;
      }
    }
    root.close();
  }
}

// #############################################################################################
//  Dirを含まない。fileのみを表示  .. by NoRi
void FilesList()
{
  SD_numfiles = 0; // Reset number of files in SD files
  File root;

  root = SD.open(SdPath, "r");

  if (root)
  {
    root.rewindDirectory();
    File file = root.openNextFile();

    while (file)
    { // Now get all the filenames, file types and sizes
      if (!file.isDirectory())
      {
        SD_Filenames[SD_numfiles].filename = (String(file.name()).startsWith("/") ? String(file.name()).substring(1) : file.name());
        SD_Filenames[SD_numfiles].ftype = (file.isDirectory() ? "Dir" : "File");
        SD_Filenames[SD_numfiles].fsize = ConvBinUnits(file.size(), 1);

        file = root.openNextFile();
        SD_numfiles++;
      }
      else
      {
        file = root.openNextFile();
      }
    }
    root.close();
  }
}

bool SDdir_notFound(AsyncWebServerRequest *request);
// #############################################################################################
bool SDdir_notFound(AsyncWebServerRequest *request)
{ // Process selected file types
  String filename;
  if (request->url().startsWith("/chdirhandler") ||
      request->url().startsWith("/mkdirhandler") ||
      request->url().startsWith("/rmdirhandler"))
  {
    SD_start = millis();
    // -------- Directory  HANDLER by NoRi --------------
    if (request->url().startsWith("/chdirhandler"))
    {
      Serial.println("chdir handler started...");
      Handle_chdir(filename);
      request->send(200, "text/html", webpage);
      return true;
    }

    if (request->url().startsWith("/mkdirhandler"))
    {
      Serial.println("mkdir handler started...");
      Handle_mkdir(request);
      request->send(200, "text/html", webpage);
      return true;
    }

    if (request->url().startsWith("/rmdirhandler"))
    {
      Serial.println("rmdir handler started...");
      Handle_rmdir(filename);
      request->send(200, "text/html", webpage);
      return true;
    }
    // -----------------------------------------------------
  }
  return false;
}
