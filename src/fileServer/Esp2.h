// *******************************************************
//  Esp2.h          by NoRi 2025-05-08
// *******************************************************
#ifndef _ESP2_H
#define _ESP2_H
// -------------------------------------------------------

#include <Arduino.h>
#include <Esp.h>
#include <esp_chip_info.h>
#include <esp32-hal-cpu.h>

class Esp2Class : public EspClass
{
public:
  uint16_t getChipFullRevision();
  String getChipFullRevisionStr();

  uint32_t getXtalFreqMHz()
  { // Xtal Frequency in MHz
    return getXtalFrequencyMhz();
  };

  uint32_t getApbFreqMHz()
  {// Advanced Peripheral Bus Frequency in MHz
    return getApbFrequency() / 1000000UL ;
  };
};

// -------------------------------------------------------
#endif // _ESP2_H
