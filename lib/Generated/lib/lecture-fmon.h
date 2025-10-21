// Generator version : v3.1
// Generation time   : 2025.10.21 21:33:02
// DBC filename      : Lecture.dbc
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// DBC file version
#define VER_LECTURE_MAJ_FMON (0U)
#define VER_LECTURE_MIN_FMON (0U)

#include "lecture-config.h"

#ifdef LECTURE_USE_DIAG_MONITORS

#include "canmonitorutil.h"
/*
This file contains the prototypes of all the functions that will be called
from each Unpack_*name* function to detect DBC related errors
It is the user responsibility to defined these functions in the
separated .c file. If it won't be done the linkage error will happen
*/

#ifdef LECTURE_USE_MONO_FMON

void _FMon_MONO_lecture(FrameMonitor_t* _mon, uint32_t msgid);

#define FMon_Cluster_lecture(x, y) _FMon_MONO_lecture((x), (y))

#else

void _FMon_Cluster_lecture(FrameMonitor_t* _mon, uint32_t msgid);

#define FMon_Cluster_lecture(x, y) _FMon_Cluster_lecture((x), (y))

#endif

#endif // LECTURE_USE_DIAG_MONITORS

#ifdef __cplusplus
}
#endif
