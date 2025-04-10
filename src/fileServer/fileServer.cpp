// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-01
// -------------------------------------------------------
// fileServer.cpp
// *******************************************************
#include "fileServer.h"

String HTML_Header();
void Display_System_Info();
bool fileServerStart();
void notFound(AsyncWebServerRequest *request);
void Page_Not_Found();
void Home();
String HTML_Footer();
String getContentType(String filenametype);
String EncryptionType(wifi_auth_mode_t encryptionType);
bool compareFileinfo(const fileinfo &a, const fileinfo &b);
uint64_t getFileSize(int flType, String filename);
bool FS_start(int flType);
bool SD_cardInfo(void);
// -------------------------------------------------------
extern bool SPIFFS_notFound(AsyncWebServerRequest *request);
extern void SPIFFS_flServerSetup();
extern void SPIFFS_Directory();
extern uint32_t SPIFFS_startTime, SPIFFS_downloadTime, SPIFFS_uploadTime;
extern uint64_t SPIFFS_downloadSize, SPIFFS_uploadSize;
extern uint32_t SPIFFS_numfiles;
// -------------------------------------------------------
extern bool SD_notFound(AsyncWebServerRequest *request);
extern void SD_flServerSetup();
extern void SD_Directory();
extern uint32_t SD_startTime, SD_downloadTime, SD_uploadTime;
extern uint64_t SD_downloadSize, SD_uploadSize;
extern uint32_t SD_numfiles;
extern bool SDdir_notFound(AsyncWebServerRequest *request);
extern void SDdir_flserverSetup();
// -------------------------------------------------------
String SSID, SSID_PASS, SERVER_NAME, IP_ADDR;
bool SD_ENABLE, SPIFFS_ENABLE;
const String HOME_IMG = "/homeImg.gif";

AsyncWebServer server(80);
String webpage;

// String HTML_Header()
// {
//   String page;
//   page = "<!DOCTYPE html>";
//   page += "<html lang = 'ja'>";
//   page += "<head>";
//   page += "<title>" + SERVER_NAME + "</title>";
//   page += "<base target='_self'>";
//   page += "<meta charset='UTF-8'>";
//   // favicon
//   page += "<link rel='icon' href='/favicon.ico'>";
//   page += "<meta name='viewport' content='width=device-width,initial-scale=1.0'>";
//   // ---javaScript ---------------------------
//   page += "<script>";
//   page += "function confirmP() {if (confirm('Can I turn off?')){window.open('/shutdown', '_blank');} else {alert('stopped');}}";
//   page += "function confirmR() {if (confirm('Can I reboot?')){window.open('/shutdown?reboot=on', '_blank');} else {alert('stopped');}}";
//   page += "</script>";
//   // ------------------------------------------
//   page += "<style>";
//   // for smartPhone style define
//   page += "@media screen and (max-width: 480px) {";
//   page += "img {width:100%;height:auto;} body {font-size: 1.4rem;} div {font-size: 1.4rem;}";
//   page += "p {font-size: 1.4rem;} h5 {font-size: 1.4rem;}";
//   page += "}";
//   // -----
//   page += "html {font-size: 62.5%;}";
//   page += "body {width:100%;margin-left:auto;margin-right:auto;font-family:Arial,Helvetica,sans-serif;font-size:1.4rem;color:#2f4f4f;background-color:#fffacd;text-align:center;}";
//   page += "footer {padding:1.0rem;background-color:cyan;font-size:1.4rem;}";
//   page += "table {font-family:arial,sans-serif;border-collapse:collapse;width:80%;}";
//   page += "table.center {margin-left:auto;margin-right:auto;}";
//   page += "td, th {border:1px solid #dddddd;text-align:left;padding:0.8rem;}";
//   page += "tr:nth-child(even) {background-color:#dddddd;}";
//   page += "h3 {color:#6ecf12;font-size:1.7rem;font-style:normal;text-align:center;}";
//   page += "h4 {color:slateblue;font-size:1.5rem;text-align:left;font-style:oblique;text-align:center;}";
//   page += ".center {margin-left:auto;margin-right:auto;}";

//   // TOPNAV
//   page += ".topnav {overflow: visible;background-color:lightPink;}";
//   // page += ".topnav {overflow: visible;background-color:cyan;}";
//   page += ".topnav a {float:center;color:blue;text-align:center;padding:1.0rem 1.0rem;text-decoration:none;font-size:1.4rem;}";
//   page += ".topnav a:hover {background-color:deepskyblue;color:white;}";
//   page += ".topnav a.active {background-color:lightblue;color:blue;}";

//   // TOPNAV2
//   page += ".topnav2 {overflow: visible;background-color:lightcyan;}";
//   // page += ".topnav2 a {float:center;color:blue;text-align:center;padding:1.2rem 1.2rem;text-decoration:none;font-size:1.5rem;}";
//   page += ".topnav2 a {float:center;color:blue;text-align:center;padding:1.2rem 1.2rem;text-decoration:none;font-size:1.5rem;line-height:2;}";
//   page += ".topnav2 a:hover {background-color:deepskyblue;color:white;}";
//   page += ".topnav2 a.active {background-color:lightblue;color:blue;}";

//   // other style
//   page += ".notfound {padding:0.8rem;text-align:center;font-size:1.3rem;}";
//   page += ".left {text-align:left;}";
//   page += ".medium {font-size:1.9rem;padding:0;margin:0}";
//   page += ".ps {font-size:1.4rem;padding:0;margin:0}";
//   page += ".sp {background-color:silver;white-space:nowrap;width:2%;}";
//   // --- end of style ---
//   page += "</style></head><body>";

//   // -- 1 --
//   page += "<div class = 'topnav'>";
//   page += "<a href='/'>Home</a>";
//   // page += "&nbsp;";
//   page += "<a href='/system'>Status</a>";
//   page += "&nbsp;";
//   page += "　";
//   // page += "<a href='/shutdown?reboot=on' target='_blank'>reboot</a>";
//   // page += "<a href='/shutdown' target='_blank'>powOff</a>";
//   page += "<input type='button' value='Reboot' onclick='confirmR();'>";
//   page += "　";
//   page += "<input type='button' value='PowOff' onclick='confirmP();'>";

//   page += "</div>";

//   // --------------- SPIFFS ------------------------
//   if (SPIFFS_ENABLE)
//   {
//     // -- 2 SPIFFS --
//     page += "<br>";
//     page += "<div class = 'topnav2'>";
//     page += "SPIFFS:<a href='/SPIFFS_dir'>Dir</a>";
//     page += "<a href='/SPIFFS_upload'>Upload</a> ";
//     page += "<a href='/SPIFFS_download'>Download</a>";
//     page += "<a href='/SPIFFS_stream'>Stream</a>";
//     page += "<a href='/SPIFFS_delete'>Delete</a>";
//     page += "<a href='/SPIFFS_rename'>Rename</a>";
//     page += "</div>";
//   }
//   page += "<br>";

//   // ------------------ SD -------------------------
//   if (SD_ENABLE)
//   {
//     // -- 3 SD --
//     page += "<div class = 'topnav2'>";
//     page += "SD:<a href='/SD_dir'>Dir</a>";
//     page += "<a href='/SD_upload'>Upload</a> ";
//     page += "<a href='/SD_download'>Download</a>";
//     page += "<a href='/SD_stream'>Stream</a>";
//     page += "<a href='/SD_delete'>Delete</a>";
//     page += "<a href='/SD_rename'>Rename</a>";
//     // page += "</div>";

//     page += "<br>";

//     // -- 4 SD path --
//     // page += "<div class = 'topnav2'>";
//     page += "path:&nbsp;" + SdPath;
//     page += "<a href='/SDdir_chTop'>Top</a>";
//     page += "<a href='/SDdir_chUp'>Up</a>";
//     // page += "</div>";
//     // -- 5 SD dir --
//     // page += "<div class = 'topnav2'>";
//     page += "<a href='/SDdir_chdir'>Chdir</a>";
//     page += "<a href='/SDdir_mkdir'>Mkdir</a>";
//     page += "<a href='/SDdir_rmdir'>Rmdir</a>";
//     page += "</div>";
//   }

//   // page += "<br>";
//   return page;
// }

String HTML_Header()
{
  String page;
  page = "<!DOCTYPE html>";
  page += "<html lang = 'ja'>";
  page += "<head>";
  page += "<title>" + SERVER_NAME + "</title>";
  page += "<base target='_self'>";
  page += "<meta charset='UTF-8'>";
  // favicon
  page += "<link rel='icon' href='/favicon.ico'>";
  page += "<meta name='viewport' content='width=device-width,initial-scale=1.0'>";
  // ---javaScript ---------------------------
  page += "<script>";
  page += "function confirmP() {if (confirm('Can I turn off?')){window.open('/shutdown', '_blank');} else {alert('stopped');}}";
  page += "function confirmR() {if (confirm('Can I reboot?')){window.open('/shutdown?reboot=on', '_blank');} else {alert('stopped');}}";
  page += "</script>";
  // ------------------------------------------
  page += "<style>";

  // --- ファイル一覧テーブル用の基本スタイル (スマホ表示) ---
  page += ".file-list-table { width: 95%; border-collapse: collapse; margin-left:auto; margin-right:auto; border: 1px solid #ccc; }";
  // theadはPC表示でのみ使用する想定だが、念のためスタイル定義
  page += ".file-list-table thead th { border: 1px solid #ddd; text-align: left; padding: 8px; box-sizing: border-box; background-color: #e9e9e9; font-weight: bold; }";
  page += ".file-list-table tbody td { border: 1px solid #ddd; text-align: left; padding: 8px; box-sizing: border-box; vertical-align: top; }";
  // 列幅指定 (クラスセレクタ使用) - スマホ表示時のデフォルト
  page += ".file-list-table td.file-type { width: 15%; }";
  page += ".file-list-table td.file-name { width: 60%; word-break: break-all; }"; // スマホでも折り返し
  page += ".file-list-table td.file-size { width: 25%; text-align: right; }";
  // 交互背景色 (スマホ表示用)
  page += ".file-list-table tbody tr:nth-child(even) { background-color: #f8f8f8; }";
  page += ".file-list-table tbody tr:nth-child(odd) { background-color: #ffffff; }";

  // --- PC向けスタイル (画面幅が 769px 以上の場合) ---
  page += "@media screen and (min-width: 769px) {";
  page += "  .file-list-table { width: 80%; border: none; }"; // PCではテーブル全体のボーダーを消す
  // PC表示ではtheadは表示しない (tbody内で完結させるため)
  page += "  .file-list-table thead { display: none; }";
  page += "  .file-list-table > tbody { ";
  page += "    display: flex; ";
  page += "    flex-wrap: wrap; ";
  page += "    border: none; "; // tbodyのボーダーも消す
  page += "  }";
  page += "  .file-list-table > tbody > tr.file-entry { ";
  page += "    display: flex; ";
  page += "    width: 50%; "; // 2列表示
  page += "    box-sizing: border-box; ";
  page += "    border: none; "; // trのボーダーも消す
  page += "    background-color: transparent !important; "; // trの背景色はtdで制御
  page += "  }";
  page += "  .file-list-table > tbody > tr.file-entry > td { ";
  page += "    border: none; "; // tdのボーダー基本消す
  page += "    border-bottom: 1px solid #eee; "; // 下線のみ表示
  page += "    padding: 8px; ";
  page += "    box-sizing: border-box; ";
  page += "    background-color: transparent !important; "; // tdの背景色はtrの交互色で制御
  page += "    text-align: left; "; // 基本左寄せ
  page += "    vertical-align: top; ";
  page += "    flex-grow: 0; ";
  page += "    flex-shrink: 0; ";
  page += "  }";

  // 各ファイル情報内の 'Type' と 'Name' の右側に薄い区切り線を追加 (左右両方に適用)
  page += "  .file-list-table > tbody > tr.file-entry > td.file-type, ";
  page += "  .file-list-table > tbody > tr.file-entry > td.file-name { ";
  page += "    border-right: 1px solid #eee; "; // 薄い右境界線
  page += "  }";

  // 左側ファイル情報の 'Size' の右側にも区切り線を追加 (左右のファイル間の区切り)
  page += "  .file-list-table > tbody > tr.file-entry:nth-child(odd) > td.file-size { ";
  page += "    border-right: 2px solid #ccc; "; // 少し濃い右境界線
  page += "  }";

  // 右側ファイル情報の 'Size' の右側には線は不要 (テーブル右端のため)
  page += "  .file-list-table > tbody > tr.file-entry:nth-child(even) > td.file-size { ";
  page += "    border-right: none; ";
  page += "  }";

  // データセルの幅指定 (flex-basis)
  page += "  .file-list-table > tbody > tr.file-entry > td.file-type { flex-basis: 15%; }";
  page += "  .file-list-table > tbody > tr.file-entry > td.file-name { flex-basis: 60%; flex-grow: 1; word-break: break-all; }"; // ファイル名は伸縮可能に
  page += "  .file-list-table > tbody > tr.file-entry > td.file-size { flex-basis: 25%; text-align: right; }"; // サイズは右寄せ

  // PC表示用の交互背景色 (2行ごと = 左右ペアごと)
  page += "  .file-list-table > tbody > tr.file-entry:nth-child(4n-1), "; // 左列の奇数行ペア
  page += "  .file-list-table > tbody > tr.file-entry:nth-child(4n) { ";   // 右列の奇数行ペア
  page += "    background-color: #f8f8f8 !important; "; // 薄いグレー
  page += "  }";
  page += "  .file-list-table > tbody > tr.file-entry:nth-child(4n-3), "; // 左列の偶数行ペア
  page += "  .file-list-table > tbody > tr.file-entry:nth-child(4n-2) { "; // 右列の偶数行ペア
  page += "    background-color: #ffffff !important; "; // 白
  page += "  }";
  page += "}"; // PC向けメディアクエリ終了

  // --- スマホ向けスタイル (画面幅が 768px 以下の場合) ---
  // PC向けスタイルを上書きして通常のテーブル表示に戻す
  page += "@media screen and (max-width: 768px) {";
  // 基本フォントサイズなど
  page += "  body {font-size: 1.4rem;} div {font-size: 1.4rem;}";
  page += "  p {font-size: 1.4rem;} h5 {font-size: 1.4rem;}";
  page += "  img {max-width:100%;height:auto;}"; // 画像ははみ出さないように
  page += "  .file-list-table { width: 95%; font-size: 1.2rem; border: 1px solid #ccc; }"; // テーブル全体のボーダー復活
  // theadはスマホでも非表示のまま (tbodyのみ使用)
  page += "  .file-list-table thead { display: none; }";
  // PC向けflexレイアウトを解除
  page += "  .file-list-table > tbody { display: table-row-group; border: none; }"; // 通常のtbody表示に戻す
  page += "  .file-list-table > tbody > tr.file-entry { display: table-row; width: 100%; border: none; }"; // 通常のtr表示に戻す
  // tdのスタイルを通常のテーブルセルに戻す
  page += "  .file-list-table > tbody > tr.file-entry > td { ";
  page += "    display: table-cell; "; // 通常のセル表示
  page += "    width: auto; ";         // 幅はクラス指定に任せる
  page += "    border: none; ";        // PCで消したボーダーは基本なし
  page += "    border-bottom: 1px solid #ddd; "; // 下線のみ
  page += "    border-right: 1px solid #ddd; ";  // 右にも線を引く (最後のセル以外)
  page += "    flex-basis: auto; ";    // PCのflex指定をリセット
  page += "    flex-grow: 0; ";        // PCのflex指定をリセット
  page += "    flex-shrink: 1; ";      // PCのflex指定をリセット
  page += "    background-color: transparent !important; "; // 背景色はtrの交互色で制御
  page += "    text-align: left; ";    // 基本左寄せ
  page += "    vertical-align: top; ";
  page += "  }";
  // 最後のセル (file-size) の右ボーダーは不要
  page += "  .file-list-table > tbody > tr.file-entry > td.file-size { border-right: none; text-align: right; }"; // 右寄せ再指定
  // スマホでの交互背景色を再適用 (trに対して)
  page += "  .file-list-table > tbody > tr.file-entry:nth-child(even) { background-color: #f8f8f8 !important; }";
  page += "  .file-list-table > tbody > tr.file-entry:nth-child(odd) { background-color: #ffffff !important; }";
  page += "}"; // スマホ向けメディアクエリ終了

  // -------------- その他の共通スタイル --------------
  page += "html {font-size: 62.5%;}"; // remの基準
  page += "body {width:100%;margin: 0; padding: 0; font-family:Arial,Helvetica,sans-serif;font-size:1.6rem;color:#2f4f4f;background-color:#fffacd;text-align:center;}"; // bodyのデフォルトサイズと余白調整
  page += "footer {padding:1.0rem;background-color:cyan;font-size:1.4rem;}";

  // --- 一般的なテーブルスタイル (ファイル一覧以外で使用する場合) ---
  page += "table:not(.file-list-table) {font-family:arial,sans-serif;border-collapse:collapse;width:90%; margin: 1em auto;}"; // ファイル一覧以外に適用、幅とマージン調整
  page += "table:not(.file-list-table) td, table:not(.file-list-table) th {border:1px solid #dddddd;text-align:left;padding:0.8rem;}";
  page += "table:not(.file-list-table) tr:nth-child(even) {background-color:#dddddd;}";
  page += "table.center {margin-left:auto;margin-right:auto;}"; // 中央寄せクラス

  // --- 見出し等のスタイル ---
  page += "h3 {color:#6ecf12;font-size:1.9rem;font-style:normal;text-align:center; margin: 1em 0;}"; // サイズとマージン調整
  page += "h4 {color:slateblue;font-size:1.7rem;text-align:center;font-style:normal; margin: 1em 0;}"; // サイズ、中央寄せ、マージン調整

  // --- 汎用クラス ---
  page += ".center {margin-left:auto;margin-right:auto;}";
  page += ".notfound {padding:1em;text-align:center;font-size:1.6rem;}"; // サイズ調整
  page += ".left {text-align:left;}";
  page += ".medium {font-size:1.9rem;padding:0;margin:0}";
  page += ".ps {font-size:1.4rem;padding:0;margin:0}";
  // page += ".sp {background-color:silver;white-space:nowrap;width:2%;}"; // file-list-tableでは使わない想定

  // --- TOPNAV スタイル ---
  // page += ".topnav {overflow: hidden; background-color:lightPink; padding: 5px 0; text-align: center;}";
  page += ".topnav {overflow: hidden; background-color:lightPink; padding: 3px 0; text-align: center;}";
  
  page += ".topnav a, .topnav input[type='button'] {display: inline-block; color:blue; text-align:center; padding:10px 12px; margin: 2px 5px; text-decoration:none; font-size:1.4rem; border: none; background-color: transparent; cursor: pointer; vertical-align: middle;}"; // ボタンもリンク風に、インラインブロック、マージン調整
  
  page += ".topnav a:hover, .topnav input[type='button']:hover {background-color:deepskyblue;color:white;}";
  page += ".topnav a.active {background-color:lightblue;color:blue;}";

  // --- TOPNAV2 スタイル ---
  // page += ".topnav2 {overflow: hidden; background-color:lightcyan; padding: 8px 0; text-align: center; line-height: 1.5;}";
  page += ".topnav2 {overflow: hidden; background-color:lightcyan; padding: 5px 0; text-align: center; line-height: 1.5;}";
  page += ".topnav2 a, .topnav2 span {display: inline-block; color:blue; text-align:center; padding:8px 10px; margin: 2px 4px; text-decoration:none; font-size:1.5rem; vertical-align: middle;}";
  
  page += ".topnav2 a:hover {background-color:deepskyblue;color:white;}";
  page += ".topnav2 a.active {background-color:lightblue;color:blue;}";

  page += ".topnav2 span { color: #555; }"; // path表示用のスタイル

  // --- ボタンの基本スタイル ---
  page += "input[type='button'], input[type='submit'] { padding: 8px 15px; font-size: 1.4rem; cursor: pointer; border: 1px solid #ccc; border-radius: 4px; background-color: #f0f0f0; margin: 5px;}";
  page += "input[type='button']:hover, input[type='submit']:hover { background-color: #e0e0e0; }";
  page += "button { padding: 8px 15px; font-size: 1.4rem; cursor: pointer; border: 1px solid #ccc; border-radius: 4px; background-color: #f0f0f0; margin: 5px;}";
  page += "button a { text-decoration: none; color: inherit; }"; // ボタン内のリンクスタイル解除
  page += "button:hover { background-color: #e0e0e0; }";

  // --- フォーム要素のスタイル ---
  page += "input[type='text'], input[type='file'] { padding: 8px; font-size: 1.4rem; border: 1px solid #ccc; border-radius: 4px; margin: 5px; box-sizing: border-box;}";
  page += "form { margin: 1em 0; }"; // フォームのマージン

  // --- end of style ---
  page += "</style></head><body>";

  // -- 1 -- Top Navigation (Home, Status, Reboot, PowerOff) --
  page += "<div class = 'topnav'>";
  page += "<a href='/'>Home</a>";
  page += "<a href='/system'>Status</a>";
  // page += "&nbsp;"; // スペースはCSSマージンで調整
  page += "　";
  page += "<input type='button' value='Reboot' onclick='confirmR();'>";
  page += "　";
  page += "<input type='button' value='PowOff' onclick='confirmP();'>";
  page += "</div>";
  page += "<br>";

  // --------------- SPIFFS Menu ------------------------
  if (SPIFFS_ENABLE)
  {
    page += "<div class = 'topnav2'>";
    page += "<span>SPIFFS:</span>"; // ラベル
    page += "<a href='/SPIFFS_dir'>Dir</a>";
    page += "<a href='/SPIFFS_upload'>Upload</a> ";
    page += "<a href='/SPIFFS_download'>Download</a>";
    page += "<a href='/SPIFFS_stream'>Stream</a>";
    page += "<a href='/SPIFFS_delete'>Delete</a>";
    page += "<a href='/SPIFFS_rename'>Rename</a>";
    page += "</div>";
  }
  page += "<br>"; // メニュー間のスペースはCSSマージンで調整

  // ------------------ SD Menu -------------------------
  if (SD_ENABLE)
  {
    // -- SD File Operations --
    page += "<div class = 'topnav2'>";
    page += "<span>SD:</span>"; // ラベル
    page += "<a href='/SD_dir'>Dir</a>";
    page += "<a href='/SD_upload'>Upload</a> ";
    page += "<a href='/SD_download'>Download</a>";
    page += "<a href='/SD_stream'>Stream</a>";
    page += "<a href='/SD_delete'>Delete</a>";
    page += "<a href='/SD_rename'>Rename</a>";
    page += "</div>";

    // page += "<br>"; // メニュー間のスペースはCSSマージンで調整

    // -- SD Path and Directory Operations --
    page += "<div class = 'topnav2'>";
    page += "<span>Path:&nbsp;" + SdPath + "</span>"; // 現在のパス表示
    page += "<a href='/SDdir_chTop'>Top</a>";
    page += "<a href='/SDdir_chUp'>Up</a>";
    page += "<a href='/SDdir_chdir'>Chdir</a>";
    page += "<a href='/SDdir_mkdir'>Mkdir</a>";
    page += "<a href='/SDdir_rmdir'>Rmdir</a>";
    page += "</div>";
  }

  page += "<br>"; // メインコンテンツとのスペース用
  return page;
}


void Display_System_Info()
{
  webpage = HTML_Header();
  webpage += "<h3>Status and System Information</h3>";
  webpage += "<br>";

  if (SPIFFS_ENABLE)
  {
    // - SPIFFS trx Statistics
    webpage += "<h4>SPIFFS:　Transfer Statistics</h4>";
    webpage += "<table class='center'>";
    webpage += "<tr><th>last upload</th><th>last download/stream</th><th>units</th></tr>";
    webpage += "<tr><td>" + ConvBytesUnits(SPIFFS_uploadSize, 1) + "</td><td>" + ConvBytesUnits(SPIFFS_downloadSize, 1) + "</td><td>File Size</td></tr> ";
    webpage += "<tr><td>" + ConvBytesUnits((float)SPIFFS_uploadSize / SPIFFS_uploadTime * 1000.0, 1) + "/Sec</td>";
    webpage += "<td>" + ConvBytesUnits((float)SPIFFS_downloadSize / SPIFFS_downloadTime * 1000.0, 1) + "/Sec</td><td>Transfer Rate</td></tr>";
    webpage += "</table>";
    webpage += "<br><br>";

    // - SPIFFS Filing-Sys
    webpage += "<h4>SPIFFS:　Filing System</h4>";
    webpage += "<table class='center'>";
    webpage += "<tr><th>total space</th><th>used space</th><th>free space</th><th>number of files</th></tr>";
    webpage += "<tr>";
    //-----------------------------------
    uint64_t SPIFFS_total = (uint64_t)SPIFFS.totalBytes();
    uint64_t SPIFFS_used = (uint64_t)SPIFFS.usedBytes();
    uint64_t SPIFFS_free = SPIFFS_total - SPIFFS_used;
    webpage += "<td>" + ConvBytesUnits(SPIFFS_total, 1, UNIT_KIRO) + "</td>";
    webpage += "<td>" + ConvBytesUnits(SPIFFS_used, 1, UNIT_KIRO) + "</td>";
    webpage += "<td>" + ConvBytesUnits(SPIFFS_free, 1, UNIT_KIRO) + "</td>";

    webpage += "<td>" + (SPIFFS_numfiles == 0 ? "Pending Dir or Empty" : String(SPIFFS_numfiles)) + "</td>";
    //-----------------------------------
    webpage += "</tr>";
    webpage += "</table>";
    webpage += "<br><br>";
  }

  if (SD_ENABLE)
  {
    // - SD trx Statistics
    webpage += "<h4>SD:　Transfer Statistics</h4>";
    webpage += "<table class='center'>";
    webpage += "<tr><th>last upload</th><th>last download/stream</th><th>units</th></tr>";
    webpage += "<tr><td>" + ConvBytesUnits(SD_uploadSize, 1) + "</td><td>" + ConvBytesUnits(SD_downloadSize, 1) + "</td><td>File Size</td></tr> ";
    webpage += "<tr><td>" + ConvBytesUnits((float)SD_uploadSize / SD_uploadTime * 1000.0, 1) + "/Sec</td>";
    webpage += "<td>" + ConvBytesUnits((float)SD_downloadSize / SD_downloadTime * 1000.0, 1) + "/Sec</td><td>Transfer Rate</td></tr>";
    webpage += "</table>";
    webpage += "<br>";

    // - SD Filing-Sys
    webpage += "<h4>SD:　Filing System</h4>";
    webpage += "<table class='center'>";
    webpage += "<tr><th>total space</th><th>used space</th><th>free space</th><th>card type</th></tr>";
    webpage += "<tr>";
    //-----------------------------------
    uint64_t SD_total = (uint64_t)SD.totalBytes();
    uint64_t SD_used = (uint64_t)SD.usedBytes();
    uint64_t SD_free = SD_total - SD_used;
    webpage += "<td>" + ConvBytesUnits(SD_total, 1) + "</td>";
    webpage += "<td>" + ConvBytesUnits(SD_used, 1) + "</td>";
    webpage += "<td>" + ConvBytesUnits(SD_free, 1) + "</td>";

    sdcard_type_t cardType = SD.cardType();
    const String cType[] = {"NONE", "MMC", "SD", "SDHC", "UNKNOWN"};
    webpage += "<td>" + cType[cardType] + "</td>";
    //-----------------------------------
    webpage += "</tr>";
    webpage += "</table>";
    webpage += "<br><br>";
  }

  // - program size
  webpage += "<h4>Program size in FLASH</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>total space</th><th>used program size</th><th>free space</th></tr>";
  webpage += "<tr>";
  //-----------------------------------
  uint64_t prog_max = (uint64_t)ESP.getFreeSketchSpace();
  uint64_t prog_used = (uint64_t)ESP.getSketchSize();
  uint64_t prog_available = prog_max - prog_used;
  webpage += "<td>" + ConvBytesUnits(prog_max, 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(prog_used, 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(prog_available, 1, UNIT_KIRO) + "</td>";
  //-----------------------------------
  webpage += "</tr>";
  webpage += "</table>";
  webpage += "<br><br>";

  //-------------------
  // - SRAM: Internal RAM
  webpage += "<h4>Internal RAM (SRAM)</h4><table class='center'>";
  webpage += "<tr><th>total heap size</th><th>free heap</th><th>min free heap<br>since boot</th><th>available max<br>allocate block</th></tr><tr>";
  webpage += "<td>" + ConvBytesUnits(ESP.getHeapSize(), 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(ESP.getFreeHeap(), 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(ESP.getMinFreeHeap(), 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(ESP.getMaxAllocHeap(), 1, UNIT_KIRO) + "</td>";
  webpage += "</tr></table>";
  //-------------------
  webpage += "<br><br>";

  // - PSRAM : External RAM
  webpage += "<h4>External RAM (PSRAM)</h4><table class='center'>";
  webpage += "<tr><th>total heap size</th><th>free heap</th><th>min free heap<br>since boot</th><th>available max<br>allocate block</th></tr><tr>";
  webpage += "<td>" + ConvBytesUnits(ESP.getPsramSize(), 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(ESP.getFreePsram(), 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(ESP.getMinFreePsram(), 1, UNIT_KIRO) + "</td>";
  webpage += "<td>" + ConvBytesUnits(ESP.getMaxAllocPsram(), 1, UNIT_KIRO) + "</td>";
  webpage += "</tr></table>";
  webpage += "<br><br>";
  //-------------------

  // - NVS
  nvs_stats_t nvsStats;
  // if (ESP_OK == nvs_get_stats("nvs", &nvsStats))
  if (ESP_OK == nvs_get_stats(NULL, &nvsStats))
  {
    webpage += "<h4>NVS : Non-Volatile Storage</h4>";
    webpage += "<table class='center'>";
    webpage += "<tr><th>available entries</th><th>used entries</th><th>free entries</th><th>name space</th></tr><tr>";

    size_t total_ent = nvsStats.total_entries;
    size_t used_ent = nvsStats.used_entries;
    size_t free_ent = nvsStats.free_entries;
    size_t namespace_cnt = nvsStats.namespace_count;
    webpage += "<td>" + String(total_ent) + "</td>";
    webpage += "<td>" + String(used_ent) + "</td>";
    webpage += "<td>" + String(free_ent) + "</td>";
    webpage += "<td>" + String(namespace_cnt) + "</td>";

    webpage += "</tr></table>";
    webpage += "<br><br>";
  }

  // - CPU information
  webpage += "<h4>CPU Information</h4><table class='center'>";
  webpage += "<tr><th>parameter</th><th>value</th></tr>";
  //-------------------
  webpage += "<tr><td>CPU Model</td><td>" + String(ESP.getChipModel()) + "</td></tr>";
  webpage += "<tr><td>Chip revision</td><td>" + String(ESP.getChipRevision()) + "</td></tr>";
  webpage += "<tr><td>SDK Version</td><td>" + String(ESP.getSdkVersion()) + "</td></tr>";

  webpage += "<tr><td>Number of Cores</td><td>" + String(ESP.getChipCores()) + "</td></tr>";
  // webpage += "<tr><td>Cycle Count</td><td>" + String(ESP.getCycleCount()) + "</td></tr>";

  webpage += "<tr><td>CPU Freq</td><td>" + String(ESP.getCpuFreqMHz()) + " MHz" + "</td></tr>";
  webpage += "<tr><td>Flash Memory Size</td><td>" + ConvBytesUnits(ESP.getFlashChipSize(), 0, UNIT_AUTO) + "</td></tr>";
  webpage += "<tr><td>Flash Freq</td><td>" + String(ESP.getFlashChipSpeed() / 1000000UL) + " MHz" + "</td></tr>";
  //-------------------
  webpage += "</table>";
  webpage += "<br><br>";

  // - MAC Address
  webpage += "<h4>MAC Address</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>parameter</th><th>value</th></tr>";
  //-------------------
  char buf[256];
  uint8_t mac0[6];
  uint64_t chipid;
  esp_read_mac(mac0, ESP_MAC_WIFI_STA);
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac0[0], mac0[1], mac0[2], mac0[3], mac0[4], mac0[5]);
  webpage += "<tr><td>WiFi STAtion MAC (default)</td><td>" + String(buf) + "</td></tr>";
  esp_read_mac(mac0, ESP_MAC_WIFI_SOFTAP);
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac0[0], mac0[1], mac0[2], mac0[3], mac0[4], mac0[5]);
  webpage += "<tr><td>WiFi softAP MAC</td><td>" + String(buf) + "</td></tr>";
  esp_read_mac(mac0, ESP_MAC_BT);
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac0[0], mac0[1], mac0[2], mac0[3], mac0[4], mac0[5]);
  webpage += "<tr><td>Bluetooth MAC</td><td>" + String(buf) + "</td></tr>";
  esp_read_mac(mac0, ESP_MAC_ETH);
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac0[0], mac0[1], mac0[2], mac0[3], mac0[4], mac0[5]);
  webpage += "<tr><td>Ethernet MAC</td><td>" + String(buf) + "</td></tr>";
  //-------------------
  webpage += "</table>";
  webpage += "<br><br>";

  // - Network Info
  webpage += "<h4>Network Information</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>parameter</th><th>value</th></tr>";
  webpage += "<tr><td>IP Address</td><td>" + String(WiFi.localIP().toString()) + "</td></tr>";
  webpage += "<tr><td>Server Name (hostName)</td><td>" + SERVER_NAME + "</td></tr>";
  webpage += "<tr><td>WiFi SSID</td><td>" + String(WiFi.SSID()) + "</td></tr>";
  webpage += "<tr><td>WiFi BSSID</td><td>" + String(WiFi.BSSIDstr()) + "</td></tr>";
  webpage += "<tr><td>WiFi Encryption Type</td><td>" + String(EncryptionType(WiFi.encryptionType(0))) + "</td></tr>";
  webpage += "</table> ";
  webpage += "<br><br>";

  // - clock
  webpage += "<h4>Clock</h4>";
  webpage += "<table class='center'>";
  webpage += "<tr><th>parameter</th><th>value</th></tr>";

  if (RTC_ENABLE)
    webpage += "<tr><td>Real Time Clock (RTC)</td><td>" + getTmRTC() + "</td></tr>";
  else
    webpage += "<tr><td>Real Time Clock (RTC)</td><td>　**　disable　**　</td></tr>";

  webpage += "<tr><td>Sync with NTP server</td><td>" + getTmNTP() + "</td></tr>";
  webpage += "</table> ";
  webpage += "<br><br>";

  // ------------------------------------------------------
  webpage += HTML_Footer();
}

bool fileServerStart()
{
  Serial.println(__FILE__);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  Serial.println("Home Page...");
  Home();
  request->send(200, "text/html", webpage); });

  server.on("/system", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  Display_System_Info();
  request->send(200, "text/html", webpage); });

  if (SPIFFS_ENABLE)
  {
    SPIFFS_flServerSetup();
    server.on("/SPIFFS_homeImg", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, HOME_IMG, "image/gif"); });
  }

  if (SD_ENABLE)
  {
    SD_flServerSetup();
    SDdir_flserverSetup();

    server.on("/SD_homeImg", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SD, HOME_IMG, "image/gif"); });
  }

  // favicon.ico
  if (SD_ENABLE && SD.exists("/favicon.ico"))
  {
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SD, "/favicon.ico", "image/x-icon"); });
  }
  else if (SPIFFS_ENABLE && SPIFFS.exists("/favicon.ico"))
  {
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/favicon.ico", "image/x-icon"); });
  }

  webApiSetup(); // shutdown API use

  server.onNotFound(notFound);

  // *** BEGIN SERVER ***
  server.begin();

  if (SPIFFS_ENABLE)
    SPIFFS_Directory();

  return true;
}

void notFound(AsyncWebServerRequest *request)
{
  Serial.println("notFound func : " + request->url());

  if (SPIFFS_ENABLE)
  {
    if (SPIFFS_notFound(request))
      return;
  }

  if (SD_ENABLE)
  {
    if (SD_notFound(request))
      return;

    if (SDdir_notFound(request))
      return;
  }

  Page_Not_Found();
  // request->send(200, "text/html", webpage);
  request->send(404, "text/html", webpage);
}

void Page_Not_Found()
{
  webpage = HTML_Header();
  webpage += "<div class='notfound'>";
  webpage += "<h1>Sorry</h1>";
  webpage += "<p>Error 404 - Page Not Found</p>";
  webpage += "</div><div class='left'>";
  webpage += "<p>The page you were looking for was not found, it may have been moved or is currently unavailable.</p>";
  webpage += "<p>Please check the address is spelt correctly and try again.</p>";
  webpage += "<p>Or click <b><a href='/'>[Here]</a></b> for the home page.</p></div>";
  webpage += HTML_Footer();
}

void Home()
{
  webpage = HTML_Header();
  webpage += "<br>";

  if (SD_ENABLE && SD.exists(HOME_IMG))
  {
    webpage += "<img src = 'SD_homeImg' alt='homeImg'>";
  }
  else if (SPIFFS_ENABLE && SPIFFS.exists(HOME_IMG))
  {
    webpage += "<img src = 'SPIFFS_homeImg' alt='homeImg'>";
  }

  webpage += "<h3>[&nbsp;Home&nbsp;]　" + SERVER_NAME + "　IP=" + IP_ADDR + "</h3>";
  webpage += HTML_Footer();
}

String HTML_Footer()
{
  String page;
  page += "<br><br>";
  page += "<footer>";
  page += "<p class='ps'><i>" + getTmNTP() + "　　" + PROG_NAME + "　" + VERSION + "</i></p>";
  page += "</footer>";
  page += "<br>";
  page += "</body>";
  page += "</html>";
  return page;
}

String getContentType(String filenametype)
{
  if (filenametype == "download")
  {
    return "application/octet-stream";
  }
  else if (filenametype.endsWith(".txt"))
  {
    return "text/plain;charset=UTF-8";
  }
  else if (filenametype.endsWith(".htm"))
  {
    return "text/html;charset=UTF-8";
  }
  else if (filenametype.endsWith(".html"))
  {
    return "text/html;charset=UTF-8";
  }
  else if (filenametype.endsWith(".css"))
  {
    return "text/css;charset=UTF-8";
  }
  else if (filenametype.endsWith(".js"))
  {
    return "application/javascript";
  }
  else if (filenametype.endsWith(".png"))
  {
    return "image/png";
  }
  else if (filenametype.endsWith(".gif"))
  {
    return "image/gif";
  }
  else if (filenametype.endsWith(".jpg"))
  {
    return "image/jpeg";
  }
  else if (filenametype.endsWith(".ico"))
  {
    return "image/x-icon";
  }
  else if (filenametype.endsWith(".xml"))
  {
    return "text/xml;charset=UTF-8";
  }
  else if (filenametype.endsWith(".pdf"))
  {
    return "application/x-pdf";
  }
  else if (filenametype.endsWith(".zip"))
  {
    return "application/x-zip";
  }
  else if (filenametype.endsWith(".gz"))
  {
    return "application/x-gzip";
  }
  else if (filenametype.endsWith(".csv"))
  {
    return "text/csv;charset=UTF-8";
  }
  else if (filenametype.endsWith(".json"))
  {
    return "application/json;charset=UTF-8";
  }
  else if (filenametype.endsWith(".bmp"))
  {
    return "image/bmp";
  }
  else if (filenametype.endsWith(".wav"))
  {
    return "audio/wav";
  }
  else if (filenametype.endsWith(".mp3"))
  {
    return "audio/mp3";
  }
  else if (filenametype.endsWith(".mp4"))
  {
    return "video/mp4";
  }

  return "text/plain;charset=UTF-8";
}

String EncryptionType(wifi_auth_mode_t encryptionType)
{
  switch (encryptionType)
  {
  case (WIFI_AUTH_OPEN):
    return "OPEN";
  case (WIFI_AUTH_WEP):
    return "WEP";
  case (WIFI_AUTH_WPA_PSK):
    return "WPA PSK";
  case (WIFI_AUTH_WPA2_PSK):
    return "WPA2 PSK";
  case (WIFI_AUTH_WPA_WPA2_PSK):
    return "WPA WPA2 PSK";
  case (WIFI_AUTH_WPA2_ENTERPRISE):
    return "WPA2 ENTERPRISE";
  case (WIFI_AUTH_MAX):
    return "WPA2 MAX";
  default:
    return "";
  }
}

bool compareFileinfo(const fileinfo &a, const fileinfo &b)
{ // ファイル情報を比較するための関数
  // ディレクトリをファイルより前に配置
  if (a.ftype == "Dir" && b.ftype != "Dir")
  {
    return true;
  }
  if (a.ftype != "Dir" && b.ftype == "Dir")
  {
    return false;
  }
  // 同じタイプの場合はファイル名でソート
  return a.filename < b.filename;
}

uint64_t getFileSize(int flType, String filename)
{
  uint64_t filesize;
  File CheckFile;

  if (flType == FS_SPIFFS)
  {
    if (!SPIFFS.exists(filename))
    {
      Serial.println("getFileSize: SPIFFS file not exists");
      return 0;
    }

    CheckFile = SPIFFS.open(filename, "r");
    filesize = (uint64_t)CheckFile.size();
    CheckFile.close();
    return filesize;
  }
  else if (flType == FS_SD)
  {
    String filename_tmp;
    if (SdPath != "/")
      filename_tmp = SdPath + filename;
    else
      filename_tmp = filename;

    if (!SD.exists(filename_tmp))
    {
      Serial.println("getFileSize: SD file not exists");
      return 0;
    }

    CheckFile = SD.open(filename_tmp, "r");
    filesize = (uint64_t)CheckFile.size();
    CheckFile.close();
    return filesize;
  }
  else
  {
    Serial.println("getFileSize Err: invalid flType");
    return 0;
  }
}

bool FS_start(int flType)
{
  if (flType == FS_SPIFFS)
  {
    if (!SPIFFS.begin(true))
    {
      Serial.println("ERR: SPIFFS begin erro...");
      return false;
    }
    return true;
  }
  else if (flType == FS_SD)
  {
    // if (!SD.begin())
    if (!SD.begin(GPIO_NUM_4, SPI, 25000000))
    {
      Serial.println("ERR: SD begin erro...");
      return false;
    }

    if (!SD_cardInfo())
      return false;

    return true;
  }
  else
  {
    Serial.println("FS_start Err: invalid flType");
    return false;
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
    return false;
  case CARD_UNKNOWN:
    Serial.println("ERR: SD card unknown Type");
    return false;
  default:
    Serial.println("ERR: SD cardType is default Type");
    return false;
  }

  return true;
}
