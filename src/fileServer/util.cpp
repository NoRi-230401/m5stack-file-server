// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-01
// -------------------------------------------------------
// util.cpp
// *******************************************************
#include "fileServer.h"

void getHeapInf();
void prtHeapInf(String message);
void prt(String message);
void error_stop();
bool wifiStart();
bool mdnsStart(void);
String ConvBytesUnits(uint64_t bytes, int dp, int unit);
String strTmInfo(struct tm &timeInfo);
String getTmNTP();
void adjustRTC();
String getTmRTC();
bool getSetting(int flType, const String filename);
bool FS_start(int flType);
uint64_t getFileSize(int flType, String filename);
bool SD_cardInfo(void);

static uint32_t HEAP_INF[8];
void getHeapInf()
{
  HEAP_INF[0] = ESP.getHeapSize();
  HEAP_INF[1] = ESP.getFreeHeap();
  HEAP_INF[2] = ESP.getMinFreeHeap();
  HEAP_INF[3] = ESP.getMaxAllocHeap();

  HEAP_INF[4] = ESP.getPsramSize();
  HEAP_INF[5] = ESP.getFreePsram();
  HEAP_INF[6] = ESP.getMinFreePsram();
  HEAP_INF[7] = ESP.getMaxAllocPsram();
}

void prtHeapInf(String message)
{
  if (message != "")
    Serial.println(message);

  for (int i = 0; i < 8; i++)
  {
    Serial.println("HeapInf[" + String(i) + "] = " + String(HEAP_INF[i] / 1024) + " KB");
    // Serial.println("HeapInf[" + String(i) + "] = " + String(HEAP_INF[i]) + " Bytes");
  }
}

void prt(String message)
{
  Serial.println(message);

  if (DISP_ON)
    M5.Display.println(message);
}

void error_stop()
{
  prt("\nERR: fail to start server");
  delay(10000);

  while (true)
    ;
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
  Serial.println("\nIP Address: " + IP_ADDR);
  return true;
}

bool mdnsStart(void)
{
  if (!MDNS.begin(SERVER_NAME.c_str()))
  {
    Serial.println("ERR: MDNS cannot start");
    Serial.println("ERR: ServerName = " + SERVER_NAME);
    return false;
  }

  Serial.println("mDNS ServerName = " + SERVER_NAME);
  return true;
}

void adjustRTC()
{
  struct tm tmInfo;

  while (!getLocalTime(&tmInfo, 1000U))
    delay(10);

  M5.Rtc.setDateTime(tmInfo);
  Serial.println("RTC adjusted .... " + strTmInfo(tmInfo));
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

  String errStr = "2025/01/01(Wed) 00:00:00";
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
    if (!SD.begin())
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

bool getSetting(int flType, const String filename)
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
    Serial.println("getSetting Err: invalid flType");
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
  SERVER_NAME = String(&buf[z]);
  Serial.println("SERVER_NAME = " + SERVER_NAME);

  if (SSID == "" || SSID_PASS == "" || SERVER_NAME == "")
    return false;

  return true;
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
