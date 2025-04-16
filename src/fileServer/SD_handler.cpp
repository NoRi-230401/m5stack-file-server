// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-15
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
void SD_Select_File_For_ViewText();
void SD_View_Text(AsyncWebServerRequest *request, String encoded_filename);
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

  // ********************** SD text file viewer ****************
  server.on("/SD_view_text", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String filename_encoded = "";
              if (request->hasParam("file"))
              { // パラメータ名を 'file' とする場合
                filename_encoded = request->arg("file");
              }
              else
              {
                // エラー処理 or notFound へ
                request->send(400, "text/plain", "Missing file parameter");
                return;
              }
              Serial.println("SD_View_Text requested for: " + filename_encoded);
              SD_View_Text(request, filename_encoded);
              // SD_View_Text 内で request->send するのでここでは不要
            });

  server.on("/SD_vTxt", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("SD_vTxt: text file viewer...");
    SD_Select_File_For_ViewText();
    request->send(200, "text/html", webpage); });
  // *********************************************************************

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
      // Name列: Dirの場合はアイコンを追加
      webpage += "<td class='file-name'>";
      if (SD_Filenames[index].ftype == "Dir")
      {
        webpage += "<span style='color: #007bff;'>&#128193;</span> "; // フォルダアイコン 📁
      }
      webpage += SD_Filenames[index].filename;
      webpage += "</td>";
      // Size列: 変更なし
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
  // webpage += "<p>in path: " + SdPath + "?</p>";
  webpage += "<p>in path:&nbsp;「&nbsp;" + SdPath + "&nbsp;」&nbsp;?</p>";
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
  // methodをGETに変更し、actionを修正 (元のコードに合わせる)
  webpage += "<form action='/SD_renamehandler' method='GET'>";
  webpage += "<table class='file-list-table rename-table'>"; // テーブル開始

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
  if (SD_numfiles > 0)
  {
    for (int index = 0; index < SD_numfiles; index++)
    {
      String current_filename = SD_Filenames[index].filename;
      String current_ftype = SD_Filenames[index].ftype;
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
      if (current_ftype == "Dir")
      {
        webpage += "<span style='color: #007bff;'>&#128193;</span> "; // フォルダアイコン
      }
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
    webpage += "<tr><td colspan='2' style='text-align: center; padding: 20px;'>No files or directories found in " + SdPath + " to rename.</td></tr>";
  }
  webpage += "</tbody>";
  webpage += "</table><br>";

  // 送信ボタン
  webpage += "<input type='submit' value='Rename Selected'>";
  webpage += "</form>";
  webpage += HTML_Footer();
}

void SD_Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args)
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
    webpage += "<h3>SD: Rename Error - No file selected.</h3>";
    webpage += "<a href='/SD_rename'>[Back]</a><br><br>";
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
    webpage += "<h3>SD: Rename Error - Internal error processing form data.</h3>";
    webpage += "<a href='/SD_rename'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    request->send(200, "text/html", webpage); // エラーページを送信して終了
    return;
  }

  Serial.println("Rename requested:");
  Serial.println("  Old filename (from choice): " + oldfilename_form);
  Serial.println("  New filename (from form): " + newfilename_form);
  Serial.println("  Current Path (SdPath): " + SdPath);

  // --- 3. 以降の入力チェックとリネーム処理 ---
  // 入力チェック (newfilename が空でないかもチェック)
  if (oldfilename_form == "" || newfilename_form == "") // newfilenameもチェック
  {
    webpage += "<h3>SD: Rename Error - File selected but new name is missing.</h3>";
    webpage += "<a href='/SD_rename'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    request->send(200, "text/html", webpage); // ★ send を追加
    return;
  }
  if (newfilename_form.indexOf('/') != -1 || newfilename_form.indexOf('\\') != -1)
  {
    webpage += "<h3>SD: Rename Error - New filename cannot contain '/' or '\'.</h3>";
    webpage += "<a href='/SD_rename'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    request->send(200, "text/html", webpage); // ★ send を追加
    return;
  }
  if (oldfilename_form == newfilename_form)
  {
    webpage += "<h3>SD: Rename Error - New filename is the same as the old filename.</h3>";
    webpage += "<a href='/SD_rename'>[Back]</a><br><br>";
    webpage += HTML_Footer();
    request->send(200, "text/html", webpage); // ★ send を追加
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
  File currentItem = SD.open(oldfilepath, "r");

  if (currentItem)
  {
    currentItem.close();
    if (SD.exists(newfilepath))
    {
      webpage += "<h3>SD: Rename Error - New filename '" + newfilename_form + "' already exists in " + SdPath + ".</h3>";
      webpage += "<a href='/SD_rename'>[Back]</a><br><br>";
    }
    else
    {
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
  else
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

void SD_Select_File_For_ViewText()
{
  SDdir_FilesList();

  webpage = HTML_Header();
  webpage += "<h3>SD: Select a Text File to View (" + SdPath + ")</h3>";

  // 許可する拡張子のリスト (小文字で定義)
  const std::vector<String> allowedExtensions = {
      ".txt", ".log", ".csv", ".json", ".yaml", ".htm", ".html", ".css", ".js", ".xml", ".md", ".ini", ".conf", ".cfg", ".c", ".h", ".cpp", ".hpp", ".py", ".inc"
      // 必要に応じて他のテキストベースの拡張子を追加する
  };

  int displayedFileCount = 0; // 表示したファイルの数をカウント

  if (SD_numfiles > 0) // SD_numfiles は SDdir_FilesList() で設定されたファイル総数
  {
    webpage += "<table class='file-list-table'>";
    webpage += "<tbody>";

    // 取得した全ファイルをループ
    for (int index = 0; index < SD_numfiles; index++)
    {
      String Fname_orig = SD_Filenames[index].filename;
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
        displayedFileCount++;
        String Fname_encoded = urlEncode(Fname_orig);
        String link_url = "/SD_view_text?file=" + Fname_encoded;

        webpage += "<tr class='file-entry'>";
        webpage += "<td class='file-name'>";
        webpage += "<button style='width: 100%; text-align: left; padding: 5px; box-sizing: border-box; white-space: normal; word-break: break-all;'>";
        webpage += "<a href='" + link_url + "' style='display: block; text-decoration: none; color: inherit;'>" + Fname_orig + "</a>";
        webpage += "</button>";
        webpage += "</td>";
        webpage += "<td class='file-size'>" + SD_Filenames[index].fsize + "</td>";
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
    webpage += "<p style='text-align: center; margin-top: 20px;'>No text files found in " + SdPath + "</p>";
  }

  webpage += HTML_Footer();
}

void SD_View_Text(AsyncWebServerRequest *request, String encoded_filename)
{
  String decoded_filename = urlDecode(encoded_filename);
  String fullPath = decoded_filename;
  if (!fullPath.startsWith("/"))
    fullPath = "/" + fullPath;
  if (SdPath != "/")
    fullPath = SdPath + fullPath;

  Serial.println("Viewing text file: " + fullPath);

  File file = SD.open(fullPath, "r");
  if (!file || file.isDirectory())
  {
    if (file)
      file.close();
    webpage = HTML_Header();
    webpage += "<h3>Error: Cannot view file</h3>";
    webpage += "<p>File not found or is a directory: " + decoded_filename + "</p>";
    webpage += "<a href='/SD_dir'>[Back to Directory]</a>";
    webpage += HTML_Footer();
    request->send(404, "text/html", webpage);
    return;
  }

  webpage = HTML_Header();
  webpage += "<h3>Viewing Text: " + decoded_filename + " (" + SdPath + ")</h3>";
  webpage += "<pre>"; // preタグで囲む

  // ファイル内容を読み込んで webpage に追加
  // 大きなファイルの場合、メモリに注意が必要。
  // ここでは一括読み込みをしている。
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
  webpage += "<br><a href='/SD_dir'>[Back to Directory]</a>";
  webpage += "<br>";
  webpage += HTML_Footer();
  request->send(200, "text/html", webpage);
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
    SDdir_DirMake();
    request->send(200, "text/html", webpage); });

  // GETで受ける場合 (SDdir_InputNewDirName からの遷移)
  server.on("/SDdir_mkdirhandler", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        Serial.println("SDdir_mkdir handler (GET) started...");
        SDdir_Handle_mkdir(request);
        request->send(200, "text/html", webpage); });

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
      webpage += "<span style='color: #007bff;'>&#128193;</span> " + Dname_orig;
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
  // webpage += "<p>in path: " + SdPath + "?</p>";
  webpage += "<p>in path:&nbsp;「&nbsp;" + SdPath + "&nbsp;」&nbsp;?</p>";
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
