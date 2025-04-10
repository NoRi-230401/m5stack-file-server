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

/*
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
      tmp.filename = String(file.name()).substring(1);
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
*/

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
      // Serial.println("Transferred : " + String(len) + " Bytes");
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

// void SPIFFS_handleFileUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
// {
//   String file = filename;
//   if (!index)
//   {
//     int lastSlash = filename.lastIndexOf('/');
//     if (lastSlash != -1)
//     {
//       file = filename.substring(lastSlash + 1);
//     }
//     lastSlash = filename.lastIndexOf('\\'); // Windows形式のパス区切りも考慮
//     if (lastSlash != -1)
//     {
//       file = filename.substring(lastSlash + 1);
//     }

//     if (!file.startsWith("/"))
//       file = "/" + file;

//     Serial.println("SPIFFS Upload target filename = " + file);
//     request->_tempFile = SPIFFS.open(file, "w");

//     if (!request->_tempFile)
//       Serial.println("Error creating file for SPIFFS upload...");

//     SPIFFS_uploadSize = 0;
//     SPIFFS_startTime = millis();
//   }

//   if (request->_tempFile)
//   {
//     if (len)
//     {
//       request->_tempFile.write(data, len);
//       // Serial.println("Transferred : " + String(len) + " Bytes"); // ログが多いのでコメントアウト推奨
//       SPIFFS_uploadSize = SPIFFS_uploadSize + len;
//     }

//     if (final)
//     {
//       request->_tempFile.close();
//       SPIFFS_uploadTime = millis() - SPIFFS_startTime;
//       Serial.println("Upload Complete: " + String(request->_tempFile.name()));
//       Serial.println("SPIFFS_uploadSize = " + String(SPIFFS_uploadSize) + " Bytes");
//       Serial.println("SPIFFS_uploadTime = " + String(SPIFFS_uploadTime) + " mSEC");
//       // request->redirect("/SPIFFS_dir");
//     }
//   }
// }

void SPIFFS_Handle_File_Delete(String filename)
{
  webpage = HTML_Header();
  String fullPath = filename;

  if (!fullPath.startsWith("/"))
    fullPath = "/" + fullPath;

  Serial.println("SPIFFS Delete target filename = " + fullPath);
  File dataFile = SPIFFS.open(fullPath, "r");

  if (dataFile)
  {
    dataFile.close();
    if (SPIFFS.remove(fullPath))
    {
      webpage += "<h3>SPIFFS: File '" + filename + "' has been deleted</h3>";
      webpage += "<a href='/SPIFFS_dir'>[Enter]</a><br><br>";
    }
    else
    {
      webpage += "<h3>SPIFFS: Failed to delete file [ " + filename + " ]</h3>";
      webpage += "<a href='/SPIFFS_dir'>[Enter]</a><br><br>";
    }
  }
  else
  {
    webpage += "<h3>SPIFFS: File [ " + filename + " ] does not exist</h3>";
    webpage += "<a href='/SPIFFS_dir'>[Enter]</a><br><br>";
  }
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
  webpage += "<th>File name</th>";    // 見出し1
  webpage += "<th>New Filename</th>"; // 見出し2
  webpage += "<th>Select</th>";       // 見出し3
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
  String filename = "";
  String url = request->url();

  String decodedUrl = url;
  decodedUrl.replace("%20", " "); // スペースなど、必要に応じて他のエンコード文字もデコード
  decodedUrl.replace("%21", "!");
  decodedUrl.replace("%23", "#");
  decodedUrl.replace("%24", "$");
  decodedUrl.replace("%25", "%");
  decodedUrl.replace("%26", "&");
  decodedUrl.replace("%27", "'");
  decodedUrl.replace("%28", "(");
  decodedUrl.replace("%29", ")");
  decodedUrl.replace("%2A", "*");
  decodedUrl.replace("%2B", "+");
  decodedUrl.replace("%2C", ",");
  // 必要に応じて他の文字も追加

  // ファイル名部分の抽出 ('~/' の後)
  int separatorIndex = decodedUrl.indexOf("~/");
  if (separatorIndex != -1)
  {
    filename = decodedUrl.substring(separatorIndex + 2);
  }

  // ハンドラ分岐
  if (url.startsWith("/SPIFFS_downloadhandler"))
  {
    Serial.println("SPIFFS_Download handler started for: " + filename);
    SPIFFS_startTime = millis();

    String fullPath = filename;
    if (!fullPath.startsWith("/"))
      fullPath = "/" + fullPath; // SPIFFSパス

    Serial.println("Download target full path = " + fullPath);
    File file = SPIFFS.open(fullPath, "r");

    if (file)
    {                                                  // SPIFFSはディレクトリがないので isDirectory チェック不要
      String contentType = getContentType("download"); // 強制ダウンロード
      AsyncWebServerResponse *response = request->beginResponse(
          contentType,
          file.size(),
          [file](uint8_t *buffer, size_t maxLen, size_t index) mutable -> size_t
          {
            size_t bytesRead = file.read(buffer, maxLen);
            // ラムダ内でのファイルcloseは避ける
            return bytesRead;
          });
      response->setContentLength(file.size());
      response->addHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
      response->addHeader("Server", "ESP Async Web Server");
      request->send(response);

      SPIFFS_downloadSize = file.size();
      SPIFFS_downloadTime = millis() - SPIFFS_startTime;
      Serial.println("SPIFFS download handler initiated...");
      // file.close(); // ここでは閉じない
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
    Serial.println("SPIFFS_Stream handler started for: " + filename);
    SPIFFS_startTime = millis();

    String fullPath = filename;
    if (!fullPath.startsWith("/"))
      fullPath = "/" + fullPath; // SPIFFSパス

    Serial.println("Stream target full path = " + fullPath);

    if (SPIFFS.exists(fullPath))
    {
      File file = SPIFFS.open(fullPath, "r"); // サイズ取得のために開く
      if (file)
      {
        String contentType = getContentType(filename);
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, fullPath, contentType);
        SPIFFS_downloadSize = file.size();
        file.close(); // サイズ取得後に閉じる
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
  else if (url.startsWith("/SPIFFS_deletehandler"))
  {
    Serial.println("SPIFFS_Delete handler started for: " + filename);
    SPIFFS_Handle_File_Delete(filename); // ファイル名のみ渡す ('/' なし)
    request->send(200, "text/html", webpage);
    return true; // ハンドルされた
  }
  else if (url.startsWith("/SPIFFS_renamehandler"))
  {
    Serial.println("SPIFFS Rename handler started...");
    // filename は使わず、フォームの引数から処理する
    SPIFFS_Handle_File_Rename(request, "", request->args()); // filename引数は空文字で渡す
    request->send(200, "text/html", webpage);
    return true; // ハンドルされた
  }

  return false; // 上記のいずれにも一致しない場合はハンドルされなかった
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
      String Fname_orig = SPIFFS_Filenames[index].filename; // 元のファイル名 (表示用, '/' なし)
      String Fname_url = Fname_orig;                        // URL生成用のファイル名 ('/' なし)

      // URLエンコード (簡易版)
      String Fname_encoded = Fname_url;
      Fname_encoded.replace(" ", "%20");
      Fname_encoded.replace("!", "%21");
      Fname_encoded.replace("#", "%23");
      Fname_encoded.replace("$", "%24");
      Fname_encoded.replace("%", "%25");
      Fname_encoded.replace("&", "%26");
      Fname_encoded.replace("'", "%27");
      Fname_encoded.replace("(", "%28");
      Fname_encoded.replace(")", "%29");
      Fname_encoded.replace("*", "%2A");
      Fname_encoded.replace("+", "%2B");
      Fname_encoded.replace(",", "%2C");

      // 各ファイルエントリの行 (クラス file-entry を追加)
      webpage += "<tr class='file-entry'>";

      // ファイル名セル (ボタン付きリンク) - file-name クラスを使用
      webpage += "<td class='file-name'>";
      webpage += "<button style='width: 100%; text-align: left; padding: 5px; box-sizing: border-box; white-space: normal; word-break: break-all;'>";
      webpage += "<a href='" + function + "~/" + Fname_encoded + "' style='display: block; text-decoration: none; color: inherit;'>" + Fname_orig + "</a>";
      webpage += "</button>";
      webpage += "</td>";

      // ファイルサイズセル - file-size クラスを使用
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
