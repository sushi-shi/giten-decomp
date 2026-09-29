#ifndef GITEN_SCRIPT_OBJECTREF_H
#define GITEN_SCRIPT_OBJECTREF_H

#include <EnumDomain.h>

// How a script names a character: directly, by character id, or by party or
// roster slot; the encoded forms add the matching base.
GZ_ENUM_BEGIN_SPLIT(ObjectRefKind, i16)
    OBJECT_REF_DIRECT = 0,
    OBJECT_REF_CHARACTER_ID = 1,
    OBJECT_REF_PARTY_SLOT = 2,
    OBJECT_REF_ROSTER_SLOT = 3
GZ_ENUM_END_SPLIT(ObjectRefKind)

GZ_ENUM_CONST_BEGIN(ObjectRefEncoding)
    SCRIPT_REF_PARTY_BASE = 1000,
    SCRIPT_REF_ROSTER_BASE = 2000,
    SCRIPT_REF_CHARACTER_BASE = 3000
GZ_ENUM_CONST_END(ObjectRefEncoding)

// The negative references ResolveScriptObject understands: character slot n is
// SCRIPT_REF_SLOT_BASE - n (ObjectSlotOfId), and the rest name the favoured
// party member, the script actor, the battle actor and target, and the fusion
// result.
GZ_ENUM_CONST_BEGIN(ScriptSpecialRef)
    SCRIPT_REF_SLOT_BASE = -1,
    SCRIPT_REF_FAVOURED_MEMBER = -16,
    SCRIPT_REF_ACTOR = -17,
    SCRIPT_REF_ACTOR_BY_ID = -18,
    SCRIPT_REF_ACTOR_ALIAS = -19,
    SCRIPT_REF_BATTLE_ACTOR = -20,
    SCRIPT_REF_BATTLE_TARGET = -21,
    SCRIPT_REF_FUSION_RESULT = -22,
    SCRIPT_REF_FUSION_RESULT_ALIAS = -23
GZ_ENUM_CONST_END(ScriptSpecialRef)

#endif // GITEN_SCRIPT_OBJECTREF_H
