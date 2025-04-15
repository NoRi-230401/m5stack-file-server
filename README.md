# m5stack-file-server  
**[`　English　`](README_en.md)**

## 1. 概要

`m5stack-file-server` は、M5Stackデバイス (ESP32ベース) をWiFiネットワーク上で動作する**Webファイルサーバー**にするソフトウエアです。Webブラウザを通じて、SDカードや内蔵SPIFFS上のファイルを簡単に管理できます。

**主な目的:**

*   PCやスマートフォンからM5Stack上のファイルへアクセス・管理する手段を提供する。
*   設定ファイルやログファイル、画像、音声の確認やプログラムの更新などを容易にする。
*   他のソフトウエアに、このソフトの機能を組み込みやすいように提供する。(ご自由にご活用ください。)<br>
    <br>

**対応デバイス:**

*   M5Stack Core, Core2, CoreS3 など、SDカードスロットを持つESP32ベースのM5Stackデバイス。
    *   (SPIFFSのみを使用する場合はSDカードスロットは不要)

* 現在、`Core2 for AWS` および `CoreS3 SE`で動作確認しています。

    不具合等があればご連絡ください。    


---


**PCおよびスマホの画面表示:**

![PC画面](images/s-gazo01.png) [PC画面](images/gazo01.png)

![スマホ画面](images/s-gazo02.png)[スマホ画面](images/gazo02.jpg)

---

## 2. 主な機能

*   **ファイルシステムアクセス:**
    *   SDカード (`SD_USE = true` の場合)
    *   SPIFFS (内蔵フラッシュメモリ上のファイルシステム, `SPIFFS_USE = true` の場合)
*   **Webインターフェース (レスポンシブデザイン):**

    *   **ファイル/ディレクトリ一覧表示:**
        *   SDカード: カレントディレクトリの内容 (ファイルとサブディレクトリ) を表示。
        *   SPIFFS: ルートにあるファイル一覧を表示。
        *   ファイル名、サイズを表示。
    *   **ファイル操作:**
        *   アップロード: PC/スマホからM5Stackへファイルを転送。
        *   ダウンロード: M5Stack上のファイルをPC/スマホへ保存。
        *   ストリーミング: 画像、音声、テキストなどをブラウザで直接表示。
        *   削除: ファイルを選択して削除 (確認画面あり)。
        *   リネーム: ファイル名を選択して変更。
        *   テキスト表示: `.txt`, `.log`, `.html` などのテキストファイルをブラウザ上で表示。
    *   **SDカード ディレクトリ操作:**
        *   ディレクトリ変更 (サブディレクトリへ移動)。
        *   ディレクトリ作成。
        *   ディレクトリ削除 (空の場合のみ、確認画面あり)。
        *   親ディレクトリ/ルートディレクトリへ移動。
*   **ネットワーク機能:**
    *   **WiFi接続:** 指定されたSSIDとパスワードでWiFiアクセスポイントに接続 (STAモード)。
    *   **mDNS:** `http://(設定したサーバー名)/`または、`http://(設定したサーバー名).local/` でアクセス可能。
    *   **設定ファイル:** SDカード/SPIFFSの `wifi.txt` で`main.cpp`のネットワーク設定を上書き可能。
*   **システム情報表示 (`/system`):**
    *   ファイルシステムの使用状況 (SD/SPIFFS)。
    *   メモリ使用状況 (SRAM/PSRAM)。
    *   NVS情報。
    *   CPU情報。
    *   ネットワーク情報 (IPアドレス、MACアドレスなど)。
    *   時刻情報 (RTC/NTP)。
    *   ファイル転送統計。
*   **Web API:**
    *   `/shutdown?reboot=on`: デバイスを再起動。
    *   `/shutdown`: デバイスをシャットダウン (電源オフ)。
*   **その他:**
    *   **NTP時刻同期:** 起動時にNTPサーバーから時刻を取得し、RTCを調整 (`RTC_ADJUST_REQ = true` の場合)。
    *   **SDカードアップデーター:** (オプション) SDカードからのファームウェア更新機能。
    *   **ディスプレイ表示:** (オプション) IPアドレスなどをM5Stack画面に表示。
    *   **Favicon/ホーム画像:** ルートに `favicon.ico` / `homeImg.gif` があれば表示。

## 3. ファイル構成 (主要ファイル)

*   **`main.cpp`**: アプリケーションのエントリーポイント。全体設定、初期化呼び出し、メインループ。
*   **`fileServer/fileServer.cpp`**: Webサーバーの基本設定、共通HTML生成、ホームページ(`/`)・システム情報ページ(`/system`)のハンドラ。
*   **`fileServer/fs_util.cpp`**: WiFi接続、mDNS、NTP時刻同期、設定ファイル読み込みなどの補助関数。
*   **`fileServer/SD_handler.cpp`**: SDカード関連のWebインターフェースとファイル/ディレクトリ操作処理。
*   **`fileServer/SPIFFS_handler.cpp`**: SPIFFS関連のWebインターフェースとファイル操作処理。
*   **`fileServer/webApi.cpp`**: `/shutdown` などのWeb APIエンドポイントの処理。
*   **`fileServer/fileServer.h`**: プロジェクト全体で共有される定義、宣言、インクルード。

## 4. セットアップと使用方法

1.  **ビルドと書き込み、Binファイル提供:**
    *   PlatformIOの開発環境でプロジェクトを開き、ターゲットデバイスを設定してビルドし、M5Stackに書き込みます。
    `m5stack-core2` `m5stack-core2-sdu` `m5stack-cores3`のデバイス環境を用意しています。他のデバイスの種類で使用する場合には、適宜`platformio.ini`にを追加してご使用ください。

    *   Binファイルの提供
           *   `m5stack-core2-sdu`で作成されるSD_Updater対応済みのBinファイルは、[`BinsPack`](https://github.com/NoRi-230401/BinsPack-for-StackChan-Core2)で提供します。
        `00_m5fileServer.bin`

2.  **ネットワーク設定:**
    *   **方法1 (推奨):** SDカードまたはSPIFFSのルートディレクトリに `wifi.txt` という名前のファイルを作成し、以下の形式で記述します。
        ```
        your_wifi_ssid
        your_wifi_ssid_password
        your_server_name
        ```
        (各行の末尾に改行を入れてください。`your_server_name` はmDNSで使用する名前です。)
    *   **方法2:** `main.cpp` 内の `YOUR_SSID`, `YOUR_SSID_PASS`, `YOUR_SERVER_NAME` の値を直接編集します。
3.  **アクセス:**
    *   M5Stackを起動すると、WiFiに接続し、IPアドレスとサーバー名がシリアルモニター (およびディスプレイが有効なら画面) に表示されます。
    *   同じネットワーク内のPCやスマートフォンのWebブラウザで、表示されたIPアドレス (`http://<IPアドレス>/`) またはmDNS名 (`http://<サーバー名>/`または`http://<サーバー名>.local/`) にアクセスします。
4.  **操作:**
    *   表示されたWebインターフェース上のメニューやボタンをクリックして、ファイル操作やシステム情報の確認を行います。

## 5. カスタマイズ

*   **機能の有効/無効化:** `main.cpp` の `SD_USE`, `SPIFFS_USE`, `DISP_ON`, `RTC_ADJUST_REQ` など、以下の定数を必要に応じて変更します。

```cpp
// main.cpp
const bool SD_USE = true;     // SDカードを使用する場合は true
const bool SPIFFS_USE = true; // SPIFFSを使用する場合は true
bool DISP_ON = true; 　　　　　// ディスプレイにメッセージを表示する場合は true
bool RTC_ADJUST_ON = true;    // RTC自動調整を行う場合は true
```


*   **ネットワーク設定:** 上記の `wifi.txt` または `main.cpp` 内のデフォルト値を変更します。
*   **外観:**
    *   SDカードまたはSPIFFSのルートに `favicon.ico` を置くと、ブラウザタブのアイコンが変わります。
    *   SDカードまたはSPIFFSのルートに `homeImg.gif` を置くと、ホームページに画像が表示されます。
        *   Githubの`CopyToSD_or_SPIFFS`フォルダ下に`favicon.ico`　`homeImg.gif`のサンプルがありますのでコピーしてご使用ください。


## 6.LICENSE
[MIT LICENSE](LICENSE)

*   author： NoRi

    
<br>

*   同梱した `homeImg.gif` と `favicon.ico`は、スタックチャン公開物のイラストを使用しています。
    -  [おきもくさんの公開物wiki](https://okimoku.com/wiki/%E3%82%A4%E3%83%A9%E3%82%B9%E3%83%88)<br>
画像の作成・使用許諾してくれた作者さんおよび公開物をまとめてくれた「おきもくさん」に感謝です。

    * [ｽﾀｯｸﾁｬﾝ](https://github.com/meganetaaan)は、`ししかわさん`が公開しているオープンソースのプロジェクトです。


## 7. Links

* Github:　https://github.com/NoRi-230401/m5stack-file-server

* M5Core2用BINS： https://github.com/NoRi-230401/BinsPack-for-StackChan-Core2

* SD-Updater：
https://github.com/tobozo/M5Stack-SD-Updater


---
