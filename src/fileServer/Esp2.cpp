// *******************************************************
//  Esp2.cpp          by NoRi 2025-05-08
//  -----------------------------------------------------
//  extens EspClass
// *******************************************************
#include "Esp2.h"

uint16_t Esp2Class::getChipFullRevision()
{
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  return chip_info.full_revision;
};

String Esp2Class::getChipFullRevisionStr()
{
  // fullRev format(3 decimac digit )
  //    : Mxx  --- M(major), xx(minor)
  uint16_t fullRev = getChipFullRevision();
  uint16_t major = (fullRev % 1000) / 100;
  uint16_t minor = fullRev % 100;

  String rev = "v" + String(major) + "." + String(minor);
  return rev;
};
