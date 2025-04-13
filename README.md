# m5stack-file-server  
**[`　English　`](README_en.md)**


# プロジェクト解説: m5stack-file-server

## 1. 概要

`m5stack-file-server` は、M5Stackデバイス (ESP32ベース) をWiFiネットワーク上で動作する**Webファイルサーバー**に変えるプロジェクトです。Webブラウザを通じて、SDカードや内蔵SPIFFS上のファイルを簡単に管理できます。

**主な目的:**

*   PCやスマートフォンからM5Stack上のファイルへアクセス・管理する手段を提供する。
*   プログラムの更新や設定ファイルの編集、ログファイルの確認などを容易にする。

**対応デバイス:**

*   M5Stack Core, Core2, CoreS3 など、SDカードスロットを持つESP32ベースのM5Stackデバイス。
    *   (SPIFFSのみを使用する場合はSDカードスロットは不要)

## 2. 主要機能

*   **ファイルシステムアクセス:**
    *   SDカード (`SD_USE = true` の場合)
    *   SPIFFS (内蔵フラッシュメモリ上のファイルシステム, `SPIFFS_USE = true` の場合)
*   **Webインターフェース (レスポンシブデザイン):**
    *   **ファイル/ディレクトリ一覧表示:**
        *   SDカード: カレントディレクトリの内容 (ファイルとサブディレクトリ) を表示。
        *   SPIFFS: ルートにあるファイル一覧を表示。
        *   ファイル名、サイズ (ディレクトリの場合は空欄) を表示。
        *   PC/スマホ表示に対応したレイアウト切り替え。
    *   **ファイル操作:**
        *   アップロード: PC/スマホからファイルを選択してM5Stackへ転送。
        *   ダウンロード: M5Stack上のファイルをPC/スマホへ保存。
        *   ストリーミング: ブラウザで直接表示可能なファイル (画像、音声、テキストなど) を表示。
        *   削除: ファイルを選択して削除 (確認画面あり)。
        *   リネーム: ファイル/ディレクトリ名を選択して変更。
        *   テキスト表示: `.txt`, `.log`, `.html`, `.js`, `.css`, `.json`, `.c`, `.h`, `.cpp` などのテキストベースのファイルをブラウザ上で表示。
    *   **SDカード ディレクトリ操作:**
        *   ディレクトリ変更 (Chdir): サブディレクトリへ移動。
        *   ディレクトリ作成 (Mkdir): 新しいディレクトリを作成。
        *   ディレクトリ削除 (Rmdir): 空のディレクトリを削除 (確認画面あり)。
        *   親ディレクトリへ移動 (Up)。
        *   ルートディレクトリへ移動 (Top)。
*   **ネットワーク機能:**
    *   **WiFi接続:** 指定されたSSIDとパスワードでWiFiアクセスポイントに接続 (STAモード)。
    *   **mDNS:** IPアドレスの代わりに `http://(設定したサーバー名).local/` (例: `http://stackchan.local/`) でアクセス可能。
    *   **設定ファイル:** SDカードまたはSPIFFSのルートにある `wifi.txt` ファイルにSSID、パスワード、サーバー名を記述することで、ソースコードのデフォルト設定を上書き可能。
*   **システム機能:**
    *   **ステータスページ (`/system`):**
        *   ファイルシステム情報 (SD/SPIFFSの合計/使用/空き容量)。
        *   メモリ情報 (SRAM/PSRAMの合計/空き/最小空き/最大確保可能ブロック)。
        *   NVS (不揮発性ストレージ) 情報。
        *   CPU情報 (モデル、リビジョン、コア数、周波数など)。
        *   ネットワーク情報 (IPアドレス、サーバー名、SSID、MACアドレスなど)。
        *   時刻情報 (RTC時刻、NTP同期時刻)。
        *   ファイル転送統計 (最後のアップロード/ダウンロードのサイズと速度)。
    *   **NTP時刻同期:** 起動時にNTPサーバーから時刻を取得し、M5StackのRTC (リアルタイムクロック) を調整 (`RTC_ADJUST_REQ = true` の場合)。
    *   **Web API:**
        *   `/shutdown?reboot=on`: デバイスを再起動 (確認ダイアログあり)。
        *   `/shutdown`: デバイスをシャットダウン (電源オフ、確認ダイアログあり)。
        *   `/test?ok=true`: APIの動作テスト用。
*   **その他:**
    *   **SDカードアップデーター:** (オプション `ENABLE_SD_UPDATER` 定義時) SDカードからのファームウェア更新機能。
    *   **ディスプレイ表示:** (オプション `DISP_ON = true` の場合) M5Stackの画面にIPアドレスや簡単なログメッセージを表示。
    *   **Favicon:** SDカードまたはSPIFFSのルートに `favicon.ico` があれば表示。
    *   **ホーム画像:** SDカードまたはSPIFFSのルートに `homeImg.gif` があればホームページに表示。

## 3. ファイル構成と役割

プロジェクトは以下のファイルで構成されています。

*   **`main.cpp`**:
    *   **役割:** アプリケーションの**エントリーポイント** (`setup()`, `loop()`)。
    *   **詳細:**
        *   全体的な設定定数 (SD/SPIFFS使用有無、ディスプレイ表示、RTC調整要否など) を定義。
        *   `setupServer()` を呼び出し、ファイルシステム、ネットワーク、Webサーバーの初期化を統括。
        *   メインループ (`loop()`) でWeb APIからのリクエスト処理 (`requestManage()`) とRTC時刻調整 (`adjustRTC()`) を実行。
*   **`fileServer.cpp`**:
    *   **役割:** Webサーバーの基本設定と共通機能。
    *   **詳細:**
        *   `AsyncWebServer` オブジェクト (`server`) の初期設定と起動 (`fileServerStart()`)。
        *   共通のHTMLヘッダー (`HTML_Header()`)、フッター (`HTML_Footer()`)、CSSスタイル (`HTML_Style()`) を生成。レスポンシブデザイン対応。
        *   ホームページ (`/`) とシステム情報ページ (`/system`) のハンドラを定義。
        *   ファイルシステム (SD/SPIFFS) の初期化関数 (`FS_start()`)。
        *   ファイル拡張子に応じたContent-Typeを返す (`getContentType()`)。
        *   未定義URLへのアクセスを処理する `notFound()` ハンドラ (各ファイルシステムハンドラへ処理を委譲)。
*   **`util.cpp`**:
    *   **役割:** 各種**補助関数**を提供。
    *   **詳細:**
        *   ネットワーク関連: WiFi接続 (`wifiStart()`)、mDNS開始 (`mdnsStart()`)。
        *   時刻関連: NTPサーバーからの時刻取得とRTC設定 (`adjustRTC()`)、時刻文字列フォーマット (`getTmRTC()`, `getTmNTP()`, `strTmInfo`)。
        *   設定読み込み: `wifi.txt` からネットワーク設定を取得 (`getSetting()`)。
        *   データ変換: ファイルサイズをKB/MB/GB等に変換 (`ConvBytesUnits()`)、URLエンコード/デコード (`urlEncode()`, `urlDecode()`)。
        *   ログ出力: シリアルとM5Stackディスプレイへの共通出力 (`prt()`)。
        *   デバッグ: ヒープメモリ情報取得/表示 (`getHeapInf()`, `prtHeapInf`)。
*   **`SD_handler.cpp`**:
    *   **役割:** **SDカード**関連のWebインターフェースとファイル/ディレクトリ操作処理。
    *   **詳細:**
        *   ファイル/ディレクトリ一覧表示 (`SD_Dir`, `SD_Directory`)。
        *   ファイル操作 (アップロード、ダウンロード、ストリーミング、削除、リネーム、テキスト表示) のWeb UI生成と処理ハンドラ (`SD_handleFileUpload`, `SD_Handle_File_Delete`, `SD_Handle_File_Rename`, `SD_View_Text` など)。
        *   ディレクトリ操作 (移動、作成、削除) のWeb UI生成と処理ハンドラ (`SDdir_*` 関数群: `SDdir_Handle_chdir`, `SDdir_Handle_mkdir`, `SDdir_Handle_rmdir` など)。
        *   削除/ディレクトリ削除時の確認ページ生成 (`SD_Generate_Confirm_Page`, `SDdir_Generate_Confirm_Page`)。
        *   SDカード関連の `notFound` 処理 (`SD_notFound`, `SDdir_notFound`)。
*   **`SPIFFS_handler.cpp`**:
    *   **役割:** **SPIFFS**関連のWebインターフェースとファイル操作処理。
    *   **詳細:**
        *   ファイル一覧表示 (`SPIFFS_Dir`, `SPIFFS_Directory`)。
        *   ファイル操作 (アップロード、ダウンロード、ストリーミング、削除、リネーム、テキスト表示) のWeb UI生成と処理ハンドラ (`SPIFFS_handleFileUpload`, `SPIFFS_Handle_File_Delete`, `SPIFFS_Handle_File_Rename`, `SPIFFS_View_Text` など)。
        *   削除時の確認ページ生成 (`SPIFFS_Generate_Confirm_Page`)。
        *   SPIFFS関連の `notFound` 処理 (`SPIFFS_notFound`)。
        *   (SPIFFSはディレクトリ構造をネイティブサポートしないため、ディレクトリ操作機能はなし)。
*   **`webApi.cpp`**:
    *   **役割:** デバイス制御用の**Web API**エンドポイントを提供。
    *   **詳細:**
        *   `/shutdown`, `/test` などのAPIルート設定 (`webApiSetup()`)。
        *   APIリクエストの処理 (`handle_shutdown`, `handle_test`)。
        *   `loop()` 関数で実行される実際のデバイス制御 (再起動 `REBOOT()`, 電源オフ `POWER_OFF()`, 停止 `STOP()`) のためのフラグ管理 (`requestManage()`, `sendReq()`)。
        *   API応答用の簡単なHTMLページ生成。
*   **`fileServer.h`** (ヘッダーファイル):
    *   **役割:** プロジェクト全体で共有される定義や宣言。
    *   **詳細:**
        *   必要なライブラリ (`M5Unified.h`, `WiFi.h`, `ESPAsyncWebServer.h`, `SPIFFS.h`, `SD.h` など) のインクルード。
        *   定数定義 (`FS_SD`, `FS_SPIFFS`, `UNIT_AUTO` など)。
        *   グローバル変数 (`server`, `webpage`, `SSID`, `IP_ADDR`, `SD_ENABLE`, `SdPath` など) の `extern` 宣言。
        *   `fileinfo` 構造体の定義。
        *   各 `.cpp` ファイルで定義されている関数のプロトタイプ宣言。

## 4. セットアップと使用方法

1.  **ビルドと書き込み:**
    *   PlatformIOなどの開発環境でプロジェクトを開き、ターゲットデバイスを設定してビルドし、M5Stackに書き込みます。
2.  **ネットワーク設定:**
    *   **方法1 (推奨):** SDカードまたはSPIFFSのルートディレクトリに `wifi.txt` という名前のファイルを作成し、以下の形式で記述します。
        ```
        YOUR_WIFI_SSID
        YOUR_WIFI_SSID_PASSWORD
        your_server_name
        ```
        (各行の末尾に改行を入れてください。`your_server_name` はmDNSで使用する名前です。)
    *   **方法2:** `main.cpp` 内の `YOUR_SSID`, `YOUR_SSID_PASS`, `YOUR_SERVER_NAME` の値を直接編集します。
3.  **アクセス:**
    *   M5Stackを起動すると、WiFiに接続し、IPアドレスとサーバー名がシリアルモニター (およびディスプレイが有効なら画面) に表示されます。
    *   同じネットワーク内のPCやスマートフォンのWebブラウザで、表示されたIPアドレス (`http://<IPアドレス>/`) またはmDNS名 (`http://<サーバー名>.local/`) にアクセスします。
4.  **操作:**
    *   表示されたWebインターフェース上のメニューやボタンをクリックして、ファイル操作やシステム情報の確認を行います。

## 5. カスタマイズ

*   **機能の有効/無効化:** `main.cpp` の `SD_USE`, `SPIFFS_USE`, `DISP_ON`, `RTC_ADJUST_REQ` などの定数を変更します。
*   **ネットワーク設定:** 上記の `wifi.txt` または `main.cpp` 内のデフォルト値を変更します。
*   **外観:**
    *   SDカードまたはSPIFFSのルートに `favicon.ico` を置くと、ブラウザタブのアイコンが変わります。
    *   SDカードまたはSPIFFSのルートに `homeImg.gif` を置くと、ホームページに画像が表示されます。
    *   `fileServer.cpp` の `HTML_Style()` 関数内のCSSを編集することで、Webインターフェースのデザインを変更できます。

---

