// Generator version : v3.1
// Generation time   : 2025.10.21 21:33:02
// DBC filename      : Lecture.dbc
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "dbccodeconf.h"

#include "lecture.h"

// There is no any RX mapped massage.

typedef struct
{
  Cluster_t Cluster;
} ecu_student_lecture_tx_t;

#ifdef __DEF_ECU_STUDENT_LECTURE__

extern ecu_student_lecture_tx_t ecu_student_lecture_tx;

#endif // __DEF_ECU_STUDENT_LECTURE__

#ifdef __cplusplus
}
#endif
