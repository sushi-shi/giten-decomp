#ifndef GITEN_SCRIPT_EVENTFLAGS_H
#define GITEN_SCRIPT_EVENTFLAGS_H

#include <rva.h>

#include <EnumDomain.h>
#include <Ints.h>
#include <Util/BitChangeMode.h>

#include <stdio.h>

// One 256-bit bank of the saved event flags. Bank 15 doubles as a settings
// record.
// @identity-TODO: the owners of bank 15's packed word and five-byte tag are
// unrecovered.
typedef union FlagBank {
    u8 bits[32];
    u32 words[8];
    struct {
        u32 packed;
        u8 tag[5];
    } sys;
} FlagBank;

// The event-flag banks, saved and loaded as one 512-byte block. Bank
// EVENT_FLAG_BANK_ACTOR is the script actor's own flags while there is one;
// EVENT_FLAG_BANK_SYSTEM holds the flag settings and tag.
#define EVENT_FLAG_BANK_COUNT 16
#define EVENT_FLAG_BANK_WORDS 8
// The scenario flags (ScenarioFlag, ScenarioFlag2), the maps and programs the
// player holds (OwnedFlag), and the opened treasure boxes.
// A new level resets EVENT_FLAG_BANK_LEVEL (unless its flags are preserved) and
// clears EVENT_FLAG_BANK_LEVEL_SCRATCH; a new area resets EVENT_FLAG_BANK_AREA
// (unless preserved) and EVENT_FLAG_BANK_SCRATCH, which each scene or actor
// script also clears before it runs.
GZ_ENUM_BEGIN_SPLIT(EventFlagBank, u8)
    EVENT_FLAG_BANK_SCENARIO = 0,
    EVENT_FLAG_BANK_SCENARIO_2 = 1,
    EVENT_FLAG_BANK_OWNED = 2,
    EVENT_FLAG_BANK_BOXES = 4,
    EVENT_FLAG_BANK_ITEM_EFFECTS = 7,
    EVENT_FLAG_BANK_LEVEL = 8,
    EVENT_FLAG_BANK_AREA = 9,
    EVENT_FLAG_BANK_SCRATCH = 12,
    EVENT_FLAG_BANK_LEVEL_SCRATCH = 13,
    EVENT_FLAG_BANK_ACTOR = 14,
    EVENT_FLAG_BANK_SYSTEM = 15
GZ_ENUM_END_SPLIT(EventFlagBank)

// Level flag 0 is the developers' "敵無し" (no enemies): a new level and the
// removal of its last field object set it, spawning an object clears it, and
// random enemies spawn only while it is clear.
#define LEVEL_FLAG_NO_ENEMIES 0
GZ_ENUM_CONST_BEGIN(AreaFlag)
    AREA_FIXED_BACKGROUND = 0x7b
GZ_ENUM_CONST_END(AreaFlag)
extern FlagBank g_eventFlags[EVENT_FLAG_BANK_COUNT];

// Bank-7 flags: a trap shield and two items unavailable until the full moon.
GZ_ENUM_BEGIN_SPLIT(ItemEffectFlag, u16)
    ITEM_EFFECT_CORE_SHIELD = 0xfd,
    ITEM_EFFECT_SOMA_CUP_USED = 0xfe,
    ITEM_EFFECT_KUSHINADA_JAR_USED = 0xff
GZ_ENUM_END_SPLIT(ItemEffectFlag)

// A flag word or operand: the bank in bits 0-6, a negate bit, and (in a word)
// the index in the high byte; bank FLAG_BANK_MASK with index 0xff always reads
// set, and ends a flag list.
#define FLAG_BANK_MASK 0x7f
#define FLAG_NEGATE 0x80
#define FLAG_INDEX_MASK 0xff00

i16 CheckFlagWord(u16* condition);
b16 MatchFlagWord(u16* condition);

void SetFlagBank(i16 bank);
void ClearFlagBank(i16 bank);
b32 ChangeEventFlag(u16 bank, u16 index, GZ_ENUM_PARAM(BitChangeMode, i16) op);
b32 ClearEventFlag(u16 bank, u16 index);
b32 SetEventFlag(u16 bank, u16 index);
b32 ToggleEventFlag(u16 bank, u16 index);
b32 IsEventFlagSet(u16 bank, u16 index);
b32 ModifyEventFlag(u16 bank, u16 index, GZ_ENUM_PARAM(BitChangeMode, i16) op);
b32 TestEventFlag(u16 bank, u16 index);
u32 GetFlagSettings(void);
void SetFlagSettings(u32 packed);

void OpModifyEventFlagByValue(void);

void OpTestEventFlagByValue(void);

// Store a value into script variable N (g_scriptVars[N]).
RVA_DECL(0x000395d0)
void OpStoreScriptVar(void);

// Load script variable N into a script register.
RVA_DECL(0x000395f0)
void OpLoadScriptVar(void);

void OpModifyEventFlag(void);

void OpTestEventFlag(void);

// The save-file sections of the flag banks and script variables.
i16 ReadEventFlags(FILE* fp);
i16 WriteEventFlags(FILE* fp);
i16 ReadScriptVars(FILE* fp);
i16 WriteScriptVars(FILE* fp);

// Whether event flag bank/index matches (a script block entry's condition).
b16 MatchEventFlag(u16 bank, u16 index);
b16 ReadAndMatchEventFlag(void);
i16 ReadFlagOperand(u16* bank, u16* index);

void ResetEventFlags(void);

#endif // GITEN_SCRIPT_EVENTFLAGS_H
