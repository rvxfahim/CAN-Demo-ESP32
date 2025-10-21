// Generator version : v3.1
// Generation time   : 2025.10.21 21:33:02
// DBC filename      : Lecture.dbc
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "dbccodeconf.h"

#include "lecture.h"

typedef struct
{
  Cluster_t Cluster;
} vector__xxx_lecture_rx_t;

// There is no any TX mapped massage.

uint32_t vector__xxx_lecture_Receive(vector__xxx_lecture_rx_t* m, const uint8_t* d, uint32_t msgid, uint8_t dlc);

#ifdef __DEF_VECTOR__XXX_LECTURE__

extern vector__xxx_lecture_rx_t vector__xxx_lecture_rx;

#endif // __DEF_VECTOR__XXX_LECTURE__

#ifdef __cplusplus
}
#endif
