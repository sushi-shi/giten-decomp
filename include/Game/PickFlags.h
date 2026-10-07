#ifndef GITEN_GAME_PICKFLAGS_H
#define GITEN_GAME_PICKFLAGS_H

#include <Ints.h>

// While itemSkill is set, pickItem holds the item and pickTarget its skill.
// @identity-TODO: recordFlagA and recordFlagB are copied from ObjectRecordFlags
// and have no scalar reader.
typedef struct PickFlags {
    // Signed storage preserves the first record-bit assignment's operand order.
    i8 recordFlagA : 1;
    u8 recordFlagB : 1;
    u8 itemSkill : 1;
} PickFlags;

#endif // GITEN_GAME_PICKFLAGS_H
