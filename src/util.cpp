// *******************************************************
//  m5stack-fileServer          by NoRi 2025-04-01
// -------------------------------------------------------
// util.cpp
// *******************************************************
#include "fileServer/fileServer.h"

void getHeapInf();
void prtHeapInf(String message);
void prt(String message);
void error_stop();
bool wifiStart();
bool mdnsStart(void);
String ConvBytesUnits(uint64_t bytes, int dp, int unit);
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
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, SSID_PASS);
  // M5.Display.printf(".");
  Serial.printf(".");
  int count = 1;
  const int COUNT_MAX = 10;
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    count++;
    // M5.Display.printf(".");
    Serial.printf(".");
    WiFi.disconnect(false);
    // delay(500);
    WiFi.begin(SSID, SSID_PASS);
    if (count >= COUNT_MAX)
    {
      Serial.printf("\ncannot connect ,Wifi faile!");
      return false;
    }
  }

  IP_ADDR = WiFi.localIP().toString();
  Serial.println("\nIP Address: " + IP_ADDR);
  if (WiFi.scanComplete() == -2)
    WiFi.scanNetworks(true);
  // Complete an initial scan for WiFi networks,
  //  otherwise = 0 on first display!

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
