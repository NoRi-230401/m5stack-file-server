// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-15
// -------------------------------------------------------
// fs_util.cpp
// *******************************************************
#include "fileServer.h"
#include <M5StackUpdater.h>
#if defined(CARDPUTER)
#include <M5Cardputer.h>
SPIClass SPI2;
#endif

void prt(String message);
String ConvBytesUnits(uint64_t bytes, int dp, int unit);
bool wifiStart();
bool mdnsStart(void);
void adjustRTC();
String getTmNTP();
String getTmRTC();
String strTmInfo(struct tm &timeInfo);
bool getWiFiSettings(int flType, const String filename);
String urlEncode(const String &input);
String urlDecode(const String &input);
void requestManage();
void sendReq(int reqNo);
void STOP();
void REBOOT();
void POWER_OFF();
bool SPIFFS_begin();
void SPIFFS_start();
bool SD_begin();
void SD_start();
bool SD_cardInfo(void);
void DISP_start();
void m5stack_begin();
void SDU_lobby();
void disp(String msg);

// -------------------------------------------------------
uint32_t SHUTDOWN_TM_SEC = 3; // default 3sec after shutdown api
int REQUEST_NO = REQ_NONE;
bool SD_ENABLE, SPIFFS_ENABLE;

void prt(String message)
{
  Serial.println(message);

  if (DISP_ON)
  {
#ifdef CARDPUTER
    M5Cardputer.Display.println(message);
#else
    M5.Display.println(message);
#endif
  }
}

String ConvBytesUnits(uint64_t bytes, int dp, int unit)
{ // int dp : 小数点以下の桁数、decimal places
  const uint64_t KILO = 1024ULL;
  const uint64_t MEGA = KILO * KILO;
  const uint64_t GIGA = MEGA * KILO;
  const uint64_t TERA = GIGA * KILO;

  if (unit == UNIT_AUTO)
  {
    if (bytes < KILO)
    {
      return (String(bytes) + " B");
    }
    else if (bytes < MEGA)
    {
      float kb = (float)bytes / (float)KILO;
      return String(kb, dp) + " KB";
    }
    else if (bytes < GIGA)
    {
      float mb = (float)bytes / (float)MEGA;
      return (String(mb, dp) + " MB");
    }
    else if (bytes < TERA)
    {
      float gb = (float)bytes / (float)GIGA;
      return (String(gb, dp) + " GB");
    }
    else
    {
      float tb = (float)bytes / (float)TERA;
      return (String(tb, dp) + " TB");
    }
  }
  else if (unit == UNIT_KIRO)
  {
    float kb = (float)bytes / (float)KILO;
    return String(kb, dp) + " KB";
  }
  else if (unit == UNIT_MEGA)
  {
    float mb = (float)bytes / (float)MEGA;
    return (String(mb, dp) + " MB");
  }
  else if (unit == UNIT_GIGA)
  {
    float gb = (float)bytes / (float)GIGA;
    return (String(gb, dp) + " GB");
  }
  else if (unit == UNIT_TERA)
  {
    float tb = (float)bytes / (float)TERA;
    return (String(tb, dp) + " TB");
  }
  // UNIT_BYTE
  return (String(bytes) + " B");
}

bool wifiStart()
{
  WiFi.disconnect();
  delay(500);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, SSID_PASS);
  Serial.printf(".");
  int count = 1;
  const int COUNT_MAX = 20;
  delay(500);

  while (WiFi.status() != WL_CONNECTED)
  {
    count++;
    Serial.printf(".");
    delay(500);
    if (count >= COUNT_MAX)
    {
      Serial.println("\ncannot connect ,Wifi faile!");
      return false;
    }
  }

  IP_ADDR = WiFi.localIP().toString();
  return true;
}

bool mdnsStart(void)
{
  if (!MDNS.begin(HOST_NAME.c_str()))
  {
    Serial.println("ERR: MDNS cannot start");
    Serial.println("ERR: ServerName = " + HOST_NAME);
    return false;
  }

  Serial.println("mDNS ServerName = " + HOST_NAME);
  return true;
}

void adjustRTC()
{
  struct tm tmInfo;

  while (!getLocalTime(&tmInfo, 1000U))
    delay(10);

  M5.Rtc.setDateTime(tmInfo);
  Serial.println("\nRTC adjusted .... " + strTmInfo(tmInfo));
}

String getTmRTC()
{
  char buf[60];
  static constexpr const char *const wd[7] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};
  auto dt = M5.Rtc.getDateTime();
  sprintf(buf, "%04d/%02d/%02d(%s) %02d:%02d:%02d", dt.date.year, dt.date.month, dt.date.date, wd[dt.date.weekDay], dt.time.hours, dt.time.minutes, dt.time.seconds);

  return String(buf);
}

String getTmNTP()
{
  struct tm Ldt;
  for (int i = 0; i < 5; i++)
  {
    if (getLocalTime(&Ldt, 1000U))
      return strTmInfo(Ldt);

    delay(10);
  }

  String errStr = "2025/04/01(Tue) 00:00:00";
  return errStr;
}

String strTmInfo(struct tm &timeInfo)
{
  char buf[60];
  static constexpr const char *const wd[7] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};

  sprintf(buf, "%04d/%02d/%02d(%s) %02d:%02d:%02d",
          timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
          wd[timeInfo.tm_wday], timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);

  return String(buf);
}

bool getWiFiSettings(int flType, const String filename)
{
  File fs;
  if (flType == FS_SPIFFS)
  {
    if (!SPIFFS.exists(filename))
      return false;

    fs = SPIFFS.open(filename, FILE_READ);

    if (!fs)
      return false;
  }
  else if (flType == FS_SD)
  {
    if (!SD.exists(filename))
      return false;

    fs = SD.open(filename, FILE_READ);
    if (!fs)
      return false;
  }
  else
  {
    Serial.println("getWiFiSettings Err: invalid flType");
    return false;
  }

  size_t length = fs.size();
  if (length <= 3) // at least 3bytes size
    return false;

  char buf[length + 1];
  fs.read((uint8_t *)buf, length);
  buf[length] = 0;
  fs.close();

  int x;
  int y = 0;
  int z = 0;
  for (x = 0; x < length; x++)
  {
    if (buf[x] == 0x0a || buf[x] == 0x0d)
      buf[x] = 0;
    else if (!y && x > 0 && !buf[x - 1] && buf[x])
      y = x;
    else if (!z && x > 0 && !buf[x - 1] && buf[x])
      z = x;
  }

  if (y == 0)
    return false;
  SSID = String(buf);
  SSID_PASS = String(&buf[y]);
  Serial.println("SSID        = " + SSID);
  Serial.println("SSID_PASS   = " + SSID_PASS);

  if (z == 0)
    return false;
  HOST_NAME = String(&buf[z]);
  Serial.println("HOST_NAME = " + HOST_NAME);

  if (SSID == "" || SSID_PASS == "" || HOST_NAME == "")
    return false;

  return true;
}

// URLエンコード関数
String urlEncode(const String &input)
{
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < input.length(); i++)
  {
    c = input.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
    {
      encodedString += c;
    }
    else if (c == ' ')
    {
      encodedString += "%20";
    }
    else
    {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9)
      {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9)
      {
        code0 = c - 10 + 'A';
      }
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
  }
  return encodedString;
}

// URLデコード関数
String urlDecode(const String &input)
{
  String decodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < input.length(); i++)
  {
    c = input.charAt(i);
    if (c == '+')
    { // '+' はスペースとしてデコードする場合もあるが、ここでは%20のみ対応
      decodedString += ' ';
    }
    else if (c == '%')
    {
      i++;
      if (i < input.length())
      {
        code0 = input.charAt(i);
        i++;
        if (i < input.length())
        {
          code1 = input.charAt(i);
          char decodedChar = 0;
          // 16進文字を数値に変換
          if (code0 >= '0' && code0 <= '9')
            decodedChar = (code0 - '0') << 4;
          else if (code0 >= 'A' && code0 <= 'F')
            decodedChar = (code0 - 'A' + 10) << 4;
          else if (code0 >= 'a' && code0 <= 'f')
            decodedChar = (code0 - 'a' + 10) << 4;
          else
          {                       // 不正なエンコード形式
            decodedString += '%'; // '%'をそのまま追加
            i -= 2;               // インデックスを戻す
            continue;
          }

          if (code1 >= '0' && code1 <= '9')
            decodedChar |= (code1 - '0');
          else if (code1 >= 'A' && code1 <= 'F')
            decodedChar |= (code1 - 'A' + 10);
          else if (code1 >= 'a' && code1 <= 'f')
            decodedChar |= (code1 - 'a' + 10);
          else
          { // 不正なエンコード形式
            decodedString += '%';
            decodedString += code0;
            i--;
            continue;
          }
          decodedString += decodedChar;
        }
        else
        { // %XX の形式でない
          decodedString += '%';
          decodedString += code0;
        }
      }
      else
      { // 文字列末尾が %
        decodedString += '%';
      }
    }
    else
    {
      decodedString += c;
    }
  }
  return decodedString;
}

void requestManage()
{
  if (RTC_ADJUST_ON && RTC_ENABLE && (millis() - TM_SETUP_DONE > TM_RTC_ADJUST))
  {
    adjustRTC();
    RTC_ADJUST_ON = false;
  }

  if (REQUEST_NO == REQ_NONE)
    return;

  int req = REQUEST_NO;
  switch (req)
  {
  case REQ_REBOOT:
    REQUEST_NO = REQ_NONE;
    REBOOT();
    return;

  case REQ_SHUTDOWN:
    REQUEST_NO = REQ_NONE;
    // SHUTDOWN_TM_SEC = 0;
    POWER_OFF();
    return;

  default:
    REQUEST_NO = REQ_NONE;
    Serial.println("requeestManage : invalid request get ");
  }
  return;
}

void sendReq(int reqNo)
{
  REQUEST_NO = reqNo;
}

void STOP()
{
  Serial.println(" *** Stop *** fatal error");
  SD.end();
  SPIFFS.end();
  delay(5000);

  for (;;)
  {
    delay(1000);
  }
}

void REBOOT()
{
  Serial.println(" *** Reboot ***");
  SD.end();
  SPIFFS.end();
  delay(SHUTDOWN_TM_SEC * 1000L);
  ESP.restart();

  for (;;)
  { // never
    delay(1000);
  }
}

void POWER_OFF()
{
  Serial.println(" *** POWER OFF ***");

  SD.end();
  SPIFFS.end();
  delay(SHUTDOWN_TM_SEC * 1000L);
  M5.Power.powerOff();

  for (;;)
  { // never
    delay(1000);
  }
}

bool SPIFFS_begin()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("ERR: SPIFFS begin erro...");
    return false;
  }
  return true;
}

void SPIFFS_start()
{
  SPIFFS_ENABLE = false;
  if (SPIFFS_USE)
  {
    SPIFFS_ENABLE = SPIFFS_begin();
    if (SPIFFS_ENABLE)
      prt("SPIFFS  .....  OK");
    else
      prt("SPIFFS  .....  NG");
  }
}

// SPIClass SPI2;
bool SD_begin()
{
  int i = 0;

#if defined(CARDPUTER)
  // ------------- CARDPUTER -------------
  while (!SD.begin(M5.getPin(m5::pin_name_t::sd_spi_ss), SPI2) && i < 10)
#else
  // ----------- Core2 and CoreS3 ----------
  while (!SD.begin(GPIO_NUM_4, SPI, 25000000) && i < 10)
#endif
  {
    delay(500);
    i++;
  }

  if (i >= 10)
  {
    Serial.println("ERR: SD begin erro...");
    return false;
  }

  if (!SD_cardInfo())
    return false;

  return true;
}

void SD_start()
{
  // --- SD start ---
  if (SD_USE)
  {
    if (SD_ENABLE)
      prt("SD      .....  OK");
    else
      prt("SD      .....  NG");
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

void DISP_start()
{
  prt("- " + PROG_NAME + " -");
}

void disp(String msg)
{
#ifdef CARDPUTER
  M5Cardputer.Display.println(msg);
#else
  M5.Display.println(msg);
#endif
}

void m5stack_begin()
{
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;

#if defined(CARDPUTER)
  // ------------- CARDPUTER ---------------
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setBrightness(70);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.fillScreen(TFT_BLACK);
  M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Cardputer.Display.setFont(&fonts::Font0);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setTextWrap(false);
  M5Cardputer.Display.setCursor(0,0);

  SPI2.begin(
      M5.getPin(m5::pin_name_t::sd_spi_sclk),
      M5.getPin(m5::pin_name_t::sd_spi_miso),
      M5.getPin(m5::pin_name_t::sd_spi_mosi),
      M5.getPin(m5::pin_name_t::sd_spi_ss));

#else
  // ----------- Core2 and CoreS3 ----------
  M5.begin(cfg);
  M5.Display.setBrightness(120);
  M5.Display.setRotation(1);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setFont(&fonts::Font0);
  M5.Display.setTextSize(2);
  M5.Display.setTextWrap(false);
  M5.Display.setCursor(0,0);
  
#endif

  SD_ENABLE = SD_begin();
}

// ------------------------------------------
// SDU_lobby : SD_uploader lobby
// ------------------------------------------
// load "/menu.bin" on SD
//    if 'a' or 'BtnA' pressed at booting
// ------------------------------------------
void SDU_lobby()
{
  // CoreS3 は、最初からBtnAを押していると認識しないのでメッセージ表示後に押下する
  // Core2 と Cardputer は、最初から BtnA or 'a' を押していればいい。
#ifdef CORES3
  M5.Display.setCursor(0, M5.Display.height() / 2 - 30);
  M5.Display.setTextColor(GREEN);
  disp("  Press BtnA to load menu");
  delay(3000);
#endif

#if defined(CARDPUTER)
  M5Cardputer.update();
  if (M5Cardputer.Keyboard.isKeyPressed('a'))
#else
  M5.update();
  if (M5.BtnA.isPressed())
#endif
  {
    updateFromFS(SD, "/menu.bin");
    ESP.restart();

    while (true)
      ;
  }

#ifdef CORES3
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(TFT_WHITE,TFT_BLACK);
#endif
}
