// Generator version : v3.1
// Generation time   : 2025.10.21 21:33:02
// DBC filename      : Lecture.dbc
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// DBC file version
#define VER_LECTURE_MAJ (0U)
#define VER_LECTURE_MIN (0U)

// include current dbc-driver compilation config
#include "lecture-config.h"

#ifdef LECTURE_USE_DIAG_MONITORS
// This file must define:
// base monitor struct
#include "canmonitorutil.h"

#endif // LECTURE_USE_DIAG_MONITORS


// DLC maximum value which is used as the limit for frame's data buffer size.
// Client can set its own value (not sure why) in driver-config
// or can test it on some limit specified by application
// e.g.: static_assert(TESTDB_MAX_DLC_VALUE <= APPLICATION_FRAME_DATA_SIZE, "Max DLC value in the driver is too big")
#ifndef LECTURE_MAX_DLC_VALUE
// The value which was found out by generator (real max value)
#define LECTURE_MAX_DLC_VALUE 0U
#endif

// The limit is used for setting frame's data bytes
#define LECTURE_VALIDATE_DLC(msgDlc) (((msgDlc) <= (LECTURE_MAX_DLC_VALUE)) ? (msgDlc) : (LECTURE_MAX_DLC_VALUE))

// Initial byte value to be filles in data bytes of the frame before pack signals
// User can define its own custom value in driver-config file
#ifndef LECTURE_INITIAL_BYTE_VALUE
#define LECTURE_INITIAL_BYTE_VALUE 0U
#endif


// def @Cluster CAN Message (101  0x65)
#define Cluster_IDE (0U)
#define Cluster_DLC (3U)
#define Cluster_CANID (0x65U)

typedef struct
{
#ifdef LECTURE_USE_BITS_SIGNAL

  uint8_t Left_Turn_Signal : 1;              //      Bits= 1

  uint16_t speed;                            //      Bits=12

  uint8_t Right_Turn_Signal : 1;             //      Bits= 1

#else

  uint8_t Left_Turn_Signal;                  //      Bits= 1

  uint16_t speed;                            //      Bits=12

  uint8_t Right_Turn_Signal;                 //      Bits= 1

#endif // LECTURE_USE_BITS_SIGNAL

#ifdef LECTURE_USE_DIAG_MONITORS

  FrameMonitor_t mon1;

#endif // LECTURE_USE_DIAG_MONITORS

} Cluster_t;

// Function signatures

uint32_t Unpack_Cluster_lecture(Cluster_t* _m, const uint8_t* _d, uint8_t dlc_);
#ifdef LECTURE_USE_CANSTRUCT
uint32_t Pack_Cluster_lecture(Cluster_t* _m, __CoderDbcCanFrame_t__* cframe);
#else
uint32_t Pack_Cluster_lecture(Cluster_t* _m, uint8_t* _d, uint8_t* _len, uint8_t* _ide);
#endif // LECTURE_USE_CANSTRUCT

#ifdef __cplusplus
}
#endif
