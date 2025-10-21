// Generator version : v3.1
// Generation time   : 2025.10.21 21:33:02
// DBC filename      : Lecture.dbc
#include "lecture-fmon.h"

#ifdef LECTURE_USE_DIAG_MONITORS

/*
Put the monitor function content here, keep in mind -
next generation will completely clear all manually added code (!)
*/

#ifdef LECTURE_USE_MONO_FMON

void _FMon_MONO_lecture(FrameMonitor_t* _mon, uint32_t msgid)
{
  (void)_mon;
  (void)msgid;
}

#else

void _FMon_Cluster_lecture(FrameMonitor_t* _mon, uint32_t msgid)
{
  (void)_mon;
  (void)msgid;
}

#endif // LECTURE_USE_MONO_FMON

#endif // LECTURE_USE_DIAG_MONITORS
