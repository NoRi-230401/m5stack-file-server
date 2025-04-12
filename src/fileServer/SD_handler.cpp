// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-01
// -------------------------------------------------------
// SD_handler.cpp
// *******************************************************
#include "fileServer.h"
// -------------------------------------------------------
void SD_flServerSetup();
void SD_Dir(AsyncWebServerRequest *request);
void SD_Directory();
void SD_UploadFileSelect();
void SD_handleFileUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);
void SD_Handle_File_Delete(String encoded_filename);
void SD_Generate_Confirm_Page(String encoded_filename);
void SD_File_Rename();
void SD_Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args);
bool SD_notFound(AsyncWebServerRequest *request);
void SD_Select_File_For_Function(String title, String function);
// -------------------------------------------------------
void SDdir_flserverSetup();
void SDdir_handle_chTop();
void SDdir_handle_chUp();
void SDdir_Select_Dir_For_Function(String title, String function);
void SDdir_Generate_Confirm_Page(String encoded_filename);
void SDdir_Handle_chdir(String filename);
void SDdir_Handle_rmdir(String encoded_filename);
void SDdir_Handle_mkdir(AsyncWebServerRequest *request);
void SDdir_DirMake();
void SDdir_DirList();
void SDdir_FilesList();
bool SDdir_notFound(AsyncWebServerRequest *request);
void SDdir_InputNewDirName(String Heading, String Command, String Arg_name);
// -------------------------------------------------------
extern AsyncWebServer server;
extern String webpage;
std::vector<fileinfo> SD_Filenames;
uint32_t SD_startTime, SD_downloadTime = 1, SD_uploadTime = 1;
uint64_t SD_downloadSize, SD_uploadSize;
uint32_t SD_numfiles;
String SdPath = "/";

void SD_flServerSetup()
{
  Serial.println(__FILE__);

  server.on("/SD_download", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SD_Downloading file...");
    SD_Select_File_For_Function("[DOWNLOAD] for PC", "SD_downloadhandler");
    request->send(200, "text/html", webpage); });

  server.on("/SD_upload", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SD_Uploading file...");
    SD_UploadFileSelect();
    request->send(200, "text/html", webpage); });

  server.on("/SD_handleupload", HTTP_POST, [](AsyncWebServerRequest *request) {}, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
            { SD_handleFileUpload(request, filename, index, data, len, final); });

  server.on("/SD_stream", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SD_Streaming file...");
    SD_Select_File_For_Function("[STREAM]", "SD_streamhandler");
    request->send(200, "text/html", webpage); });

  server.on("/SD_rename", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SD_Renaming file...");
    SD_File_Rename();
    request->send(200, "text/html", webpage); });

  server.on("/SD_dir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SD_File Directory...");
    SD_Dir(request);
    request->send(200, "text/html", webpage); });

  server.on("/SD_delete", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SD_Deleting file...");
    SD_Select_File_For_Function("[DELETE]", "SD_deletehandler");
    request->send(200, "text/html", webpage); });
}

void SD_Dir(AsyncWebServerRequest *request)
{
  SD_Directory();
  webpage = HTML_Header();
  webpage += "<h3>SD: Content (" + SdPath + ")</h3>";
  if (SD_numfiles > 0)
  {
    webpage += "<table class='file-list-table'>";
    webpage += "<tbody>";
    for (int index = 0; index < SD_numfiles; index++)
    {
      webpage += "<tr class='file-entry'>";
      webpage += "<td class='file-type'>" + SD_Filenames[index].ftype + "</td>";
      webpage += "<td class='file-name'>" + SD_Filenames[index].filename + "</td>";
      webpage += "<td class='file-size'>" + SD_Filenames[index].fsize + "</td>";
      webpage += "</tr>";
    }
    webpage += "</tbody>";
    webpage += "</table>";
  }
  else
  {
    webpage += "<p style='text-align: center; margin-top: 20px;'>No files or directories found in " + SdPath + "</p>";
  }
  webpage += HTML_Footer();
}

const String SD_SYSTEM_FILE = "System Volume Information";
void SD_Directory()
{
  SD_numfiles = 0;
  SD_Filenames.clear();
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

      // パスからファイル名部分だけを抽出 (file.name()がフルパスを返す場合があるため)
      int lastSlash = tmp_filename.lastIndexOf('/');
      if (lastSlash != -1)
      {
        tmp_filename = tmp_filename.substring(lastSlash + 1);
      }

      if (tmp_filename != SD_SYSTEM_FILE && tmp_filename != "") // 空のファイル名も除外
      {
        fileinfo tmp;
        tmp.filename = tmp_filename;
        tmp.ftype = (file.isDirectory() ? "Dir" : "File");
        if (tmp.ftype == "File")
          tmp.fsize = ConvBytesUnits(file.size(), 1);
        else
          tmp.fsize = "";

        SD_Filenames.push_back(tmp);
        SD_numfiles++;
      }
      file = root.openNextFile();
    }
    root.close();
  }
  std::sort(SD_Filenames.begin(), SD_Filenames.end(), compareFileinfo);
}

void SD_UploadFileSelect()
{
  webpage = HTML_Header();
  webpage += "<h3>SD: Select a File to [UPLOAD] to this device (" + SdPath + ")</h3>";
  webpage += "<form method = 'POST' action = '/SD_handleupload' enctype='multipart/form-data'>";
  webpage += "<input type='file' name='filename'><br><br>";
  webpage += "<input type='submit' value='Upload'>";
  webpage += "</form>";
  webpage += HTML_Footer();
}

void SD_handleFileUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
  String file = filename;
  if (!index)
  {
    int lastSlash = filename.lastIndexOf('/');
    if (lastSlash != -1)
    {
      file = filename.substring(lastSlash + 1);
    }
    lastSlash = filename.lastIndexOf('\\'); // Windows形式のパス区切りも考慮
    if (lastSlash != -1)
    {
      file = filename.substring(lastSlash + 1);
    }

    if (!file.startsWith("/"))
      file = "/" + file;

    String fullPath = file;
    if (SdPath != "/")
      fullPath = SdPath + file;

    Serial.println("Upload target filename = " + fullPath);
    request->_tempFile = SD.open(fullPath, "w");

    if (!request->_tempFile)
      Serial.println("Error creating file for upload...");

    SD_uploadSize = 0;
    SD_startTime = millis();
  }

  if (request->_tempFile)
  {
    if (len)
    {
      request->_tempFile.write(data, len);
      // Serial.println("Transferred : " + String(len) + " Bytes");
      SD_uploadSize = SD_uploadSize + len;
    }

    if (final)
    {
      request->_tempFile.close();
      SD_uploadTime = millis() - SD_startTime;
      Serial.println("Upload Complete: " + String(request->_tempFile.name()));
      Serial.println("SD_uploadSize = " + String(SD_uploadSize) + " Bytes");
      Serial.println("SD_uploadTime = " + String(SD_uploadTime) + " mSEC");
      request->redirect("/SD_dir");
    }
  }
}

void SD_Handle_File_Delete(String encoded_filename)
{
  webpage = HTML_Header();
  String decoded_filename = urlDecode(encoded_filename);

  String fullPath = decoded_filename;
  if (!fullPath.startsWith("/"))
    fullPath = "/" + fullPath;

  if (SdPath != "/")
    fullPath = SdPath + fullPath;

  Serial.println("SD Delete execute target filename = " + fullPath + " (decoded)");
  File dataFile = SD.open(fullPath, "r");

  if (dataFile)
  {
    dataFile.close(); // ファイルを閉じてから削除
    if (SD.remove(fullPath))
    {
      webpage += "<h3>SD: File '" + decoded_filename + "' in " + SdPath + " has been deleted</h3>";
      webpage += "<a href='/SD_dir'>[OK]</a><br><br>";
    }
    else
    {
      webpage += "<h3>SD: Failed to delete file [ " + decoded_filename + " ] in " + SdPath + "</h3>";
      webpage += "<a href='/SD_delete'>[Back to Select]</a><br><br>";
    }
  }
  else
  {
    webpage += "<h3>SD: File [ " + decoded_filename + " ] in " + SdPath + " does not exist</h3>";
    webpage += "<a href='/SD_delete'>[Back to Select]</a><br><br>";
  }
  webpage += HTML_Footer();
}

void SD_Generate_Confirm_Page(String encoded_filename)
{
  webpage = HTML_Header();
  String decoded_filename = urlDecode(encoded_filename);

  webpage += "<h3>Confirm File Deletion (SD)</h3>";
  webpage += "<p>Are you sure you want to delete the file:</p>";
  webpage += "<p style='font-weight: bold; color: red;'>" + decoded_filename + "</p>";
  webpage += "<p>in path: " + SdPath + "?</p>";
  webpage += "<br>";

  // はい（削除実行）ボタン - エンコードされたファイル名を渡す
  webpage += "<a href='/SD_delete_execute~/" + encoded_filename + "' style='padding: 10px 20px; background-color: #dc3545; color: white; text-decoration: none; border-radius: 5px; margin-right: 10px;'>Yes, Delete</a>";

  // いいえ（キャンセル）ボタン
  webpage += "<a href='/SD_delete' style='padding: 10px 20px; background-color: #6c757d; color: white; text-decoration: none; border-radius: 5px;'>No, Cancel</a>";

  webpage += HTML_Footer();
}

void SD_File_Rename()
{
  SD_Directory();
  webpage = HTML_Header();
  webpage += "<h3>SD: Select a Dir/File to [RENAME] (" + SdPath + ")</h3>";
  webpage += "<FORM action='/SD_renamehandler'>";
  webpage += "<table class='file-list-table rename-table'>";
  webpage += "<thead>";
  webpage += "<tr>";
  webpage += "<th>File name</th>";
  webpage += "<th>New Filename</th>";
  webpage += "<th>Select</th>";
  webpage += "</tr>";
  webpage += "</thead>";
  webpage += "<tbody>";

  if (SD_numfiles > 0)
  {
    int index = 0;
    while (index < SD_numfiles)
    {
      webpage += "<tr class='file-entry'>";
      webpage += "<td class='file-name'><input type='text' name='oldfile' style='color:blue; width: 95%; box-sizing: border-box;' value='" + SD_Filenames[index].filename + "' readonly></td>";

      webpage += "<td class='rename-new-name'><input type='text' name='newfile' style='width: 95%; box-sizing: border-box;'></td>";
      webpage += "<td class='rename-select'><input type='radio' name='choice'></td>";
      webpage += "</tr>";
      index++;
    }
  }
  else
  {
    webpage += "<tr><td colspan='3' style='text-align: center; padding: 20px;'>No files or directories found in " + SdPath + " to rename.</td></tr>";
  }

  webpage += "</tbody>";
  webpage += "</table><br>";
  webpage += "<input type='submit' value='Enter'>";
  webpage += "</form>";
  webpage += HTML_Footer();
}

void SD_Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args)
{
  // 'filename' 引数は SD_notFound から渡されるが、この関数では使わない。
  // フォームから送信された 'oldfile', 'newfile', 'choice' を使う。
  String oldfilename_form = "";
  String newfilename_form = "";
  webpage = HTML_Header();

  // フォームデータの解析: 'choice'が'on'になっている行の'oldfile'と'newfile'を取得
  for (int i = 0; i < Args; i++)
  {
    if (request->argName(i) == "choice" && request->arg(i) == "on")
    {
      // 'choice' が 'on' の場合、その前の2つの引数が newfile と oldfile のはず
      if (i >= 2 && request->argName(i - 1) == "newfile" && request->argName(i - 2) == "oldfile")
      {
        oldfilename_form = request->arg(i - 2);
        newfilename_form = request->arg(i - 1);
        break;
      }
    }
  }

  Serial.println("Rename requested:");
  Serial.println("  Old filename (from form): " + oldfilename_form);
  Serial.println("  New filename (from form): " + newfilename_form);
  Serial.println("  Current Path (SdPath): " + SdPath);

  // 入力チェック
  if (oldfilename_form == "" || newfilename_form == "")
  {
    webpage += "<h3>SD: Rename Error - No file selected or new name missing.</h3>";
    webpage += "<a href='/SD_rename'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    return;
  }
  if (newfilename_form.indexOf('/') != -1 || newfilename_form.indexOf('\\') != -1)
  {
    webpage += "<h3>SD: Rename Error - New filename cannot contain '/' or '\'.</h3>";
    webpage += "<a href='/SD_rename'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    return;
  }
  if (oldfilename_form == newfilename_form)
  {
    webpage += "<h3>SD: Rename Error - New filename is the same as the old filename.</h3>";
    webpage += "<a href='/SD_rename'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    return;
  }

  // フルパスの構築
  String oldfilepath = oldfilename_form;
  if (!oldfilepath.startsWith("/"))
    oldfilepath = "/" + oldfilepath;
  if (SdPath != "/")
    oldfilepath = SdPath + oldfilepath;

  String newfilepath = newfilename_form;
  if (!newfilepath.startsWith("/"))
    newfilepath = "/" + newfilepath;
  if (SdPath != "/")
    newfilepath = SdPath + newfilepath;

  Serial.println("  Old full path: " + oldfilepath);
  Serial.println("  New full path: " + newfilepath);

  // ファイル/ディレクトリの存在確認とリネーム実行
  File currentItem = SD.open(oldfilepath, "r"); // ファイルまたはディレクトリを開く試み

  if (currentItem) // 存在する場合
  {
    currentItem.close(); // 確認のため開いただけなので閉じる

    // 新しい名前のファイル/ディレクトリが既に存在しないか確認
    if (SD.exists(newfilepath))
    {
      webpage += "<h3>SD: Rename Error - New filename '" + newfilename_form + "' already exists in " + SdPath + ".</h3>";
      webpage += "<a href='/SD_rename'>[Back]</a><br><br>";
    }
    else
    {
      // リネーム実行
      if (SD.rename(oldfilepath, newfilepath))
      {
        webpage += "<h3>SD: Item '" + oldfilename_form + "' in " + SdPath + " has been renamed to '" + newfilename_form + "'</h3>";
        webpage += "<a href='/SD_dir'>[OK]</a><br><br>";
      }
      else
      {
        webpage += "<h3>SD: Rename Error - Failed to rename '" + oldfilename_form + "' to '" + newfilename_form + "' in " + SdPath + ".</h3>";
        webpage += "<a href='/SD_rename'>[Back]</a><br><br>";
      }
    }
  }
  else // 元のファイル/ディレクトリが存在しない場合
  {
    webpage += "<h3>SD: Rename Error - Original item '" + oldfilename_form + "' not found in " + SdPath + ".</h3>";
    webpage += "<a href='/SD_rename'>[Back]</a><br><br>";
  }
  webpage += HTML_Footer();
}

bool SD_notFound(AsyncWebServerRequest *request)
{
  String filename_encoded = "";
  String url = request->url();

  // ファイル名部分の抽出 ('~/' の後) - ★エンコードされたまま取得
  int separatorIndex = url.indexOf("~/");
  if (separatorIndex != -1)
  {
    filename_encoded = url.substring(separatorIndex + 2);
  }
  
  // ハンドラ分岐
  if (url.startsWith("/SD_downloadhandler"))
  {
    String dl_filename_decoded = urlDecode(filename_encoded);
    Serial.println("SD_Download handler started for: " + dl_filename_decoded + " (decoded)");
    SD_startTime = millis();

    String fullPath = dl_filename_decoded;
    if (!fullPath.startsWith("/"))
      fullPath = "/" + fullPath;
    if (SdPath != "/")
      fullPath = SdPath + fullPath;

    Serial.println("Download target full path = " + fullPath);
    File file = SD.open(fullPath, "r");

    if (file && !file.isDirectory())
    {
      String contentType = getContentType("download");
      AsyncWebServerResponse *response = request->beginResponse(
          contentType,
          file.size(),
          [file](uint8_t *buffer, size_t maxLen, size_t index) mutable -> size_t
          {
            size_t bytesRead = file.read(buffer, maxLen);
            return bytesRead;
          });
      response->setContentLength(file.size());
      response->addHeader("Content-Disposition", "attachment; filename=\"" + dl_filename_decoded + "\"");
      response->addHeader("Server", "ESP Async Web Server");
      request->send(response);

      SD_downloadSize = file.size();
      SD_downloadTime = millis() - SD_startTime;
      Serial.println("SD download handler initiated...");
    }
    else
    {
      if (file)
        file.close();
      Serial.println("Error: File not found or is a directory: " + fullPath);
      request->send(404, "text/plain", "File not found or is a directory");
    }
    return true;
  }
  else if (url.startsWith("/SD_streamhandler"))
  {
    String stream_filename_decoded = urlDecode(filename_encoded);
    Serial.println("SD_Stream handler started for: " + stream_filename_decoded + " (decoded)");
    SD_startTime = millis();

    String fullPath = stream_filename_decoded;
    if (!fullPath.startsWith("/"))
      fullPath = "/" + fullPath;
    if (SdPath != "/")
      fullPath = SdPath + fullPath;

    Serial.println("Stream target full path = " + fullPath);

    if (SD.exists(fullPath))
    {
      File file = SD.open(fullPath, "r");
      if (file && !file.isDirectory())
      {
        String contentType = getContentType(stream_filename_decoded);
        AsyncWebServerResponse *response = request->beginResponse(SD, fullPath, contentType);
        SD_downloadSize = file.size();
        file.close();
        SD_downloadTime = millis() - SD_startTime;
        request->send(response);
        Serial.println("SD stream handler initiated...");
      }
      else
      {
        if (file)
          file.close();
        Serial.println("Error: Cannot stream directory or file open failed: " + fullPath);
        request->send(404, "text/plain", "Cannot stream directory or file open failed");
      }
    }
    else
    {
      Serial.println("Error: File not found for streaming: " + fullPath);
      request->send(404, "text/plain", "File not found for streaming");
    }
    return true;
  }
  else if (url.startsWith("/SD_delete_confirm"))
  {
    Serial.println("SD_Delete confirm page requested for: " + filename_encoded);
    SD_Generate_Confirm_Page(filename_encoded);
    request->send(200, "text/html", webpage);
    return true;
  }
  else if (url.startsWith("/SD_delete_execute"))
  {
    Serial.println("SD_Delete execute handler started for: " + filename_encoded);
    SD_Handle_File_Delete(filename_encoded);
    request->send(200, "text/html", webpage);
    return true;
  }
  else if (url.startsWith("/SD_renamehandler"))
  {
    Serial.println("SD Rename handler started...");
    SD_Handle_File_Rename(request, "", request->args());
    request->send(200, "text/html", webpage);
    return true;
  }
  return false;
}

void SD_Select_File_For_Function(String title, String function)
{
  SDdir_FilesList();

  webpage = HTML_Header();
  webpage += "<h3>SD: Select a File to " + title + " (" + SdPath + ")</h3>";

  if (SD_numfiles > 0)
  {
    webpage += "<table class='file-list-table'>";
    webpage += "<tbody>";

    for (int index = 0; index < SD_numfiles; index++)
    {
      String Fname_orig = SD_Filenames[index].filename;
      String Fname_encoded = urlEncode(Fname_orig);
      
      String target_function = function;
      if (function == "SD_deletehandler")
      {
        target_function = "SD_delete_confirm";
      }

      webpage += "<tr class='file-entry'>";
      webpage += "<td class='file-name'>";
      webpage += "<button style='width: 100%; text-align: left; padding: 5px; box-sizing: border-box; white-space: normal; word-break: break-all;'>";
      
      webpage += "<a href='" + target_function + "~/" + Fname_encoded + "' style='display: block; text-decoration: none; color: inherit;'>" + Fname_orig + "</a>";
      webpage += "</button>";
      webpage += "</td>";
      webpage += "<td class='file-size'>" + SD_Filenames[index].fsize + "</td>";
      webpage += "</tr>";
    }
    webpage += "</tbody>";
    webpage += "</table>";
  }
  else
  {
    webpage += "<p style='text-align: center; margin-top: 20px;'>No files found in " + SdPath + " to " + title + "</p>";
  }
  webpage += HTML_Footer();
}

// -------------------------------------------------------
// SDdir_* 関数群
// -------------------------------------------------------
void SDdir_flserverSetup()
{
  server.on("/SDdir_chdir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SDdir_chdir...");
    SDdir_Select_Dir_For_Function("[CHDIR:change directory]", "SDdir_chdirhandler");
    request->send(200, "text/html", webpage); });

  server.on("/SDdir_mkdir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SDdir_mkdir ...");
    SDdir_DirMake(); // 入力フォーム表示
    request->send(200, "text/html", webpage); });

  // mkdir の実処理ハンドラ (POSTまたはGETで受ける)
  // GETで受ける場合 (SDdir_InputNewDirName からの遷移)
  server.on("/SDdir_mkdirhandler", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        Serial.println("SDdir_mkdir handler (GET) started...");
        SDdir_Handle_mkdir(request);
        request->send(200, "text/html", webpage); });
  // POSTで受ける場合も考慮するなら追加
  /*
  server.on("/SDdir_mkdirhandler", HTTP_POST, [](AsyncWebServerRequest *request) {
       Serial.println("SDdir_mkdir handler (POST) started...");
       // POSTの場合、引数は request->arg("arg_name") ではなく
       // request->getParam("arg_name", true)->value() などで取得する必要がある場合がある
       SDdir_Handle_mkdir(request);
       request->send(200, "text/html", webpage);
   });
  */

  server.on("/SDdir_rmdir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SDdir_rmdir...");
    SDdir_Select_Dir_For_Function("[RMDIR:remove directory]", "SDdir_rmdirhandler");
    request->send(200, "text/html", webpage); });

  server.on("/SDdir_chTop", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("change SdPath to Root directory...");
    SDdir_handle_chTop();
    request->send(200, "text/html", webpage); });

  server.on("/SDdir_chUp", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("change SdPath to Up directory...");
    SDdir_handle_chUp();
    request->send(200, "text/html", webpage); });
}

void SDdir_handle_chTop()
{
  SdPath = String("/");
  webpage = HTML_Header();
  webpage += "<h3>Changed SD Path to Root Directory (/)</h3>";
  webpage += "<a href='/SD_dir'>[Show Content]</a><br><br>";
  webpage += HTML_Footer();
}

void SDdir_handle_chUp()
{
  String upPath = String("/");

  if (SdPath != "/")
  {
    int i = SdPath.lastIndexOf('/');
    if (i > 0)
    { // ルート("/")の一つ下の階層の場合、iは0になる
      upPath = SdPath.substring(0, i);
    }
    else
    {
      // ルート直下か、予期せぬパス形式の場合はルートに戻す
      upPath = "/";
    }
  }
  // upPath が "/" の場合は変更しない
  if (SdPath != "/")
  {
    SdPath = upPath;
  }

  Serial.println("Changed SdPath to = " + SdPath);
  webpage = HTML_Header();
  webpage += "<h3>Changed SD Path to Up Directory (" + SdPath + ")</h3>";
  webpage += "<a href='/SD_dir'>[Show Content]</a><br><br>";
  webpage += HTML_Footer();
}

void SDdir_Handle_chdir(String encoded_filename)
{
  String filename = urlDecode(encoded_filename);

  webpage = HTML_Header();
  String targetPath = filename;

  if (!targetPath.startsWith("/"))
    targetPath = "/" + targetPath;

  if (SdPath != "/")
    targetPath = SdPath + targetPath;

  Serial.println("Change directory target = " + targetPath);

  // ターゲットパスの正規化 (例: "/dir1//dir2" -> "/dir1/dir2")
  targetPath.replace("//", "/");
  if (targetPath != "/" && targetPath.endsWith("/"))
  {
    targetPath = targetPath.substring(0, targetPath.length() - 1);
  }

  File dir = SD.open(targetPath);
  if (dir && dir.isDirectory())
  { // ディレクトリが存在するか確認
    dir.close();
    SdPath = targetPath;
    Serial.println("Successfully changed SdPath = " + SdPath);
    webpage += "<h3>Directory changed to '" + SdPath + "'</h3>";
    webpage += "<a href='/SD_dir'>[Show Content]</a><br><br>";
  }
  else
  {
    if (dir)
      dir.close(); // ファイルだった場合や開けなかった場合に閉じる
    Serial.println("Failed to change directory to [ " + targetPath + " ]");
    webpage += "<h3>Error: Directory [ " + filename + " ] not found or is not a directory in " + SdPath + "</h3>";
    webpage += "<a href='/SDdir_chdir'>[Back to Select]</a><br><br>"; // 選択画面に戻るリンク
    webpage += "<a href='/SD_dir'>[Show Current Content]</a><br><br>";
  }
  webpage += HTML_Footer();
}

void SDdir_Handle_rmdir(String encoded_filename)
{
  String filename = urlDecode(encoded_filename);
  webpage = HTML_Header();
  String targetPath = filename;

  if (!targetPath.startsWith("/"))
    targetPath = "/" + targetPath;

  if (SdPath != "/")
    targetPath = SdPath + targetPath;

  Serial.println("Remove directory target = " + targetPath);

  // ルートディレクトリは削除不可
  if (targetPath == "/")
  {
    webpage += "<h3>Error: Cannot remove the root directory.</h3>";
    webpage += "<a href='/SDdir_rmdir'>[Back to Select]</a><br><br>";
    webpage += HTML_Footer();
    return;
  }

  // ディレクトリ削除実行
  if (SD.rmdir(targetPath))
  {
    Serial.println("Removed dir = " + targetPath);
    webpage += "<h3>Directory '" + filename + "' in " + SdPath + " has been removed</h3>";
    webpage += "<a href='/SD_dir'>[OK]</a><br><br>"; // 成功したらDir表示へ
  }
  else
  {
    // 失敗理由を推測 (存在しない、空でない、など)
    String errorMsg = "<h3>Error: Failed to remove directory [ " + filename + " ] in " + SdPath + ".";
    File dir = SD.open(targetPath);
    if (!dir)
    {
      errorMsg += " (Directory not found)";
    }
    else
    {
      dir.close();
      // 空かどうかをチェックするのは難しい場合がある
      errorMsg += " (Directory might not be empty or is write-protected)";
    }
    errorMsg += "</h3>";
    webpage += errorMsg;
    webpage += "<a href='/SDdir_rmdir'>[Back to Select]</a><br><br>";
  }
  webpage += HTML_Footer();
}

void SDdir_Handle_mkdir(AsyncWebServerRequest *request)
{
  webpage = HTML_Header();
  String filename = "";

  // GET または POST パラメータからファイル名を取得
  if (request->hasParam("filename", false))
  { // GET パラメータチェック
    filename = request->arg("filename");
  }
  else if (request->hasParam("filename", true))
  { // POST パラメータチェック
    filename = request->getParam("filename", true)->value();
  }

  Serial.println("Make directory requested name = " + filename);

  // 入力チェック
  if (filename == "")
  {
    webpage += "<h3>Error: Directory name cannot be empty.</h3>";
    webpage += "<a href='/SDdir_mkdir'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    return;
  }
  if (filename.indexOf('/') != -1 || filename.indexOf('\\') != -1)
  {
    webpage += "<h3>Error: Directory name cannot contain '/' or '\'.</h3>";
    webpage += "<a href='/SDdir_mkdir'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    return;
  }

  // フルパス構築
  String fullPath = filename;
  if (!fullPath.startsWith("/"))
    fullPath = "/" + fullPath;

  if (SdPath != "/")
    fullPath = SdPath + fullPath;

  Serial.println("Make directory full path = " + fullPath);

  // ディレクトリ作成実行
  if (SD.mkdir(fullPath))
  {
    Serial.println("Created new dir = " + fullPath);
    webpage += "<h3>Directory '" + filename + "' has been created in " + SdPath + "</h3>";
    webpage += "<a href='/SD_dir'>[OK]</a><br><br>"; // 成功したらDir表示へ
  }
  else
  {
    String errorMsg = "<h3>Error: Failed to create directory [ " + filename + " ] in " + SdPath + ".";
    if (SD.exists(fullPath))
    {
      errorMsg += " (Item with the same name already exists)";
    }
    else
    {
      errorMsg += " (Invalid path or insufficient permissions)";
    }
    errorMsg += "</h3>";
    webpage += errorMsg;
    webpage += "<a href='/SDdir_mkdir'>[Back]</a><br><br>";
  }
  webpage += HTML_Footer();
}

void SDdir_DirMake()
{
  SDdir_InputNewDirName("Make New Directory in " + SdPath, "SDdir_mkdirhandler", "filename");
}

void SDdir_Select_Dir_For_Function(String title, String function)
{
  SDdir_DirList();
  webpage = HTML_Header();
  webpage += "<h3>SD: Select a Directory to " + title + " (" + SdPath + ")</h3>";

  if (SD_numfiles > 0)
  {
    webpage += "<table class='file-list-table'>";
    webpage += "<tbody>";

    for (int index = 0; index < SD_numfiles; index++)
    {
      String Dname_orig = SD_Filenames[index].filename;
      String Dname_encoded = urlEncode(Dname_orig);

      String target_function = function;
      if (function == "SDdir_rmdirhandler")
      {
        target_function = "SDdir_rmdir_confirm";
      }

      webpage += "<tr class='file-entry'>";
      webpage += "<td class='file-name'>";
      webpage += "<button style='width: 100%; text-align: left; padding: 5px; box-sizing: border-box; white-space: normal; word-break: break-all;'>";
      webpage += "<a href='" + target_function + "~/" + Dname_encoded + "' style='display: block; text-decoration: none; color: inherit;'>";
      webpage += "<span style='color: #007bff;'>&#128193;</span> " + Dname_orig; // フォルダアイコンを追加
      webpage += "</a>";
      webpage += "</button>";
      webpage += "</td>";
      webpage += "</tr>";
    }
    webpage += "</tbody>";
    webpage += "</table>";
  }
  else
  {
    webpage += "<p style='text-align: center; margin-top: 20px;'>No sub-directories found in " + SdPath + "</p>";
  }

  webpage += HTML_Footer();
}

void SDdir_Generate_Confirm_Page(String encoded_filename)
{
  webpage = HTML_Header();
  String decoded_filename = urlDecode(encoded_filename);

  webpage += "<h3>Confirm Directory Deletion</h3>";
  webpage += "<p>Are you sure you want to delete the directory:</p>";
  webpage += "<p style='font-weight: bold; color: red;'>" + decoded_filename + "</p>";
  webpage += "<p>in path: " + SdPath + "?</p>";
  webpage += "<p style='color: grey;'>Note: Only empty directories can be deleted.</p>";
  webpage += "<br>";

  // はい（削除実行）ボタン
  webpage += "<a href='/SDdir_rmdir_execute~/" + encoded_filename + "' style='padding: 10px 20px; background-color: #dc3545; color: white; text-decoration: none; border-radius: 5px; margin-right: 10px;'>Yes, Delete</a>";

  // いいえ（キャンセル）ボタン
  webpage += "<a href='/SDdir_rmdir' style='padding: 10px 20px; background-color: #6c757d; color: white; text-decoration: none; border-radius: 5px;'>No, Cancel</a>";

  webpage += HTML_Footer();
}

void SDdir_DirList()
{ // 'Dir' type only , not involve 'File'
  SD_numfiles = 0;
  SD_Filenames.clear();

  if (SdPath == "")
    SdPath = "/";
  Serial.println("Listing directories in: " + SdPath);
  File root = SD.open(SdPath, "r");

  if (root && root.isDirectory()) // ディレクトリとして開けるか確認
  {
    root.rewindDirectory();
    File file = root.openNextFile();
    while (file)
    {
      if (file.isDirectory()) // ディレクトリかどうかをチェック
      {
        String tmp_filename = file.name(); // フルパスが返ることがある
        // パスから最後の部分（ディレクトリ名）を抽出
        int lastSlash = tmp_filename.lastIndexOf('/');
        if (lastSlash != -1)
        {
          tmp_filename = tmp_filename.substring(lastSlash + 1);
        }

        if (tmp_filename != "" && tmp_filename != SD_SYSTEM_FILE) // システムフォルダと空名は除外
        {
          fileinfo tmp;
          tmp.filename = tmp_filename;
          tmp.ftype = "Dir";
          tmp.fsize = ""; // ディレクトリなのでサイズは空

          SD_Filenames.push_back(tmp);
          SD_numfiles++;
        }
      }
      file = root.openNextFile();
    }
    root.close();
  }
  else
  {
    if (root)
      root.close(); // 開けたがディレクトリでなかった場合
    Serial.println("Error opening directory: " + SdPath);
  }
  std::sort(SD_Filenames.begin(), SD_Filenames.end(), compareFileinfo); // 名前順ソート
}

void SDdir_FilesList()
{ // 'File' type only , not involve 'Dir'
  SD_numfiles = 0;
  SD_Filenames.clear();
  if (SdPath == "")
    SdPath = "/";
  // Serial.println("Listing files in: " + SdPath);
  File root = SD.open(SdPath, "r");

  if (root && root.isDirectory())
  {
    root.rewindDirectory();
    File file = root.openNextFile();

    while (file)
    {
      if (!file.isDirectory()) // ファイルかどうかをチェック
      {
        String tmp_filename = file.name();
        int lastSlash = tmp_filename.lastIndexOf('/');
        if (lastSlash != -1)
        {
          tmp_filename = tmp_filename.substring(lastSlash + 1);
        }

        if (tmp_filename != "")
        { // 空のファイル名は除外
          fileinfo tmp;
          tmp.filename = tmp_filename;
          tmp.ftype = "File";
          tmp.fsize = ConvBytesUnits(file.size(), 1);

          SD_Filenames.push_back(tmp);
          SD_numfiles++;
        }
      }
      file = root.openNextFile();
    }
    root.close();
  }
  else
  {
    if (root)
      root.close();
    Serial.println("Error opening directory for file listing: " + SdPath);
  }
  std::sort(SD_Filenames.begin(), SD_Filenames.end(), compareFileinfo);
}

bool SDdir_notFound(AsyncWebServerRequest *request)
{
  String filename_encoded = "";
  String url = request->url();

  // ファイル名部分の抽出 ('~/' の後) - ★エンコードされたまま取得
  int separatorIndex = url.indexOf("~/");
  if (separatorIndex != -1)
  {
    filename_encoded = url.substring(separatorIndex + 2); // エンコードされたファイル名
  }

  if (url.startsWith("/SDdir_chdirhandler"))
  {
    Serial.println("SDdir_chdir handler started for: " + filename_encoded);
    SDdir_Handle_chdir(filename_encoded);
    request->send(200, "text/html", webpage);
    return true;
  }
  else if (url.startsWith("/SDdir_mkdirhandler")) // GETリクエストで処理
  {
    Serial.println("SDdir_mkdir handler (from notFound) started...");
    SDdir_Handle_mkdir(request); // requestオブジェクトを渡して中で引数を解析
    request->send(200, "text/html", webpage);
    return true;
  }
  else if (url.startsWith("/SDdir_rmdir_confirm"))
  {
    Serial.println("SDdir_rmdir confirm page requested for: " + filename_encoded);
    SDdir_Generate_Confirm_Page(filename_encoded);
    request->send(200, "text/html", webpage);
    return true;
  }
  else if (url.startsWith("/SDdir_rmdir_execute"))
  {
    Serial.println("SDdir_rmdir execute handler started for: " + filename_encoded);
    SDdir_Handle_rmdir(filename_encoded);
    request->send(200, "text/html", webpage);
    return true;
  }

  return false;
}


void SDdir_InputNewDirName(String Heading, String Command, String Arg_name)
{
  webpage = HTML_Header();
  webpage += "<h3>" + Heading + "</h3>";
  webpage += "<form method='GET' action='/" + Command + "'>";
  webpage += "New Directory Name: <input type='text' name='" + Arg_name + "' required pattern='[^/\\]+' title='Directory name cannot contain / or \'><br><br>";
  webpage += "<input type='submit' value='Create'>";
  webpage += "</form>";
  webpage += "<br><a href='/SD_dir'>[Cancel and Back to Directory]</a>";
  webpage += HTML_Footer();
}
