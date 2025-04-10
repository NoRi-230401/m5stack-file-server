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
void SD_Handle_File_Delete(String filename);
void SD_File_Rename();
void SD_Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args);
bool SD_notFound(AsyncWebServerRequest *request);
void SD_Select_File_For_Function(String title, String function);
// -------------------------------------------------------
void SDdir_flserverSetup();
void SDdir_handle_chTop();
void SDdir_handle_chUp();
void SDdir_Select_Dir_For_Function(String title, String function);
void SDdir_Handle_chdir(String filename);
void SDdir_Handle_rmdir(String filename);
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
    SD_Select_File_For_Function("[DOWNLOAD] for PC", "SD_downloadhandler"); // Build webpage ready for display
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
      // Serial.println("Transferred : " + String(len) + " Bytes"); // ログが多いのでコメントアウト推奨
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

void SD_Handle_File_Delete(String filename)
{
  webpage = HTML_Header();
  String fullPath = filename; // filename は既に '/' なしの想定

  if (!fullPath.startsWith("/"))
    fullPath = "/" + fullPath;

  if (SdPath != "/")
    fullPath = SdPath + fullPath;

  Serial.println("Delete target filename = " + fullPath);
  File dataFile = SD.open(fullPath, "r");

  if (dataFile)
  {
    dataFile.close(); // ファイルを閉じてから削除
    if (SD.remove(fullPath))
    {
      webpage += "<h3>SD: File '" + filename + "' in " + SdPath + " has been deleted</h3>"; // パスも表示
      webpage += "<a href='/SD_dir'>[Enter]</a><br><br>";
    }
    else
    {
      webpage += "<h3>SD: Failed to delete file [ " + filename + " ] in " + SdPath + "</h3>";
      webpage += "<a href='/SD_dir'>[Enter]</a><br><br>";
    }
  }
  else
  {
    webpage += "<h3>SD: File [ " + filename + " ] in " + SdPath + " does not exist</h3>";
    webpage += "<a href='/SD_dir'>[Enter]</a><br><br>";
  }
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
  webpage += "<th>File name</th>";    // 見出し1
  webpage += "<th>New Filename</th>"; // 見出し2
  webpage += "<th>Select</th>";       // 見出し3
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
        break; // 最初に見つかったものを採用
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
        webpage += "<a href='/SD_dir'>[OK]</a><br><br>"; // 成功したらDir表示へ
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
  String filename = ""; // ハンドラ内で必要に応じて設定される
  String url = request->url();

  // URLデコードが必要な場合があるためデコード処理を追加
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
  if (url.startsWith("/SD_downloadhandler"))
  {
    Serial.println("SD_Download handler started for: " + filename);
    SD_startTime = millis();

    String fullPath = filename;
    if (!fullPath.startsWith("/"))
      fullPath = "/" + fullPath;
    if (SdPath != "/")
      fullPath = SdPath + fullPath;

    Serial.println("Download target full path = " + fullPath);
    File file = SD.open(fullPath, "r");

    if (file && !file.isDirectory())
    {                                                  // ファイルが存在し、ディレクトリでないことを確認
      String contentType = getContentType("download"); // 強制ダウンロード
      AsyncWebServerResponse *response = request->beginResponse(
          contentType,
          file.size(),
          [file](uint8_t *buffer, size_t maxLen, size_t index) mutable -> size_t
          {
            // ファイルを閉じる前に読み取りを完了させる
            size_t bytesRead = file.read(buffer, maxLen);
            if (index + bytesRead == file.size())
            {
              // 最後の読み取り後にファイルを閉じる
              // file.close(); // beginResponseのラムダ内でcloseすると問題が起きる可能性があるため、ここではcloseしない
            }
            return bytesRead;
          });
      // ファイルが閉じられるようにコールバックを設定 (レスポンス送信完了後)
      response->setContentLength(file.size());                                                 // ファイルサイズを明示的に設定
      response->addHeader("Content-Disposition", "attachment; filename=\"" + filename + "\""); // ダウンロードファイル名を指定
      response->addHeader("Server", "ESP Async Web Server");
      request->send(response);

      // ファイルサイズ取得と時間計測 (ここではファイルはまだ開いている可能性がある)
      SD_downloadSize = file.size(); // getFileSizeを再度呼ぶ代わりに保持しているfileオブジェクトから取得
      SD_downloadTime = millis() - SD_startTime;
      Serial.println("SD download handler initiated...");
      // 注意: ラムダ内でファイルを閉じるとクラッシュする可能性があるため、
      // AsyncWebServerがレスポンス送信後に自動で閉じることを期待するか、
      // onDisconnect等で閉じる処理が必要になる場合がある。
      // beginResponse(Stream) を使う方が安全かもしれない。
      // file.close(); // ここでは閉じない
    }
    else
    {
      if (file)
        file.close(); // ディレクトリだった場合や開けなかった場合に閉じる
      Serial.println("Error: File not found or is a directory: " + fullPath);
      request->send(404, "text/plain", "File not found or is a directory");
    }
    return true; // ハンドルされた
  }
  else if (url.startsWith("/SD_streamhandler"))
  {
    Serial.println("SD_Stream handler started for: " + filename);
    SD_startTime = millis();

    String fullPath = filename;
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
        // ファイルが存在し、ディレクトリでない場合のみストリーミング
        String contentType = getContentType(filename); // ファイル拡張子に基づくContent-Type
        AsyncWebServerResponse *response = request->beginResponse(SD, fullPath, contentType);
        // beginResponse(FS, path, contentType) は内部でファイルを開閉するため安全
        SD_downloadSize = file.size(); // サイズ取得のために一度開く必要がある
        file.close();                  // サイズ取得後に閉じる
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
    return true; // ハンドルされた
  }
  else if (url.startsWith("/SD_deletehandler"))
  {
    Serial.println("SD_Delete handler started for: " + filename);
    SD_Handle_File_Delete(filename); // ファイル名のみ渡す
    request->send(200, "text/html", webpage);
    return true; // ハンドルされた
  }
  else if (url.startsWith("/SD_renamehandler"))
  {
    Serial.println("SD Rename handler started...");
    // filename は使わず、フォームの引数から処理する
    SD_Handle_File_Rename(request, "", request->args()); // filename引数は空文字で渡す
    request->send(200, "text/html", webpage);
    return true; // ハンドルされた
  }

  return false; // 上記のいずれにも一致しない場合はハンドルされなかった
}

void SD_Select_File_For_Function(String title, String function)
{
  SDdir_FilesList(); // ファイルのみリストアップ

  webpage = HTML_Header();
  webpage += "<h3>SD: Select a File to " + title + " (" + SdPath + ")</h3>"; // パス表示を追加

  if (SD_numfiles > 0)
  {
    //(クラス file-list-table )、PCではflexboxによる2列表示、スマホでは通常のテーブル表示になる
    webpage += "<table class='file-list-table'>";
    webpage += "<tbody>";

    for (int index = 0; index < SD_numfiles; index++)
    {
      String Fname_orig = SD_Filenames[index].filename; // 元のファイル名 (表示用)
      String Fname_url = Fname_orig;                    // URL生成用のファイル名

      // URLエンコードが必要な文字を処理 (簡易版: スペースのみ)
      // より堅牢にするには、他の文字 (% $ & + , / : ; = ? @ など) もエンコードが必要
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
      // 必要に応じて他の文字も追加

      // 各ファイルエントリの行 (クラス file-entry を追加)、PCでは各エントリが幅50%になる
      webpage += "<tr class='file-entry'>";

      // ファイル名セル (ボタン付きリンク) - file-name クラスを使用
      // PC表示(flex)では、このセルが伸縮して幅を調整する
      webpage += "<td class='file-name'>";
      // ボタンにスタイルを追加して、セル内で適切に表示されるように調整
      webpage += "<button style='width: 100%; text-align: left; padding: 5px; box-sizing: border-box; white-space: normal; word-break: break-all;'>";
      // 幅100%, 左寄せ, パディング, 折り返し有効
      webpage += "<a href='" + function + "~/" + Fname_encoded + "' style='display: block; text-decoration: none; color: inherit;'>" + Fname_orig + "</a>";
      // リンクスタイル調整

      webpage += "</button>";
      webpage += "</td>";

      // ファイルサイズセル - file-size クラスを使用
      // PC表示(flex)では、このセルが指定された幅(25%)を維持する
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
// SDdir_* 関数群 (変更なし)
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

void SDdir_Handle_chdir(String filename)
{
  webpage = HTML_Header();
  String targetPath = filename; // filename は '/' なしのディレクトリ名想定

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

void SDdir_Handle_rmdir(String filename)
{
  webpage = HTML_Header();
  String targetPath = filename; // filename は '/' なしのディレクトリ名想定

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
  // HTMLフォームを表示する関数を呼び出す
  SDdir_InputNewDirName("Make New Directory in " + SdPath, "SDdir_mkdirhandler", "filename");
}

void SDdir_Select_Dir_For_Function(String title, String function)
{
  SDdir_DirList(); // ディレクトリのみリストアップ
  webpage = HTML_Header();
  webpage += "<h3>SD: Select a Directory to " + title + " (" + SdPath + ")</h3>"; // パス表示

  if (SD_numfiles > 0)
  {
    // file-list-table を使用してレスポンシブ表示
    webpage += "<table class='file-list-table'>";
    webpage += "<tbody>";

    for (int index = 0; index < SD_numfiles; index++)
    {
      String Dname_orig = SD_Filenames[index].filename; // 表示用ディレクトリ名
      String Dname_url = Dname_orig;                    // URL用

      // URLエンコード (簡易版)
      String Dname_encoded = Dname_url;
      Dname_encoded.replace(" ", "%20");
      Dname_encoded.replace("!", "%21");
      Dname_encoded.replace("#", "%23");
      Dname_encoded.replace("$", "%24");
      Dname_encoded.replace("%", "%25");
      Dname_encoded.replace("&", "%26");
      Dname_encoded.replace("'", "%27");
      Dname_encoded.replace("(", "%28");
      Dname_encoded.replace(")", "%29");
      Dname_encoded.replace("*", "%2A");
      Dname_encoded.replace("+", "%2B");
      Dname_encoded.replace(",", "%2C");

      // 各ディレクトリのエントリ行
      webpage += "<tr class='file-entry'>";

      // ディレクトリ名セル (ボタン付きリンク) - file-name クラスを使用
      webpage += "<td class='file-name'>";
      webpage += "<button style='width: 100%; text-align: left; padding: 5px; box-sizing: border-box; white-space: normal; word-break: break-all;'>";
      webpage += "<a href='" + function + "~/" + Dname_encoded + "' style='display: block; text-decoration: none; color: inherit;'>";
      webpage += "<span style='color: #007bff;'>&#128193;</span> " + Dname_orig; // フォルダアイコンを追加
      webpage += "</a>";
      webpage += "</button>";
      webpage += "</td>";

      // ディレクトリの場合はサイズセルは空にするか、タイプを表示
      webpage += "<td class='file-size'>Directory</td>"; // file-size クラスを使うが内容は変更

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
  // Serial.println("Listing files in: " + SdPath); // ログが多いのでコメントアウト推奨
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
  String filename = "";
  String url = request->url();

  // URLデコード
  String decodedUrl = url;
  decodedUrl.replace("%20", " ");
  decodedUrl.replace("%21", "!");
  decodedUrl.replace("%23", "#");
  decodedUrl.replace("%24", "$");
  decodedUrl.replace("%25", "%");
  decodedUrl.replace("%26", "&");
  decodedUrl.replace("%27", "'");
  decodedUrl.replace("%28", "(");
  decodedUrl.replace(")", "%29");
  decodedUrl.replace("*", "%2A");
  decodedUrl.replace("+", "%2B");
  decodedUrl.replace(",", "%2C");

  int separatorIndex = decodedUrl.indexOf("~/");
  if (separatorIndex != -1)
  {
    filename = decodedUrl.substring(separatorIndex + 2);
  }

  if (url.startsWith("/SDdir_chdirhandler"))
  {
    Serial.println("SDdir_chdir handler started for: " + filename);
    SDdir_Handle_chdir(filename); // ディレクトリ名のみ渡す
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
  else if (url.startsWith("/SDdir_rmdirhandler"))
  {
    Serial.println("SDdir_rmdir handler started for: " + filename);
    SDdir_Handle_rmdir(filename); // ディレクトリ名のみ渡す
    request->send(200, "text/html", webpage);
    return true;
  }

  return false; // 上記以外はハンドルしない
}

void SDdir_InputNewDirName(String Heading, String Command, String Arg_name)
{
  webpage = HTML_Header();
  webpage += "<h3>" + Heading + "</h3>";
  // フォームの action を修正
  webpage += "<form method='GET' action='/" + Command + "'>";                                                                                                   // method='GET' を明示
  webpage += "New Directory Name: <input type='text' name='" + Arg_name + "' required pattern='[^/\\]+' title='Directory name cannot contain / or \'><br><br>"; // 簡単な入力検証を追加
  webpage += "<input type='submit' value='Create'>";                                                                                                            // ボタンのラベルを修正
  webpage += "</form>";
  webpage += "<br><a href='/SD_dir'>[Cancel and Back to Directory]</a>"; // キャンセルリンク
  webpage += HTML_Footer();
}
