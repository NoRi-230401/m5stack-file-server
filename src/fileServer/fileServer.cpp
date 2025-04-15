// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-15
// -------------------------------------------------------
// fileServer.cpp
// *******************************************************
#include "fileServer.h"

bool setupServer();
String HTML_Header();
String HTML_Style();
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
// const String githubPage = "https://github.com/NoRi-230401/m5stack-file-server";

// NTP connection information.
#define NTP_SVR1 "ntp.nict.jp"         // NTP server1
#define NTP_SVR2 "ntp.jst.mfeed.ad.jp" // NTP server2
#define NTP_GMT_OFFSET 9 * 3600L       // Sec  : GMT offset
#define NTP_DAYLIGHT_OFFSET 0          // Sec  : daylight offset

// RTC adjust
uint32_t TM_RTC_ADJUST = 10 * 1000L; // mSec : adjust after setup()
uint32_t TM_SETUP_DONE = 0;
bool RTC_ENABLE = false;

AsyncWebServer server(80);
String webpage;

bool setupServer()
{
   prt("-   " + PROG_NAME + "   -\n");

   // --- SD and SPIFFS start ---
   SD_ENABLE = false;
   if (SD_USE)
   {
      SD_ENABLE = FS_start(FS_SD);
      if (SD_ENABLE)
         prt("SD      .....  OK");
      else
         prt("SD      .....  NG");
   }

   SPIFFS_ENABLE = false;
   if (SPIFFS_USE)
   {
      SPIFFS_ENABLE = FS_start(FS_SPIFFS);
      if (SPIFFS_ENABLE)
         prt("SPIFFS  .....  OK");
      else
         prt("SPIFFS  .....  NG");
   }

   if (!SPIFFS_ENABLE && !SD_ENABLE)
   {
      prt("Both SD and SPIFFS are not available");
      return false;
   }

   // ------- Network Settings Read ---------
   SSID = "";
   SSID_PASS = "";
   SERVER_NAME = "";

   if (SD_ENABLE && getWiFiSettings(FS_SD, WIFI_TXT))
      prt(" Settings read from SD");
   else if (SPIFFS_ENABLE && getWiFiSettings(FS_SPIFFS, WIFI_TXT))
      prt(" Settings read from SPIFFS");

   if (SSID == "")
      SSID = YOUR_SSID;
   prt(" SSID: " + SSID);

   if (SSID_PASS == "")
      SSID_PASS = YOUR_SSID_PASS;

   if (SERVER_NAME == "")
      SERVER_NAME = YOUR_SERVER_NAME;

   if (SSID == "" || SSID_PASS == "" || SERVER_NAME == "")
   {
      prt("SETTINGS.....  NG");
      return false;
   }

   // --- wifi and Server Start -------
   if (!wifiStart())
   {
      prt("WiFi    .....  NG");
      return false;
   }
   prt("WiFi    .....  OK");

   if (!mdnsStart())
   {
      prt("mDNS    .....  NG");
      return false;
   }
   prt("mDNS    .....  OK");

   // NTP Server config
   configTime(NTP_GMT_OFFSET, NTP_DAYLIGHT_OFFSET, NTP_SVR1, NTP_SVR2);

   // check RTC enable
   if (RTC_ENABLE = M5.Rtc.isEnabled())
   {
      Serial.println("RTC is enable");
   }
   else
   {
      Serial.println("RTC is disable");
      RTC_ADJUST_ON = false;
   }

   if (!fileServerStart())
   {
      prt("fileServer ..  NG");
      return false;
   }
   prt("fileServer ..  OK");
   prt("\nIP Addr: " + IP_ADDR);
   prt("\nServerName: " + SERVER_NAME);

   TM_SETUP_DONE = millis();
   return true;
}

String HTML_Header()
{
   String page;
   page = "<!DOCTYPE html>";
   page += "<html lang = 'ja'>";
   page += "<head>";
   page += "<title>" + SERVER_NAME + "</title>";
   page += "<base target='_self'>";
   page += "<meta charset='UTF-8'>";
   page += "<link rel='icon' href='/favicon.ico'>";
   page += "<meta name='viewport' content='width=device-width,initial-scale=1.0'>";
   // ---javaScript ----
   page += "<script>";
   page += "function confirmP() {if (confirm('Can I turn off?')){window.open('/shutdown?time=5', '_blank');} else {alert('stopped');}}";
   page += "function confirmR() {if (confirm('Can I reboot?')){window.open('/shutdown?reboot=on&time=5', '_blank');} else {alert('stopped');}}";
   page += "</script>";
   // ------------------------------------------
   page += HTML_Style();
   // ------------------------------------------
   page += "</head>";
   page += "<body>";

   // -- 1 -- Top Navigation (Home, Status, Reboot, PowerOff) --
   page += "<div class = 'topnav'>";
   page += "<a href='/'>Home</a>";
   page += "<a href='/system'>Status</a>";
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
      page += "<a href='/SPIFFS_vTxt'>ViewTxt</a>";
      page += "<a href='/SPIFFS_stream'>Stream</a>";
      page += "<a href='/SPIFFS_delete'>Delete</a>";
      page += "<a href='/SPIFFS_rename'>Rename</a>";
      page += "</div>";
   }
   page += "<br>";

   // ------------------ SD Menu -------------------------
   if (SD_ENABLE)
   {
      // -- SD File Operations --
      page += "<div class = 'topnav2'>";
      page += "<span>SD:</span>"; // ラベル
      page += "<a href='/SD_dir'>Dir</a>";
      page += "<a href='/SD_upload'>Upload</a> ";
      page += "<a href='/SD_download'>Download</a>";
      page += "<a href='/SD_vTxt'>ViewTxt</a>";
      page += "<a href='/SD_stream'>Stream</a>";
      page += "<a href='/SD_delete'>Delete</a>";
      page += "<a href='/SD_rename'>Rename</a>";
      page += "</div>";

      // ------- SD Path and Directory Operations ----------
      page += "<div class = 'topnav2'>";
      page += "<span>Path:&nbsp;" + SdPath + "</span>";
      page += "<a href='/SDdir_chTop'>Top</a>";
      page += "<a href='/SDdir_chUp'>Up</a>";
      page += "<a href='/SDdir_chdir'>Chdir</a>";
      page += "<a href='/SDdir_mkdir'>Mkdir</a>";
      page += "<a href='/SDdir_rmdir'>Rmdir</a>";
      page += "</div>";
   }
   return page;
}

String HTML_Style()
{
   String page;
   page += "<style>";

   // ------------------------------------------------
   // - ファイル一覧テーブル用の基本スタイル
   // ------------------------------------------------
   page += ".file-list-table { width: 95%; border-collapse: collapse; margin-left:auto; margin-right:auto; border: 1px solid #ccc; }";
   page += ".file-list-table thead th { border: 1px solid #ddd; text-align: left; padding: 8px; box-sizing: border-box; background-color: #e9e9e9; font-weight: bold; }";
   page += ".file-list-table tbody td { border: 1px solid #ddd; text-align: left; padding: 8px; box-sizing: border-box; vertical-align: top; }";
   // 列幅指定 (クラスセレクタ使用)
   page += ".file-list-table td.file-name { width: 75%; word-break: break-all; }";
   page += ".file-list-table td.file-size { width: 25%; text-align: right; }";
   // 交互背景色
   page += ".file-list-table tbody tr:nth-child(even) { background-color: #f8f8f8; }";
   page += ".file-list-table tbody tr:nth-child(odd) { background-color: #ffffff; }";

   // --- <pre> タグ用のスタイルを追加 ---
   page += "pre {";
   page += "  white-space: pre-wrap;";     /* CSS3: 自動折り返し */
   page += "  word-wrap: break-word;";     /* IEなど: 単語の途中でも折り返し */
   page += "  font-family: monospace;";    /* 等幅フォント指定 */
   page += "  font-size: 1.4rem;";         /* 基本のフォントサイズ (PC向け) */
   page += "  text-align: left;";          /* 左寄せ */
   page += "  padding: 10px;";             /* 内側余白 */
   page += "  margin: 1em;";               /* 外側余白 */
   page += "  background-color: #f8f8f8;"; /* 背景色 (任意) */
   page += "  border: 1px solid #ddd;";    /* 枠線 (任意) */
   page += "  overflow-x: auto;";          /* 必要なら横スクロール */
   page += "}";

   // ------------------------------------------------
   // --- PC向けスタイル (画面幅が 769px 以上の場合) ---
   // ------------------------------------------------
   page += "@media screen and (min-width: 769px) {";
   page += "  .file-list-table { width: 80%; border: none; }";
   page += "  .file-list-table:not(.rename-table) thead { display: none; }";
   page += "  .file-list-table > tbody { ";
   page += "    display: flex; ";
   page += "    flex-wrap: wrap; ";
   page += "    border: none; ";
   page += "  }";

   page += "  .file-list-table > tbody > tr.file-entry { ";
   page += "    display: flex; ";
   page += "    width: 50%; "; // 2列表示
   page += "    box-sizing: border-box; ";
   page += "    border: none; ";
   page += "    background-color: transparent !important; ";
   page += "  }";
   page += "  .file-list-table > tbody > tr.file-entry > td { ";
   page += "    border: none; ";
   page += "    border-bottom: 1px solid #eee; ";
   page += "    padding: 8px; ";
   page += "    box-sizing: border-box; ";
   page += "    background-color: transparent !important; "; // tdの背景色はtrの交互色で制御
   page += "    text-align: left; ";                         // 基本左寄せ
   page += "    vertical-align: top; ";
   page += "    flex-grow: 0; ";
   page += "    flex-shrink: 0; ";
   page += "  }";

   // 各ファイル情報内の 'Name' の右側に薄い区切り線を追加 (左右両方に適用)
   page += "  .file-list-table > tbody > tr.file-entry > td.file-name { ";
   page += "    border-right: 1px solid #eee; "; // 薄い右境界線 (ファイル名とサイズの区切り)
   page += "  }";

   // 左側ファイル情報の 'Size' の右側にも区切り線を追加 (左右のファイル間の区切り)
   page += "  .file-list-table > tbody > tr.file-entry:nth-child(odd) > td.file-size { ";
   page += "    border-right: 2px solid #ccc; "; // 少し濃い右境界線
   page += "  }";

   // 右側ファイル情報の 'Size' の右側には線は不要 (テーブル右端のため)
   page += "  .file-list-table > tbody > tr.file-entry:nth-child(even) > td.file-size { ";
   page += "    border-right: none; ";
   page += "  }";

   // データセルの幅指定 (flex-basis) - Type列削除に伴い調整
   page += "  .file-list-table > tbody > tr.file-entry > td.file-name { flex-basis: 75%; flex-grow: 1; word-break: break-all; }"; // ファイル名は伸縮可能に
   page += "  .file-list-table > tbody > tr.file-entry > td.file-size { flex-basis: 25%; text-align: right; }";                   // サイズは右寄せ

   // PC表示用の交互背景色 (2行ごと = 左右ペアごと)
   page += "  .file-list-table > tbody > tr.file-entry:nth-child(4n-1), "; // 左列の奇数行ペア
   page += "  .file-list-table > tbody > tr.file-entry:nth-child(4n) { ";  // 右列の奇数行ペア
   page += "    background-color: #f8f8f8 !important; ";                   // 薄いグレー
   page += "  }";
   page += "  .file-list-table > tbody > tr.file-entry:nth-child(4n-3), ";  // 左列の偶数行ペア
   page += "  .file-list-table > tbody > tr.file-entry:nth-child(4n-2) { "; // 右列の偶数行ペア
   page += "    background-color: #ffffff !important; ";                    // 白
   page += "  }";

   // --- <rename-table専用> ---
   page += "  .rename-table thead {";
   page += "      display: table-header-group;";
   page += "  }";
   page += "  .rename-table thead th {";
   page += "      /* 継承されるスタイル */";
   page += "  }";
   // ヘッダー列幅を調整 (例: 60% と 40%)
   page += "  .rename-table thead th.rename-select-header { width: 60%; }"; // 1列目
   page += "  .rename-table thead th.rename-new-header { width: 40%; }";    // 2列目

   /* --- <rename画面>専用スタイル (tbody) --- */
   page += "  .rename-table > tbody {";
   page += "    display: table-row-group;";
   page += "    border: none;";
   page += "  }";
   page += "  .rename-table > tbody > tr.file-entry {";
   page += "    display: table-row;";
   page += "    width: 100%;";
   page += "    border: none;";
   page += "    background-color: transparent !important;";
   page += "  }";
   page += "  .rename-table > tbody > tr.file-entry > td {";
   page += "    display: table-cell;";
   page += "    border: none;";
   page += "    border-bottom: 1px solid #ddd;";
   page += "    border-right: 1px solid #ddd;"; // 右境界線を追加
   page += "    padding: 0; /* ラベル/入力にpaddingを持たせるためtdは0に */";
   page += "    box-sizing: border-box;";
   page += "    background-color: transparent !important;";
   page += "    text-align: left;";
   page += "    vertical-align: middle; /* 垂直方向中央揃え */";
   page += "    /* flex関連リセット */";
   page += "    flex-basis: auto;";
   page += "    flex-grow: 0;";
   page += "    flex-shrink: 1;";
   page += "  }";
   // 最後のセルの右境界線を削除
   page += "  .rename-table > tbody > tr.file-entry > td:last-child {";
   page += "    border-right: none;";
   page += "  }";

   // 各セルの幅指定 (theadに合わせて)
   page += "  .rename-table td.rename-select-cell { width: 60%; }";            // 1列目セル
   page += "  .rename-table td.rename-new-name { width: 40%; padding: 8px; }"; // 2列目セル (入力欄にpadding)

   // ラベル (ボタン風) のスタイル
   page += "  .rename-table label.rename-select-label {";
   page += "      display: block;"; // セル全体に広がる
   page += "      width: 100%;";
   page += "      padding: 8px;"; // 内側の余白
   page += "      margin: 0;";
   page += "      box-sizing: border-box;";
   page += "      cursor: pointer;";
   page += "      background-color: transparent;"; // デフォルト背景なし
   page += "      border: none;";
   page += "      text-align: left;";
   page += "      white-space: normal;";                    // 折り返し許可
   page += "      word-break: break-all;";                  // 強制改行
   page += "      transition: background-color 0.2s ease;"; // ホバー/選択効果用
   page += "  }";
   // ラベルホバー時のスタイル
   page += "  .rename-table label.rename-select-label:hover {";
   page += "      background-color: #f0f0f0;"; // 薄いグレー
   page += "  }";
   // 選択されたラジオボタンに対応するラベルのスタイル
   // input[type=radio]:checked + input[type=hidden] + label の順序に合わせる
   page += "  .rename-table input[type='radio']:checked + input[type='hidden'] + label.rename-select-label {";
   page += "      background-color: lightblue;"; // 選択時の背景色
   page += "      font-weight: bold;";
   page += "  }";

   // 交互背景色はそのまま
   page += "  .rename-table > tbody > tr.file-entry:nth-child(even) { background-color: #f8f8f8 !important; }";
   page += "  .rename-table > tbody > tr.file-entry:nth-child(odd) { background-color: #ffffff !important; }";
   page += "}"; // PC向け @media 終了 ************************************

   // ------------------------------------------------
   // - スマホ向けスタイル (画面幅が 768px 以下の場合) -
   // ------------------------------------------------
   page += "@media screen and (max-width: 768px) {";
   page += "  body {font-size: 1.4rem;} div {font-size: 1.4rem;}";
   page += "  p {font-size: 1.4rem;} h5 {font-size: 1.4rem;}";
   page += "  img {max-width:100%;height:auto;}";
   page += "  .file-list-table { width: 95%; font-size: 1.2rem; border: 1px solid #ccc; }"; // 基本は変更なし
   page += "  .file-list-table thead { display: none; }";
   page += "  .file-list-table > tbody { display: table-row-group; border: none; }";
   page += "  .file-list-table > tbody > tr.file-entry { display: table-row; width: 100%; border: none; }";
   page += "  .file-list-table > tbody > tr.file-entry > td { ";
   page += "    display: table-cell; ";                      // 通常のセル表示
                                                             // page += "    width: auto; ";
   page += "    border: none; ";                             // PCで消したボーダーは基本なし
   page += "    border-bottom: 1px solid #ddd; ";            // 下線のみ
   page += "    border-right: 1px solid #ddd; ";             // 右にも線を引く (最後のセル以外)
   page += "    flex-basis: auto; ";                         // PCのflex指定をリセット
   page += "    flex-grow: 0; ";                             // PCのflex指定をリセット
   page += "    flex-shrink: 1; ";                           // PCのflex指定をリセット
   page += "    background-color: transparent !important; "; // 背景色はtrの交互色で制御
   page += "    text-align: left; ";                         // 基本左寄せ
   page += "    vertical-align: top; ";
   page += "  }";

   // スマホ表示時の列幅指定
   page += "  .file-list-table > tbody > tr.file-entry > td.file-name { width: 75%; }";
   page += "  .file-list-table > tbody > tr.file-entry > td.file-size { width: 25%; border-right: none; text-align: right; }";
   page += "  .file-list-table > tbody > tr.file-entry:nth-child(even) { background-color: #f8f8f8 !important; }";
   page += "  .file-list-table > tbody > tr.file-entry:nth-child(odd) { background-color: #ffffff !important; }";

   // rename-table のスマホ向けスタイル調整 (PC向けと同様の構造を適用)
   // 基本的にPC向けスタイルが適用されるはずだが、必要に応じて上書き
   page += "  .rename-table { /* スマホでのテーブル全体の調整が必要なら記述 */ }";
   page += "  .rename-table thead { /* スマホでヘッダー表示が必要なら display: table-header-group; */ }";
   page += "  .rename-table thead th.rename-select-header { width: 60%; }";
   page += "  .rename-table thead th.rename-new-header { width: 40%; }";
   page += "  .rename-table > tbody > tr.file-entry > td { /* スマホでのセル共通スタイルの調整 */ }";
   page += "  .rename-table td.rename-select-cell { width: 60%; }";
   page += "  .rename-table td.rename-new-name { width: 40%; padding: 8px; }";
   page += "  .rename-table label.rename-select-label { /* スマホでのラベルスタイルの調整 */ }";

   page += "  pre {";
   page += "    font-size: 1.4rem;";
   page += "    /* 必要であれば他のスタイルも上書き */";
   page += "  }";

   page += "}"; // スマホ向け @media 終了 ******************************

   // ------------------------------------------------
   // -----　　 その他の共通スタイル
   // ------------------------------------------------
   page += "html {font-size: 62.5%;}"; // remの基準
   page += "body {width:100%;margin: 0; padding: 0; font-family:Arial,Helvetica,sans-serif;font-size:1.6rem;color:#2f4f4f;background-color:#fffacd;text-align:center;}";
   // bodyのデフォルトサイズと余白調整
   page += "footer {padding:1.0rem;background-color:cyan;font-size:1.4rem;}";

   // --- 一般的なテーブルスタイル (ファイル一覧以外で使用する場合) ---
   page += "table:not(.file-list-table) {font-family:arial,sans-serif;border-collapse:collapse;width:90%; margin: 1em auto;}";

   // ファイル一覧以外に適用、幅とマージン調整
   page += "table:not(.file-list-table) td, table:not(.file-list-table) th {border:1px solid #dddddd;text-align:left;padding:0.8rem;}";
   page += "table:not(.file-list-table) tr:nth-child(even) {background-color:#dddddd;}";
   page += "table.center {margin-left:auto;margin-right:auto;}"; // 中央寄せクラス

   // --- 見出し等のスタイル ---
   page += "h3 {color:#6ecf12;font-size:1.9rem;font-style:normal;text-align:center; margin: 1em 0;}";   // サイズとマージン調整
   page += "h4 {color:slateblue;font-size:1.7rem;text-align:center;font-style:normal; margin: 1em 0;}"; // サイズ、中央寄せ、マージン調整

   // --- 汎用クラス ---
   page += ".center {margin-left:auto;margin-right:auto;}";
   page += ".notfound {padding:1em;text-align:center;font-size:1.6rem;}"; // サイズ調整
   page += ".left {text-align:left;}";
   page += ".medium {font-size:1.9rem;padding:0;margin:0}";
   page += ".ps {font-size:1.4rem;padding:0;margin:0}";
   // page += ".sp {background-color:silver;white-space:nowrap;width:2%;}"; // file-list-tableでは使わない想定

   // --- TOPNAV スタイル ---
   page += ".topnav {overflow: hidden; background-color:lightPink; padding: 2px 0; text-align: center;}";

   // --- Topnav のリンク (a タグ) スタイル ---
   page += ".topnav a {";
   page += "  display: inline-block;";
   page += "  color: blue;";
   page += "  text-align: center;";
   page += "  padding: 6px 10px;";
   page += "  margin: 1px 4px;";
   page += "  text-decoration: none;";
   page += "  font-size: 1.4rem;";
   page += "  border: none;";                  // 枠線なし
   page += "  background-color: transparent;"; // 背景透明
   page += "  cursor: pointer;";
   page += "  vertical-align: middle;";
   page += "}";
   page += ".topnav a:hover { background-color: deepskyblue; color: white; }"; // リンクのホバー効果
   page += ".topnav a.active { background-color: lightblue; color: blue; }";   // アクティブなリンク

   // --- Topnav のボタン (input type='button') スタイル ---
   page += ".topnav input[type='button'] {";
   page += "  display: inline-block;";     // 横並びのため
   page += "  padding: 5px 12px;";         // パディングを少し調整
   page += "  margin: 1px 4px;";           // マージン
   page += "  font-size: 1.4rem;";         // フォントサイズ
   page += "  color: #333;";               // 文字色 (例: 暗いグレー)
   page += "  background-color: #f0f0f0;"; // 背景色 (例: 薄いグレー)
   page += "  border: 1px solid #ccc;";    // 枠線 (例: グレー)
   page += "  border-radius: 4px;";        // 角丸
   page += "  cursor: pointer;";           // カーソル
   page += "  vertical-align: middle;";    // 垂直位置揃え
   page += "  text-decoration: none;";     // 下線なし
   page += "}";

   // ボタンのホバー効果
   page += ".topnav input[type='button']:hover {";
   page += "  background-color: #e0e0e0;"; // ホバー時の背景色 (少し濃いグレー)
   page += "  border-color: #bbb;";        // ホバー時の枠線色
   page += "  color: #000;";               // ホバー時の文字色 (黒)
   page += "}";

   // --- 'PowOff' ボタンを少し目立たせる (オプション) ---
   page += ".topnav input[value='PowOff'] {"; // value属性で選択
   page += "  background-color: #f8d7da;";    // 背景色 (薄い赤)
   page += "  border-color: #f5c6cb;";        // 枠線 (赤系)
   page += "  color: #721c24;";               // 文字色 (濃い赤)
   page += "}";
   page += ".topnav input[value='PowOff']:hover {";
   page += "  background-color: #f5c6cb;"; // ホバー時の背景色
   page += "  border-color: #f1b0b7;";
   page += "  color: #721c24;";
   page += "}";

   // --- TOPNAV2 スタイル ---
   page += ".topnav2 {overflow: hidden; background-color:lightcyan; padding: 3px 0; text-align: center; line-height: 1.3;}";
   page += ".topnav2 a, .topnav2 span {display: inline-block; color:blue; text-align:center; padding: 5px 8px; margin: 1px 3px; text-decoration:none; font-size:1.5rem; vertical-align: middle;}";
   page += ".topnav2 a:hover {background-color:deepskyblue;color:white;}";
   page += ".topnav2 a.active {background-color:lightblue;color:blue;}";
   page += ".topnav2 span { color: #555; }";

   // --- ボタンの基本スタイル ---
   page += "input[type='button'], input[type='submit'] { padding: 8px 15px; font-size: 1.4rem; cursor: pointer; border: 1px solid #ccc; border-radius: 4px; background-color: #f0f0f0; margin: 5px;}";
   page += "input[type='button']:hover, input[type='submit']:hover { background-color: #e0e0e0; }";

   page += "button { padding: 8px 15px; font-size: 1.4rem; cursor: pointer; border: 1px solid #ccc; border-radius: 4px; background-color: #f0f0f0; margin: 5px;}";
   page += "button a { text-decoration: none; color: inherit; }";
   page += "button:hover { background-color: #e0e0e0; }";

   // --- フォーム要素のスタイル ---
   page += "input[type='text'], input[type='file'] { padding: 8px; font-size: 1.4rem; border: 1px solid #ccc; border-radius: 4px; margin: 5px; box-sizing: border-box;}";
   page += "form { margin: 1em 0; }"; // フォームのマージン

   // ------------- end of style -----------------
   page += "</style>";
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

   webApiSetup();

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

   webpage += "<h3>[Home]　" + SERVER_NAME + "　IP=" + IP_ADDR + "</h3>";
   webpage += HTML_Footer();
}

String HTML_Footer()
{
   String page;
   page += "<br>";
   page += "<footer>";
   
   // page += "<p class='ps'><i>" + getTmNTP() + "　" + PROG_NAME + "&nbsp;" + VERSION + "</i></p>";
   page += "<p class='ps'><i>" + getTmNTP() + "　<a href=" + GITHUB_URL + " target='_blank'>" + PROG_NAME +"</a>　" + VERSION + "</i></p>";
   
   page += "</footer>";
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
