// Generator version : v3.1
// Generation time   : 2025.10.21 21:33:02
// DBC filename      : Lecture.dbc
#include "vector__xxx_lecture-binutil.h"

// DBC file version
#if (VER_LECTURE_MAJ != (0U)) || (VER_LECTURE_MIN != (0U))
#error The VECTOR__XXX_LECTURE binutil source file has inconsistency with core dbc lib!
#endif

#ifdef __DEF_VECTOR__XXX_LECTURE__

vector__xxx_lecture_rx_t vector__xxx_lecture_rx;

#endif // __DEF_VECTOR__XXX_LECTURE__

uint32_t vector__xxx_lecture_Receive(vector__xxx_lecture_rx_t* _m, const uint8_t* _d, uint32_t _id, uint8_t dlc_)
{
 uint32_t recid = 0;
 if (_id == 0x65U) {
  recid = Unpack_Cluster_lecture(&(_m->Cluster), _d, dlc_);
 }

 return recid;
}

