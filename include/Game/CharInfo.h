#ifndef GITEN_GAME_CHARINFO_H
#define GITEN_GAME_CHARINFO_H

#include <Game/Character.h>

typedef struct PartyMemberList {
    i16 count;
    i16 ids[1];
} PartyMemberList;

char* FormatFullName(char* buf, Character* character);
i16 CountRosterEntries(i16 all);
Character* FreeCharacterRecord(Character* character);
i16 FindRosterSlotIn(i16 id, i16 inParty);
Character* GetRosterCharacterById(i16 id, i16 inParty);
i16 TickActionWait(ActionWait* wait, i16 speed);
i16 TickPartyActionWaits(void);
i16 GetPickState(Character* character);
i16 FindReadyMember(i16 needMark);
PartyMemberList* ListPickableMembers(PartyMemberList* list, i16 max, i16 idleOnly);
i16 FindMemberByPoolState(i16 start, i16 mode, i16 state, u8 pools);

// charpool functions. Codegen constraint: declared here rather than in
// <Game/Stats.h>, where they flip CalcMagicAccuracyStat and CalcMagicEvasionStat.
// The pool state: 2 full, 1 partly spent, 0 empty.
i16 PoolState(CurMax* pool);

i32 ScalePercent999(i16 value, i16 percent);

#endif // GITEN_GAME_CHARINFO_H
