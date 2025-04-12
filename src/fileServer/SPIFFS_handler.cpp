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
void SPIFFS_Handle_File_Delete(String encoded_filename);
void SPIFFS_Generate_Confirm_Page(String encoded_filename);
void SPIFFS_File_Rename();
void SPIFFS_Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args);
bool SPIFFS_notFound(AsyncWebServerRequest *request);
void SPIFFS_Select_File_For_Function(String title, String function);
// -------------------------------------------------------
extern AsyncWebServer server;
extern String webpage;
std::vector<fileinfo> SPIFFS_Filenames;
uint32_t SPIFFS_startTime, SPIFFS_downloadTime = 1, SPIFFS_uploadTime = 1;
uint64_t SPIFFS_downloadSize, SPIFFS_uploadSize;
uint32_t SPIFFS_numfiles;

void SPIFFS_flServerSetup()
{
  Serial.println(__FILE__);

  server.on("/SPIFFS_download", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SPIFFS Downloading file...");
    SPIFFS_Select_File_For_Function("[DOWNLOAD] for PC", "SPIFFS_downloadhandler");
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

      if (tmp.filename != "")
      {
        SPIFFS_Filenames.push_back(tmp);
        SPIFFS_numfiles++;
      }
      file = root.openNextFile();
    }
    root.close();
  }
  std::sort(SPIFFS_Filenames.begin(), SPIFFS_Filenames.end(), compareFileinfo);
}

void SPIFFS_Dir(AsyncWebServerRequest *request)
{
  SPIFFS_Directory();
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS: Content</h3>";

  if (SPIFFS_numfiles > 0)
  {
    webpage += "<table class='file-list-table'>";
    webpage += "<tbody>";
    for (int index = 0; index < SPIFFS_numfiles; index++)
    {
      webpage += "<tr class='file-entry'>";
      webpage += "<td class='file-type'>" + SPIFFS_Filenames[index].ftype + "</td>";
      webpage += "<td class='file-name'>" + SPIFFS_Filenames[index].filename + "</td>";
      webpage += "<td class='file-size'>" + SPIFFS_Filenames[index].fsize + "</td>";
      webpage += "</tr>";
    }
    webpage += "</tbody>";
    webpage += "</table>";
  }
  else
  {
    webpage += "<p style='text-align: center; margin-top: 20px;'>No files found in SPIFFS</p>";
  }
  webpage += HTML_Footer();
}

void SPIFFS_UploadFileSelect()
{
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS: Select a File to [UPLOAD] to this device</h3>";
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

    Serial.println("SPIFFS Upload target filename = " + file);
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
      // Serial.println("Transferred : " + String(len) + " Bytes");
      SPIFFS_uploadSize = SPIFFS_uploadSize + len;
    }

    if (final)
    {
      request->_tempFile.close();
      SPIFFS_uploadTime = millis() - SPIFFS_startTime;
      Serial.println("Upload Complete: " + String(request->_tempFile.name()));
      Serial.println("SPIFFS_uploadSize = " + String(SPIFFS_uploadSize) + " Bytes");
      Serial.println("SPIFFS_uploadTime = " + String(SPIFFS_uploadTime) + " mSEC");
      request->redirect("/SPIFFS_dir");
    }
  }
}

void SPIFFS_Handle_File_Delete(String encoded_filename)
{
  String filename = urlDecode(encoded_filename);
  webpage = HTML_Header();
  String fullPath = filename; // デコード後のファイル名を使用

  if (!fullPath.startsWith("/"))
    fullPath = "/" + fullPath;

  Serial.println("SPIFFS Delete execute target filename = " + fullPath + " (decoded)");
  File dataFile = SPIFFS.open(fullPath, "r");

  if (dataFile)
  {
    dataFile.close();
    if (SPIFFS.remove(fullPath))
    {
      webpage += "<h3>SPIFFS: File '" + filename + "' has been deleted</h3>";
      webpage += "<a href='/SPIFFS_dir'>[OK]</a><br><br>"; // Dir表示へ
    }
    else
    {
      webpage += "<h3>SPIFFS: Failed to delete file [ " + filename + " ]</h3>";
      webpage += "<a href='/SPIFFS_delete'>[Back to Select]</a><br><br>"; // 削除選択画面へ戻る
    }
  }
  else
  {
    webpage += "<h3>SPIFFS: File [ " + filename + " ] does not exist</h3>";
    webpage += "<a href='/SPIFFS_delete'>[Back to Select]</a><br><br>"; // 削除選択画面へ戻る
  }
  webpage += HTML_Footer();
}

void SPIFFS_Generate_Confirm_Page(String encoded_filename)
{
  webpage = HTML_Header();
  String decoded_filename = urlDecode(encoded_filename);

  webpage += "<h3>Confirm File Deletion (SPIFFS)</h3>";
  webpage += "<p>Are you sure you want to delete the file:</p>";
  webpage += "<p style='font-weight: bold; color: red;'>" + decoded_filename + "</p>";
  webpage += "<br>";

  // はい（削除実行）ボタン - エンコードされたファイル名を渡す
  webpage += "<a href='/SPIFFS_delete_execute~/" + encoded_filename + "' style='padding: 10px 20px; background-color: #dc3545; color: white; text-decoration: none; border-radius: 5px; margin-right: 10px;'>Yes, Delete</a>";

  // いいえ（キャンセル）ボタン
  webpage += "<a href='/SPIFFS_delete' style='padding: 10px 20px; background-color: #6c757d; color: white; text-decoration: none; border-radius: 5px;'>No, Cancel</a>";

  webpage += HTML_Footer();
}

void SPIFFS_File_Rename()
{
  SPIFFS_Directory();
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS: Select a File to [RENAME]</h3>";
  webpage += "<FORM action='/SPIFFS_renamehandler'>";

  webpage += "<table class='file-list-table rename-table'>";
  webpage += "<thead>";
  webpage += "<tr>";
  webpage += "<th>File name</th>";
  webpage += "<th>New Filename</th>";
  webpage += "<th>Select</th>";
  webpage += "</tr>";
  webpage += "</thead>";
  webpage += "<tbody>";

  if (SPIFFS_numfiles > 0)
  {
    int index = 0;
    while (index < SPIFFS_numfiles)
    {
      webpage += "<tr class='file-entry'>";
      webpage += "<td class='file-name'><input type='text' name='oldfile' style='color:blue; width: 95%; box-sizing: border-box;' value='" + SPIFFS_Filenames[index].filename + "' readonly></td>";
      webpage += "<td class='rename-new-name'><input type='text' name='newfile' style='width: 95%; box-sizing: border-box;'></td>";
      webpage += "<td class='rename-select'><input type='radio' name='choice'></td>";
      webpage += "</tr>";
      index++;
    }
  }
  else
  {
    webpage += "<tr><td colspan='3' style='text-align: center; padding: 20px;'>No files found in SPIFFS to rename.</td></tr>";
  }

  webpage += "</tbody>";
  webpage += "</table><br>";
  webpage += "<input type='submit' value='Enter'>";
  webpage += "</form>";
  webpage += HTML_Footer();
}

void SPIFFS_Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args)
{
  // 'filename' 引数は SPIFFS_notFound から渡されるが、この関数では使わない。
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
        break; // 最初に見つかったものを採用
      }
    }
  }

  Serial.println("SPIFFS Rename requested:");
  Serial.println("  Old filename (from form): " + oldfilename_form);
  Serial.println("  New filename (from form): " + newfilename_form);

  // 入力チェック
  if (oldfilename_form == "" || newfilename_form == "")
  {
    webpage += "<h3>SPIFFS: Rename Error - No file selected or new name missing.</h3>";
    webpage += "<a href='/SPIFFS_rename'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    return;
  }
  // SPIFFSのファイル名は '/' で始まる必要があるが、入力自体に '/' を含めるのは禁止
  if (newfilename_form.indexOf('/') != -1 || newfilename_form.indexOf('\\') != -1)
  {
    webpage += "<h3>SPIFFS: Rename Error - New filename cannot contain '/' or '\'.</h3>";
    webpage += "<a href='/SPIFFS_rename'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    return;
  }
  if (oldfilename_form == newfilename_form)
  {
    webpage += "<h3>SPIFFS: Rename Error - New filename is the same as the old filename.</h3>";
    webpage += "<a href='/SPIFFS_rename'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    return;
  }

  // フルパスの構築 (SPIFFSでは常に '/' で始まる)
  String oldfilepath = oldfilename_form;
  if (!oldfilepath.startsWith("/"))
    oldfilepath = "/" + oldfilepath;

  String newfilepath = newfilename_form;
  if (!newfilepath.startsWith("/"))
    newfilepath = "/" + newfilepath;

  Serial.println("  Old full path: " + oldfilepath);
  Serial.println("  New full path: " + newfilepath);

  // ファイルの存在確認とリネーム実行
  File currentItem = SPIFFS.open(oldfilepath, "r");

  if (currentItem) // 存在する場合
  {
    currentItem.close(); // 確認のため開いただけなので閉じる

    // 新しい名前のファイルが既に存在しないか確認
    if (SPIFFS.exists(newfilepath))
    {
      webpage += "<h3>SPIFFS: Rename Error - New filename '" + newfilename_form + "' already exists.</h3>";
      webpage += "<a href='/SPIFFS_rename'>[Back]</a><br><br>";
    }
    else
    {
      // リネーム実行
      if (SPIFFS.rename(oldfilepath, newfilepath))
      {
        webpage += "<h3>SPIFFS: File '" + oldfilename_form + "' has been renamed to '" + newfilename_form + "'</h3>";
        webpage += "<a href='/SPIFFS_dir'>[OK]</a><br><br>"; // 成功したらDir表示へ
      }
      else
      {
        webpage += "<h3>SPIFFS: Rename Error - Failed to rename '" + oldfilename_form + "' to '" + newfilename_form + "'.</h3>";
        webpage += "<a href='/SPIFFS_rename'>[Back]</a><br><br>";
      }
    }
  }
  else // 元のファイルが存在しない場合
  {
    webpage += "<h3>SPIFFS: Rename Error - Original file '" + oldfilename_form + "' not found.</h3>";
    webpage += "<a href='/SPIFFS_rename'>[Back]</a><br><br>";
  }

  webpage += HTML_Footer();
}

bool SPIFFS_notFound(AsyncWebServerRequest *request)
{
  String filename_encoded = ""; // エンコードされたファイル名を保持
  String url = request->url();

  // ファイル名部分の抽出 ('~/' の後) - ★エンコードされたまま取得
  int separatorIndex = url.indexOf("~/");
  if (separatorIndex != -1)
  {
    filename_encoded = url.substring(separatorIndex + 2); // エンコードされたファイル名
  }

  // ハンドラ分岐
  if (url.startsWith("/SPIFFS_downloadhandler"))
  {
    String dl_filename_decoded = urlDecode(filename_encoded); // ★ 修正: デコード
    Serial.println("SPIFFS_Download handler started for: " + dl_filename_decoded + " (decoded)");
    SPIFFS_startTime = millis();

    String fullPath = dl_filename_decoded; // デコード後のファイル名を使用
    if (!fullPath.startsWith("/"))
      fullPath = "/" + fullPath; // SPIFFSパス

    Serial.println("Download target full path = " + fullPath);
    File file = SPIFFS.open(fullPath, "r");

    if (file)
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
      response->addHeader("Content-Disposition", "attachment; filename=\"" + dl_filename_decoded + "\""); // デコード後を使用
      response->addHeader("Server", "ESP Async Web Server");
      request->send(response);

      SPIFFS_downloadSize = file.size();
      SPIFFS_downloadTime = millis() - SPIFFS_startTime;
      Serial.println("SPIFFS download handler initiated...");
    }
    else
    {
      Serial.println("Error: File not found: " + fullPath);
      request->send(404, "text/plain", "File not found");
    }
    return true;
  }
  else if (url.startsWith("/SPIFFS_streamhandler"))
  {
    String stream_filename_decoded = urlDecode(filename_encoded);
    Serial.println("SPIFFS_Stream handler started for: " + stream_filename_decoded + " (decoded)");
    SPIFFS_startTime = millis();

    String fullPath = stream_filename_decoded;

    if (!fullPath.startsWith("/"))
      fullPath = "/" + fullPath; // SPIFFSパス

    Serial.println("Stream target full path = " + fullPath);

    if (SPIFFS.exists(fullPath))
    {
      File file = SPIFFS.open(fullPath, "r");
      if (file)
      {
        String contentType = getContentType(stream_filename_decoded); // デコード後のファイル名で判定
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, fullPath, contentType);
        SPIFFS_downloadSize = file.size();
        file.close();
        SPIFFS_downloadTime = millis() - SPIFFS_startTime;
        request->send(response);
        Serial.println("SPIFFS stream handler initiated...");
      }
      else
      {
        Serial.println("Error: File open failed for streaming: " + fullPath);
        request->send(404, "text/plain", "File open failed for streaming");
      }
    }
    else
    {
      Serial.println("Error: File not found for streaming: " + fullPath);
      request->send(404, "text/plain", "File not found for streaming");
    }
    return true; // ハンドルされた
  }
  else if (url.startsWith("/SPIFFS_delete_confirm"))
  {
    Serial.println("SPIFFS_Delete confirm page requested for: " + filename_encoded);
    SPIFFS_Generate_Confirm_Page(filename_encoded); // エンコードされたファイル名を渡す
    request->send(200, "text/html", webpage);
    return true;
  }
  else if (url.startsWith("/SPIFFS_delete_execute"))
  {
    Serial.println("SPIFFS_Delete execute handler started for: " + filename_encoded);
    SPIFFS_Handle_File_Delete(filename_encoded);
    request->send(200, "text/html", webpage);
    return true;
  }
  else if (url.startsWith("/SPIFFS_renamehandler"))
  {
    Serial.println("SPIFFS Rename handler started...");
    SPIFFS_Handle_File_Rename(request, "", request->args());
    request->send(200, "text/html", webpage);
    return true;
  }
  return false;
}

void SPIFFS_Select_File_For_Function(String title, String function)
{
  SPIFFS_Directory();
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS: Select a File to " + title + "</h3>";

  if (SPIFFS_numfiles > 0)
  {
    webpage += "<table class='file-list-table'>";
    webpage += "<tbody>";

    for (int index = 0; index < SPIFFS_numfiles; index++)
    {
      String Fname_orig = SPIFFS_Filenames[index].filename;
      String Fname_encoded = urlEncode(Fname_orig);

      String target_function = function;
      if (function == "SPIFFS_deletehandler")
      {
        target_function = "SPIFFS_delete_confirm";
      }

      webpage += "<tr class='file-entry'>";
      webpage += "<td class='file-name'>";
      webpage += "<button style='width: 100%; text-align: left; padding: 5px; box-sizing: border-box; white-space: normal; word-break: break-all;'>";

      webpage += "<a href='" + target_function + "~/" + Fname_encoded + "' ...>" + Fname_orig + "</a>";

      webpage += "</button>";
      webpage += "</td>";
      webpage += "<td class='file-size'>" + SPIFFS_Filenames[index].fsize + "</td>";
      webpage += "</tr>";
    }
    webpage += "</tbody>";
    webpage += "</table>";
  }
  else
  {
    webpage += "<p style='text-align: center; margin-top: 20px;'>No files found in SPIFFS to " + title + "</p>";
  }
  webpage += HTML_Footer();
}
