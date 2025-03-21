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
extern String ConvBinUnits(int bytes, int resolution);
extern String EncryptionType(wifi_auth_mode_t encryptionType);
extern String HTML_Header();
extern String HTML_Footer();
// -------------------------------------------------------

void SDdir_flserverSetup();
void SDdir_handle_goRoot();
void SDdir_Select_Dir_For_Function(String title, String function);
void SDdir_Handle_chdir(String filename);
void SDdir_Handle_rmdir(String filename);
void SDdir_Handle_mkdir(AsyncWebServerRequest *request);
void SDdir_DirMake();
void SDdir_SelectInputDirName(String Heading, String Command, String Arg_name);
void SDdir_DirList();
void SDdir_FilesList();
bool SDdir_notFound(AsyncWebServerRequest *request);
// -------------------------------------------------------
extern AsyncWebServer server;
extern String webpage;
extern bool StartupErrors;

typedef struct
{
  String filename;
  String ftype;
  String fsize;
} fileinfo;
fileinfo SD_Filenames[200];
String SD_MessageLine;
int SD_start, SD_downloadtime = 1, SD_uploadtime = 1, SD_downloadsize, SD_uploadsize, SD_downloadrate, SD_uploadrate, SD_numfiles;
String SdPath = "/";

void SD_flServerSetup()
{
  Serial.println(__FILE__);

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

  if (SdPath == "")
    SdPath = "/";

  Serial.println("SdPath = " + SdPath);
  // File root = SD.open("/");
  File root = SD.open(SdPath);
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
  // SD_Directory();
  SDdir_FilesList();
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
      Serial.println("filename = " + filename);
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
  // SD_Directory();
  SDdir_FilesList();

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
  if (SdPath != "/")
    filename = SdPath + filename;

  Serial.println("filename = " + filename);
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
// *** SD directory function support by Nori
//   chdir,mkdir,rmdir, goRoot,

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

  // ######################  ROOT_SD ################################
  server.on("/SDdir_goRoot", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("change SdPath to Root ...");
    SDdir_handle_goRoot(); 
    request->send(200, "text/html", webpage); });
}

// #############################################################################################
void SDdir_handle_goRoot()
{ // change SD Path to Root

  SdPath = String("/");
  Serial.println("change SdPath to Root");

  webpage = HTML_Header();
  webpage += "<h3>Change SD Path to Root</h3>";
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
    Fname2 = SD_Filenames[index + 1].filename;
    if (Fname1.startsWith("/"))
      Fname1 = Fname1.substring(1);
    if (Fname2.startsWith("/"))
      Fname2 = Fname2.substring(1);

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
//  File含まない。Dirのみの表示 for SD Only  .. by NoRi
// void DirsList()
void SDdir_DirList()
{
  SD_numfiles = 0;
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
        SD_Filenames[SD_numfiles].filename = tmp_filename;
        SD_Filenames[SD_numfiles].ftype = (file.isDirectory() ? "Dir" : "File");
        SD_Filenames[SD_numfiles].fsize = ConvBinUnits(file.size(), 1);
        file = root.openNextFile();
        SD_numfiles++;
      }
    }
    root.close();
  }
}

// #############################################################################################
//  Dirを含まない。fileのみを表示  .. by NoRi
void SDdir_FilesList()
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

// #############################################################################################
bool SDdir_notFound(AsyncWebServerRequest *request)
{ // Process selected file types
  String filename;
  if (request->url().startsWith("/SDdir_chdirhandler") ||
      request->url().startsWith("/SDdir_mkdirhandler") ||
      request->url().startsWith("/SDdir_rmdirhandler"))
  {
    // Now get the filename and handle the request for 'chdir' or 'mkdir' or 'rmdir' functions
    filename = request->url().substring(request->url().indexOf("~/") + 1);

    // -------- Directory  HANDLER by NoRi --------------
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
    // -----------------------------------------------------
  }
  return false;
}
