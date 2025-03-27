// *******************************************************
//  m5stack-fileServer          by NoRi 2025-01-23
// -------------------------------------------------------
// fileServer.h
// *******************************************************
#ifndef _M5_FILE_SERVER_H
#define _M5_FILE_SERVER_H
// -------------------------------------------------------

#include <Arduino.h>
// #include <M5Unified.h>

typedef struct
{
  String filename;
  String ftype;
  String fsize;
} fileinfo;

// --- Status Report File System ------
#define STREP_FS_TOTALBYTES 11
#define STREP_FS_USEDBYTES 12
#define STREP_FS_FREESPACE 13
#define STREP_SD_TOTALBYTES 21
#define STREP_SD_USEDBYTES 22
#define STREP_SD_FREESPACE 23
#define STREP_SD_CARDTYPE 24
#define STREP_FS_START STREP_FS_TOTALBYTES
#define STREP_FS_END STREP_FS_FREESPACE
#define STREP_SD_START STREP_SD_TOTALBYTES
#define STREP_SD_END STREP_SD_CARDTYPE

// -------------------------------------------------------
#endif  // _M5_FILE_SERVER_H
