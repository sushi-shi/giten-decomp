#ifndef GITEN_SCRIPT_ROSTERQUERYRESULT_H
#define GITEN_SCRIPT_ROSTERQUERYRESULT_H

#include <EnumDomain.h>

// Whether a roster query returns the first matching slot or a mask of all.
GZ_ENUM_BEGIN_SPLIT(RosterQueryResult, i16)
    ROSTER_QUERY_FIRST_SLOT = 0,
    ROSTER_QUERY_SLOT_MASK = 1
GZ_ENUM_END_SPLIT(RosterQueryResult)

#endif // GITEN_SCRIPT_ROSTERQUERYRESULT_H
