// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-15
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
void SPIFFS_Select_File_For_ViewText();
void SPIFFS_View_Text(AsyncWebServerRequest *request, String encoded_filename);
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

  // ********* SPIFFS text file viewer ***********
  server.on("/SPIFFS_view_text", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  String filename_encoded = "";
  if (request->hasParam("file"))
  { // パラメータ名を 'file' とする場合
    filename_encoded = request->arg("file");
  }
  else
  {
    request->send(400, "text/plain", "Missing file parameter");
    return;
  }
  Serial.println("SPIFFS_View_Text requested for: " + filename_encoded);
  SPIFFS_View_Text(request, filename_encoded); });

  server.on("/SPIFFS_vTxt", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  Serial.println("SPIFFS_vTxt: text file viewer...");
  SPIFFS_Select_File_For_ViewText();
  request->send(200, "text/html", webpage); });
  // ***********************************************

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
  String decoded_filename = urlDecode(encoded_filename);
  webpage = HTML_Header();
  String fullPath = decoded_filename;

  if (!fullPath.startsWith("/"))
    fullPath = "/" + fullPath;

  Serial.println("SPIFFS Delete execute target filename = " + fullPath + " (decoded)");
  File dataFile = SPIFFS.open(fullPath, "r");

  if (dataFile)
  {
    dataFile.close();
    if (SPIFFS.remove(fullPath))
    {
      webpage += "<h3>SPIFFS: File '" + decoded_filename + "' has been deleted</h3>";
      webpage += "<a href='/SPIFFS_dir'>[OK]</a><br><br>";
    }
    else
    {
      webpage += "<h3>SPIFFS: Failed to delete file [ " + decoded_filename + " ]</h3>";
      webpage += "<a href='/SPIFFS_delete'>[Back to Select]</a><br><br>";
    }
  }
  else
  {
    webpage += "<h3>SPIFFS: File [ " + decoded_filename + " ] does not exist</h3>";
    webpage += "<a href='/SPIFFS_delete'>[Back to Select]</a><br><br>";
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
  // methodをGETに明示 (元のコードに合わせる)
  webpage += "<form action='/SPIFFS_renamehandler' method='GET'>";
  webpage += "<table class='file-list-table rename-table'>";

  // --- thead (ヘッダー) ---
  webpage += "<thead>";
  webpage += "<tr>";
  // ヘッダーを2列に変更
  webpage += "<th class='rename-select-header'>Select / File Name</th>"; 
  webpage += "<th class='rename-new-header'>New Filename</th>";
  webpage += "</tr>";
  webpage += "</thead>";

  // --- tbody (ボディ) ---
  webpage += "<tbody>";
  if (SPIFFS_numfiles > 0)
  {
    for (int index = 0; index < SPIFFS_numfiles; index++)
    {
      String current_filename = SPIFFS_Filenames[index].filename;
      // String current_ftype = SPIFFS_Filenames[index].ftype; // SPIFFSはファイルのみ
      String radio_id = "choice_" + String(index); // ラジオボタン用の一意なID

      webpage += "<tr class='file-entry'>"; // 各行

      // --- 1列目: Select / File Name ---
      webpage += "<td class='rename-select-cell'>";
      // 非表示のラジオボタン: name='choice', value=ファイル名
      webpage += "<input type='radio' name='choice' value='" + current_filename + "' id='" + radio_id + "' style='display: none;'>";
      // 隠しフィールド: name='oldfile', value=ファイル名 (ハンドラでの取得を容易にするため)
      webpage += "<input type='hidden' name='oldfile' value='" + current_filename + "'>";
      // ラベル (ボタン風): radio_idに対応付け
      webpage += "<label for='" + radio_id + "' class='rename-select-label'>";
      // フォルダアイコンは不要
      webpage += current_filename; // ファイル名表示
      webpage += "</label>";
      webpage += "</td>";

      // --- 2列目: New Filename ---
      webpage += "<td class='rename-new-name'>";
      // テキスト入力: name='newfile' (各行で同じname属性)
      webpage += "<input type='text' name='newfile' style='width: 95%; box-sizing: border-box;'>";
      webpage += "</td>";

      webpage += "</tr>"; // 行終了
    }
  }
  else
  {
    // ファイルがない場合の表示 (colspanを2に変更)
    webpage += "<tr><td colspan='2' style='text-align: center; padding: 20px;'>No files found in SPIFFS to rename.</td></tr>";
  }
  webpage += "</tbody>";
  webpage += "</table><br>";

  // 送信ボタン (Valueを変更)
  webpage += "<input type='submit' value='Rename Selected'>";
  webpage += "</form>";
  webpage += HTML_Footer();
}

void SPIFFS_Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args)
{
  String oldfilename_form = "";
  String newfilename_form = "";
  webpage = HTML_Header();

  // 1. 選択されたファイル名 (choice の value) を取得
  if (request->hasParam("choice"))
  {
    oldfilename_form = request->arg("choice");
  }
  else
  {
    // choice が選択されていない場合のエラー処理
    webpage += "<h3>SPIFFS: Rename Error - No file selected.</h3>";
    webpage += "<a href='/SPIFFS_rename'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    request->send(200, "text/html", webpage); // エラーページを送信して終了
    return;
  }

  // 2. 対応する newfile を取得
  //    フォームの引数をループして、選択された oldfile に対応する newfile を探す
  bool found_newfile = false;
  String current_oldfile_check = ""; // ループ内で oldfile を追跡
  for (int i = 0; i < Args; i++)
  {
    String argName = request->argName(i);
    String argValue = request->arg(i);

    if (argName == "oldfile")
    {
      current_oldfile_check = argValue; // 現在の行の oldfile を記録
    }
    else if (argName == "newfile" && current_oldfile_check == oldfilename_form)
    {
      // oldfile が選択されたものと一致し、かつ引数名が newfile なら、それが対応する新しい名前
      newfilename_form = argValue;
      found_newfile = true;
      break; // 見つかったらループを抜ける
    }
    // choice パラメータはここでは無視 (既に取得済みのため)
  }

  // newfile が見つからなかった場合 (通常は発生しないはず)
  if (!found_newfile)
  {
    Serial.println("Error: Could not find corresponding newfile parameter for selected oldfile: " + oldfilename_form);
    webpage += "<h3>SPIFFS: Rename Error - Internal error processing form data.</h3>";
    webpage += "<a href='/SPIFFS_rename'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    request->send(200, "text/html", webpage); // エラーページを送信して終了
    return;
  }

  Serial.println("SPIFFS Rename requested:");
  Serial.println("  Old filename (from choice): " + oldfilename_form);
  Serial.println("  New filename (from form): " + newfilename_form);

  // --- 3. 以降の入力チェックとリネーム処理 ---
  // 入力チェック (newfilename が空でないかもチェック)
  if (oldfilename_form == "" || newfilename_form == "") // newfilenameもチェック
  {
    webpage += "<h3>SPIFFS: Rename Error - File selected but new name is missing.</h3>";
    webpage += "<a href='/SPIFFS_rename'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    request->send(200, "text/html", webpage);
    return;
  }
  // SPIFFSのファイル名は '/' で始まる必要があるが、入力自体に '/' を含めるのは禁止
  if (newfilename_form.indexOf('/') != -1 || newfilename_form.indexOf('\\') != -1)
  {
    webpage += "<h3>SPIFFS: Rename Error - New filename cannot contain '/' or '\'.</h3>";
    webpage += "<a href='/SPIFFS_rename'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    request->send(200, "text/html", webpage);
    return;
  }
  if (oldfilename_form == newfilename_form)
  {
    webpage += "<h3>SPIFFS: Rename Error - New filename is the same as the old filename.</h3>";
    webpage += "<a href='/SPIFFS_rename'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    request->send(200, "text/html", webpage);
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
    currentItem.close();

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
  else
  {
    webpage += "<h3>SPIFFS: Rename Error - Original file '" + oldfilename_form + "' not found.</h3>";
    webpage += "<a href='/SPIFFS_rename'>[Back]</a><br><br>";
  }

  webpage += HTML_Footer();
}

bool SPIFFS_notFound(AsyncWebServerRequest *request)
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
  if (url.startsWith("/SPIFFS_downloadhandler"))
  {
    String dl_filename_decoded = urlDecode(filename_encoded);
    Serial.println("SPIFFS_Download handler started for: " + dl_filename_decoded + " (decoded)");
    SPIFFS_startTime = millis();

    String fullPath = dl_filename_decoded;
    if (!fullPath.startsWith("/"))
      fullPath = "/" + fullPath;

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
      response->addHeader("Content-Disposition", "attachment; filename=\"" + dl_filename_decoded + "\"");
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
      fullPath = "/" + fullPath;

    Serial.println("Stream target full path = " + fullPath);

    if (SPIFFS.exists(fullPath))
    {
      File file = SPIFFS.open(fullPath, "r");
      if (file)
      {
        String contentType = getContentType(stream_filename_decoded);
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
    return true;
  }
  else if (url.startsWith("/SPIFFS_delete_confirm"))
  {
    Serial.println("SPIFFS_Delete confirm page requested for: " + filename_encoded);
    SPIFFS_Generate_Confirm_Page(filename_encoded);
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

void SPIFFS_Select_File_For_ViewText()
{
  SPIFFS_Directory();
  webpage = HTML_Header();
  webpage += "<h3>SPIFFS: Select a Text File to View</h3>";

  // 許可する拡張子のリスト (小文字で定義)
  const std::vector<String> allowedExtensions = {
      ".txt", ".log", ".csv", ".json", ".yaml", ".htm", ".html", ".css", ".js", ".xml", ".md", ".ini", ".conf", ".cfg", ".c", ".h", ".cpp", ".hpp", ".py", ".inc"
      // 必要に応じて他のテキストベースの拡張子を追加する
  };

  int displayedFileCount = 0;

  if (SPIFFS_numfiles > 0)
  {
    webpage += "<table class='file-list-table'>";
    webpage += "<tbody>";

    // 取得した全ファイルをループ
    for (int index = 0; index < SPIFFS_numfiles; index++)
    {
      String Fname_orig = SPIFFS_Filenames[index].filename;
      String Fname_lower = Fname_orig; // 比較用にファイル名を小文字に変換
      Fname_lower.toLowerCase();

      bool isAllowed = false; // このファイルを表示するかどうかのフラグ

      // 許可リスト内の拡張子と一致するかチェック
      for (const String &ext : allowedExtensions)
      {
        if (Fname_lower.endsWith(ext))
        {
          isAllowed = true; // 一致したらフラグを立ててループを抜ける
          break;
        }
      }

      // 許可された拡張子の場合のみ、テーブルに行を追加
      if (isAllowed)
      {
        displayedFileCount++; // 表示カウントを増やす
        String Fname_encoded = urlEncode(Fname_orig);
        String link_url = "/SPIFFS_view_text?file=" + Fname_encoded;

        webpage += "<tr class='file-entry'>";
        webpage += "<td class='file-name'>";
        webpage += "<button style='width: 100%; text-align: left; padding: 5px; box-sizing: border-box; white-space: normal; word-break: break-all;'>";
        webpage += "<a href='" + link_url + "' style='display: block; text-decoration: none; color: inherit;'>" + Fname_orig + "</a>";
        webpage += "</button>";
        webpage += "</td>";
        webpage += "<td class='file-size'>" + SPIFFS_Filenames[index].fsize + "</td>";
        webpage += "</tr>";
      }
    }

    webpage += "</tbody>";
    webpage += "</table>";
  }

  // 表示するテキストファイルが一つもなかった場合のメッセージ
  // SD_numfiles > 0 でも、許可された拡張子のファイルがなければ displayedFileCount は 0 のまま
  if (displayedFileCount == 0)
  {
    // 元々ファイルがなかった場合も、テキストファイルがなかった場合もこのメッセージを表示
    webpage += "<p style='text-align: center; margin-top: 20px;'>No text files found</p>";
  }

  webpage += HTML_Footer();
}

void SPIFFS_View_Text(AsyncWebServerRequest *request, String encoded_filename)
{
  String decoded_filename = urlDecode(encoded_filename);
  String fullPath = decoded_filename;
  if (!fullPath.startsWith("/"))
    fullPath = "/" + fullPath;

  Serial.println("Viewing text file: " + fullPath);

  File file = SPIFFS.open(fullPath, "r");
  if (!file || file.isDirectory())
  {
    if (file)
      file.close();
    webpage = HTML_Header();
    webpage += "<h3>Error: Cannot view file</h3>";
    webpage += "<p>File not found or is a directory: " + decoded_filename + "</p>";
    webpage += "<a href='/SPIFFS_dir'>[Back to Directory]</a>";
    webpage += HTML_Footer();
    request->send(404, "text/html", webpage);
    return;
  }

  webpage = HTML_Header();
  webpage += "<h3>Viewing Text: " + decoded_filename + "</h3>";
  webpage += "<pre>"; // preタグで囲む

  // ファイル内容を読み込んで webpage に追加
  // 大きなファイルの場合、メモリに注意が必要。
  // ここでは一括読み込みしている。
  while (file.available())
  {
    // 一行ずつ読み込むか、バッファで読み込む
    String line = file.readStringUntil('\n');
    // HTMLエスケープが必要な場合 (例: < > & を表示したい場合)
    line.replace("&", "&amp;");
    line.replace("<", "&lt;");
    line.replace(">", "&gt;");
    webpage += line + "\n"; // 改行も維持
  }
  file.close();

  webpage += "</pre>";
  webpage += "<br><a href='/SPIFFS_dir'>[Back to Directory]</a>";
  webpage += "<br>";
  webpage += HTML_Footer();
  request->send(200, "text/html", webpage);
}
