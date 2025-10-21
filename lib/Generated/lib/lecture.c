// Generator version : v3.1
// Generation time   : 2025.10.21 21:33:02
// DBC filename      : Lecture.dbc
#include "lecture.h"


// DBC file version
#if (VER_LECTURE_MAJ != (0U)) || (VER_LECTURE_MIN != (0U))
#error The LECTURE dbc source files have different versions
#endif

#ifdef LECTURE_USE_DIAG_MONITORS
// Function prototypes to be called each time CAN frame is unpacked
// FMon function may detect RC, CRC or DLC violation
#include "lecture-fmon.h"

#endif // LECTURE_USE_DIAG_MONITORS

// This macro guard for the case when you need to enable
// using diag monitors but there is no necessity in proper
// SysTick provider. For providing one you need define macro
// before this line - in dbccodeconf.h

#ifndef GetSystemTick
#define GetSystemTick() (0u)
#endif

// This macro guard is for the case when you want to build
// app with enabled optoin auto CSM, but don't yet have
// proper getframehash implementation

#ifndef GetFrameHash
#define GetFrameHash(a,b,c,d,e) (0u)
#endif

// This function performs extension of sign for the signals
// whose bit width value is not aligned to one of power of 2 or less than 8.
// The types 'bitext_t' and 'ubitext_t' define the biggest bit width which
// can be correctly handled. You need to select type which can contain
// n+1 bits where n is the largest signed signal width. For example if
// the most wide signed signal has a width of 31 bits you need to set
// bitext_t as int32_t and ubitext_t as uint32_t
// Defined these typedefs in @dbccodeconf.h or locally in 'dbcdrvname'-config.h
static bitext_t __ext_sig__(ubitext_t val, uint8_t bits)
{
  ubitext_t const m = (ubitext_t) (1u << (bits - 1u));
  return ((val ^ m) - m);
}

uint32_t Unpack_Cluster_lecture(Cluster_t* _m, const uint8_t* _d, uint8_t dlc_)
{
  (void)dlc_;
  _m->Left_Turn_Signal = (uint8_t) ( (_d[1] & (0x01U)) );
  _m->speed = (uint16_t) ( ((_d[2] & (0x1FU)) << 7U) | ((_d[1] >> 1U) & (0x7FU)) );
  _m->Right_Turn_Signal = (uint8_t) ( ((_d[2] >> 5U) & (0x01U)) );

#ifdef LECTURE_USE_DIAG_MONITORS
  _m->mon1.dlc_error = (dlc_ < Cluster_DLC);
  _m->mon1.last_cycle = GetSystemTick();
  _m->mon1.frame_cnt++;

  FMon_Cluster_lecture(&_m->mon1, Cluster_CANID);
#endif // LECTURE_USE_DIAG_MONITORS

  return Cluster_CANID;
}

#ifdef LECTURE_USE_CANSTRUCT

uint32_t Pack_Cluster_lecture(Cluster_t* _m, __CoderDbcCanFrame_t__* cframe)
{
  uint8_t i; for (i = 0u; i < LECTURE_VALIDATE_DLC(Cluster_DLC); cframe->Data[i++] = LECTURE_INITIAL_BYTE_VALUE);

  cframe->Data[1] |= (uint8_t) ( (_m->Left_Turn_Signal & (0x01U)) | ((_m->speed & (0x7FU)) << 1U) );
  cframe->Data[2] |= (uint8_t) ( ((_m->speed >> 7U) & (0x1FU)) | ((_m->Right_Turn_Signal & (0x01U)) << 5U) );

  cframe->MsgId = (uint32_t) Cluster_CANID;
  cframe->DLC = (uint8_t) Cluster_DLC;
  cframe->IDE = (uint8_t) Cluster_IDE;
  return Cluster_CANID;
}

#else

uint32_t Pack_Cluster_lecture(Cluster_t* _m, uint8_t* _d, uint8_t* _len, uint8_t* _ide)
{
  uint8_t i; for (i = 0u; i < LECTURE_VALIDATE_DLC(Cluster_DLC); _d[i++] = LECTURE_INITIAL_BYTE_VALUE);

  _d[1] |= (uint8_t) ( (_m->Left_Turn_Signal & (0x01U)) | ((_m->speed & (0x7FU)) << 1U) );
  _d[2] |= (uint8_t) ( ((_m->speed >> 7U) & (0x1FU)) | ((_m->Right_Turn_Signal & (0x01U)) << 5U) );

  *_len = (uint8_t) Cluster_DLC;
  *_ide = (uint8_t) Cluster_IDE;
  return Cluster_CANID;
}

#endif // LECTURE_USE_CANSTRUCT

