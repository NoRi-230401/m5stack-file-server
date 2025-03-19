// *** Modified by NoRi 2025-03-18 ***
#include <SPIFFS.h>            // Built-in
// #include <WiFi.h>              // Built-in
// #include <ESPmDNS.h>           // Built-in
#include <AsyncTCP.h>          // https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer
// #include "esp_system.h"        // Built-in
// #include "esp_spi_flash.h"     // Built-in
// #include "esp_wifi_types.h"    // Built-in
// #include "esp_bt.h"            // Built-in
#define FS SPIFFS              // In preparation for the introduction of LITTLFS
                               // see https://github.com/lorol/LITTLEFS replace SPIFFS with LITTLEFS

// -------------------------------------------------------
void SPIFFS_flServerSetup();
void Dir(AsyncWebServerRequest *request);
void Directory();
void UploadFileSelect();
// void Format();
void handleFileUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);
void File_Stream();
void File_Delete();
void Handle_File_Delete(String filename);
void File_Rename();
void Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args);
bool SPIFFS_notFound(AsyncWebServerRequest *request);
void Handle_File_Download();
void Select_File_For_Function(String title, String function);
int GetFileSize(String filename);

extern void SelectInput(String Heading, String Command, String Arg_name);
extern void Display_System_Info();
extern void Home();
extern void LogOut();
extern void Display_New_Page();
extern String getContentType(String filenametype);
extern String ConvBinUnits(int bytes, int resolution);
extern String EncryptionType(wifi_auth_mode_t encryptionType);
extern String HTML_Header();
extern String HTML_Footer();
// -------------------------------------------------------
extern AsyncWebServer server;

typedef struct
{
  String filename;
  String ftype;
  String fsize;
} fileinfo;

String webpage, MessageLine;
fileinfo Filenames[200]; // Enough for most purposes!
bool StartupErrors = false;
int start, downloadtime = 1, uploadtime = 1, downloadsize, uploadsize, downloadrate, uploadrate, numfiles;
float Temperature = 21.34; // for example new page, amend in a sensor function if required
String Name = "Dave";

void SPIFFS_flServerSetup()
{
  Serial.println(__FILE__);
  if (!FS.begin(true))
  {
    Serial.println("Error preparing Filing System...");
    StartupErrors = true;
  }

  // ##################### HOMEPAGE HANDLER ###########################
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Home Page...");
    Home(); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  // ##################### LOGOUT HANDLER ############################
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    LogOut();
    request->send(200, "text/html", webpage); });

  // ##################### DOWNLOAD HANDLER ##########################
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Downloading file...");
    Select_File_For_Function("[DOWNLOAD]", "downloadhandler");
    request->send(200, "text/html", webpage); });

  // ##################### UPLOAD HANDLERS ###########################
  server.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Uploading file...");
    UploadFileSelect(); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  // Set handler for '/handleupload'
  server.on("/handleupload", HTTP_POST, [](AsyncWebServerRequest *request) {},
   [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
            { handleFileUpload(request, filename, index, data, len, final); });

  // Set handler for '/handleformat'
  // server.on("/handleformat", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  //   Serial.println("Processing Format Request of File System...");
  //   if (request->getParam("format")->value() == "YES") {
  //     Serial.print("Starting to Format Filing System...");
  //     FS.end();
  //     bool formatted = FS.format();
  //     if (formatted) {
  //       Serial.println(" Successful Filing System Format...");
  //     }
  //     else         {
  //       Serial.println(" Formatting Failed...");
  //     }
  //   }
  //   request->redirect("/dir"); });

  // ##################### STREAM HANDLER ############################
  server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Streaming file...");
    Select_File_For_Function("[STREAM]", "streamhandler"); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  // ##################### RENAME HANDLER ############################
  server.on("/rename", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Renaming file...");
    File_Rename(); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  // ##################### DIR HANDLER ###############################
  server.on("/dir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("File Directory...");
    Dir(request); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  // ##################### DELETE HANDLER ############################
  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Deleting file...");
    Select_File_For_Function("[DELETE]", "deletehandler"); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  // ##################### FORMAT HANDLER ############################
  // server.on("/format", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  //   Serial.println("Request to Format File System...");
  //   Format(); // Build webpage ready for display
  //   request->send(200, "text/html", webpage); });

  // ##################### SYSTEM HANDLER ############################
  server.on("/system", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Display_System_Info(); // Build webpage ready for display
    request->send(200, "text/html", webpage); });

  // ##################### IMAGE HANDLER ############################
  server.on("/icon", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(FS, "/icon.gif", "image/gif"); });
}

// #############################################################################################
void Dir(AsyncWebServerRequest *request)
{
  String Fname1, Fname2;
  int index = 0;
  Directory(); // Get a list of the current files on the FS
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS:　Filing System Content</h3><br>";
  if (numfiles > 0)
  {
    webpage += "<table class='center'>";
    webpage += "<tr><th>Type</th><th>File Name</th><th>File Size</th><th class='sp'></th><th>Type</th><th>File Name</th><th>File Size</th></tr>";
    while (index < numfiles)
    {
      Fname1 = Filenames[index].filename;
      Fname2 = Filenames[index + 1].filename;
      webpage += "<tr>";
      webpage += "<td style = 'width:5%'>" + Filenames[index].ftype + "</td><td style = 'width:25%'>" + Fname1 + "</td><td style = 'width:10%'>" + Filenames[index].fsize + "</td>";
      webpage += "<td class='sp'></td>";
      if (index < numfiles - 1)
      {
        webpage += "<td style = 'width:5%'>" + Filenames[index + 1].ftype + "</td><td style = 'width:25%'>" + Fname2 + "</td><td style = 'width:10%'>" + Filenames[index + 1].fsize + "</td>";
      }
      webpage += "</tr>";
      index = index + 2;
    }
    webpage += "</table>";
    webpage += "<p style='background-color:yellow;'><b>" + MessageLine + "</b></p>";
    MessageLine = "";
  }
  else
  {
    webpage += "<h2>No Files Found</h2>";
  }
  webpage += HTML_Footer();
  request->send(200, "text/html", webpage);
}
// #############################################################################################
void Directory()
{
  numfiles = 0; // Reset number of FS files counter
  File root = FS.open("/");
  if (root)
  {
    root.rewindDirectory();
    File file = root.openNextFile();
    while (file)
    { // Now get all the filenames, file types and sizes
      Filenames[numfiles].filename = (String(file.name()).startsWith("/") ? String(file.name()).substring(1) : file.name());
      Filenames[numfiles].ftype = (file.isDirectory() ? "Dir" : "File");
      Filenames[numfiles].fsize = ConvBinUnits(file.size(), 1);
      file = root.openNextFile();
      numfiles++;
    }
    root.close();
  }
}
// #############################################################################################
void UploadFileSelect()
{
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS:　Select a File to [UPLOAD] to this device</h3>";
  webpage += "<form method = 'POST' action = '/handleupload' enctype='multipart/form-data'>";
  webpage += "<input type='file' name='filename'><br><br>";
  webpage += "<input type='submit' value='Upload'>";
  webpage += "</form>";
  webpage += HTML_Footer();
}

// #############################################################################################
// void Format()
// {
//   webpage = HTML_Header();
//   webpage += "<h3>*** SPIFFS:　 Format Filing System on this device ***</h3>";
//   webpage += "<form action='/handleformat'>";
//   webpage += "<input type='radio' id='YES' name='format' value = 'YES'><label for='YES'>YES</label><br><br>";
//   webpage += "<input type='radio' id='NO'  name='format' value = 'NO' checked><label for='NO'>NO</label><br><br>";
//   webpage += "<input type='submit' value='Format?'>";
//   webpage += "</form>";
//   webpage += HTML_Footer();
// }

// #############################################################################################
void handleFileUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    String file = filename;
    if (!filename.startsWith("/"))
      file = "/" + filename;
    request->_tempFile = FS.open(file, "w");
    if (!request->_tempFile)
      Serial.println("Error creating file for upload...");
    start = millis();
  }
  if (request->_tempFile)
  {
    if (len)
    {
      request->_tempFile.write(data, len); // Chunked data
      Serial.println("Transferred : " + String(len) + " Bytes");
    }
    if (final)
    {
      uploadsize = request->_tempFile.size();
      request->_tempFile.close();
      uploadtime = millis() - start;
      request->redirect("/dir");
    }
  }
}
// #############################################################################################
void File_Stream()
{
  SelectInput("Select a File to Stream", "handlestream", "filename");
}
// #############################################################################################
void File_Delete()
{
  SelectInput("Select a File to Delete", "handledelete", "filename");
}
// #############################################################################################
void Handle_File_Delete(String filename)
{ // Delete the file
  webpage = HTML_Header();
  if (!filename.startsWith("/"))
    filename = "/" + filename;
  File dataFile = FS.open(filename, "r"); // Now read FS to see if file exists
  if (dataFile)
  { // It does so delete it
    FS.remove(filename);
    webpage += "<h3>SPIFFS:　File '" + filename.substring(1) + "' has been deleted</h3>";
    webpage += "<a href='/dir'>[Enter]</a><br><br>";
  }
  else
  {
    webpage += "<h3>SPIFFS:　File [ " + filename + " ] does not exist</h3>";
    webpage += "<a href='/dir'>[Enter]</a><br><br>";
  }
  webpage += HTML_Footer();
}
// #############################################################################################
void File_Rename()
{ // Rename the file
  Directory();
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS:　Select a File to [RENAME] on this device</h3>";
  webpage += "<FORM action='/renamehandler'>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>File name</th><th>New Filename</th><th>Select</th></tr>";
  int index = 0;
  while (index < numfiles)
  {
    webpage += "<tr><td><input type='text' name='oldfile' style='color:blue;' value = '" + Filenames[index].filename + "' readonly></td>";
    webpage += "<td><input type='text' name='newfile'></td><td><input type='radio' name='choice'></tr>";
    index++;
  }
  webpage += "</table><br>";
  webpage += "<input type='submit' value='Enter'>";
  webpage += "</form>";
  webpage += HTML_Footer();
}
// #############################################################################################
void Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args)
{ // Rename the file
  String newfilename;
  // int Args = request->args();
  webpage = HTML_Header();
  for (int i = 0; i < Args; i++)
  {
    if (request->arg(i) != "" && request->arg(i + 1) == "on")
    {
      filename = request->arg(i - 1);
      newfilename = request->arg(i);
    }
  }
  if (!filename.startsWith("/"))
    filename = "/" + filename;
  if (!newfilename.startsWith("/"))
    newfilename = "/" + newfilename;
  File CurrentFile = FS.open(filename, "r"); // Now read FS to see if file exists
  if (CurrentFile && filename != "/" && newfilename != "/" && (filename != newfilename))
  { // It does so rename it, ignore if no entry made, or Newfile name exists already
    if (FS.rename(filename, newfilename))
    {
      filename = filename.substring(1);
      newfilename = newfilename.substring(1);
      webpage += "<h3>SPIFFS:　File '" + filename + "' has been renamed to '" + newfilename + "'</h3>";
      webpage += "<a href='/dir'>[Enter]</a><br><br>";
    }
  }
  else
  {
    if (filename == "/" && newfilename == "/")
      webpage += "<h3>SPIFFS:　File was not renamed</h3>";
    else
      webpage += "<h3>SPIFFS:　New filename exists, cannot rename</h3>";
    webpage += "<a href='/rename'>[Enter]</a><br><br>";
  }
  CurrentFile.close();
  webpage += HTML_Footer();
}

// #############################################################################################
//  Not found handler is also the handler for 'delete', 'download' and 'stream' functions
bool SPIFFS_notFound(AsyncWebServerRequest *request)
{ // Process selected file types
  Serial.println("SPIFFS_notFund func ...");

  String filename;
  if (request->url().startsWith("/downloadhandler") ||
      request->url().startsWith("/streamhandler") ||
      request->url().startsWith("/deletehandler") ||
      request->url().startsWith("/renamehandler"))
  {
    // Now get the filename and handle the request for 'delete' or 'download' or 'stream' functions
    if (!request->url().startsWith("/renamehandler"))
      filename = request->url().substring(request->url().indexOf("~/") + 1);
    start = millis();
    if (request->url().startsWith("/downloadhandler"))
    {
      Serial.println("Download handler started...");
      MessageLine = "";
      File file = FS.open(filename, "r");
      String contentType = getContentType("download");
      AsyncWebServerResponse *response = request->beginResponse(contentType, file.size(), [file](uint8_t *buffer, size_t maxLen, size_t total) mutable -> size_t
                                                                { return file.read(buffer, maxLen); });
      response->addHeader("Server", "ESP Async Web Server");
      request->send(response);
      downloadtime = millis() - start;
      downloadsize = GetFileSize(filename);
      // request->redirect("/dir");
    }
    if (request->url().startsWith("/streamhandler"))
    {
      Serial.println("Stream handler started...");
      String ContentType = getContentType(filename);
      AsyncWebServerResponse *response = request->beginResponse(FS, filename, ContentType);
      request->send(response);
      downloadsize = GetFileSize(filename);
      downloadtime = millis() - start;
      // request->redirect("/dir");
    }
    if (request->url().startsWith("/deletehandler"))
    {
      Serial.println("Delete handler started...");
      Handle_File_Delete(filename); // Build webpage ready for display
      request->send(200, "text/html", webpage);
    }
    if (request->url().startsWith("/renamehandler"))
    {
      Handle_File_Rename(request, filename, request->args()); // Build webpage ready for display
      request->send(200, "text/html", webpage);
    }
    return true;
  }
  return false;
}

// #############################################################################################
void Handle_File_Download()
{
  String filename = "";
  int index = 0;
  Directory(); // Get a list of files on the FS
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS:　Select a File to Download</h3>";
  webpage += "<table>";
  webpage += "<tr><th>File Name</th><th>File Size</th></tr>";
  while (index < numfiles)
  {
    webpage += "<tr><td><a href='" + Filenames[index].filename + "'></a><td>" + Filenames[index].fsize + "</td></tr>";
    index++;
  }
  webpage += "</table>";
  webpage += "<p>" + MessageLine + "</p>";
  webpage += HTML_Footer();
}

// #############################################################################################
void Select_File_For_Function(String title, String function)
{
  String Fname1, Fname2;
  int index = 0;
  Directory(); // Get a list of files on the FS
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS:　Select a File to " + title + " from this device</h3>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>File Name</th><th>File Size</th><th class='sp'></th><th>File Name</th><th>File Size</th></tr>";
  while (index < numfiles)
  {
    Fname1 = Filenames[index].filename;
    Fname2 = Filenames[index + 1].filename;
    if (Fname1.startsWith("/"))
      Fname1 = Fname1.substring(1);
    if (Fname2.startsWith("/"))
      Fname2 = Fname2.substring(1);
    webpage += "<tr>";
    webpage += "<td style='width:25%'><button><a href='" + function + "~/" + Fname1 + "'>" + Fname1 + "</a></button></td><td style = 'width:10%'>" + Filenames[index].fsize + "</td>";
    webpage += "<td class='sp'></td>";
    if (index < numfiles - 1)
    {
      webpage += "<td style='width:25%'><button><a href='" + function + "~/" + Fname2 + "'>" + Fname2 + "</a></button></td><td style = 'width:10%'>" + Filenames[index + 1].fsize + "</td>";
    }
    webpage += "</tr>";
    index = index + 2;
  }
  webpage += "</table>";
  webpage += HTML_Footer();
}

// #############################################################################################
// void SelectInput(String Heading, String Command, String Arg_name)
// {
//   webpage = HTML_Header();
//   webpage += "<h3>" + Heading + "</h3>";
//   webpage += "<form  action='/" + Command + "'>";
//   webpage += "Filename: <input type='text' name='" + Arg_name + "'><br><br>";
//   webpage += "<input type='submit' value='Enter'>";
//   webpage += "</form>";
//   webpage += HTML_Footer();
// }

// #############################################################################################
int GetFileSize(String filename)
{
  int filesize;
  File CheckFile = FS.open(filename, "r");
  filesize = CheckFile.size();
  CheckFile.close();
  return filesize;
}
