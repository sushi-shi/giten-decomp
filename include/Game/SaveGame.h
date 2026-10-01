#ifndef GITEN_GAME_SAVEGAME_H
#define GITEN_GAME_SAVEGAME_H

#include <rva.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Ints.h>

#include <stdio.h>

// The save-file header: the leader's full name, level, the format version,
// the area name and the displayed floor; the names fill fixed text fields.
#define SAVE_TEXT_SIZE 32
#define SAVE_FORMAT_VERSION 4

// The header fields ReadSaveSummary reads up to.
GZ_ENUM_BEGIN(SaveSummaryField)
    SAVE_SUMMARY_NAME = 0,
    SAVE_SUMMARY_LEVEL = 1,
    SAVE_SUMMARY_AREA = 2,
    SAVE_SUMMARY_FLOOR = 3
GZ_ENUM_END(SaveSummaryField)

i16 ReadSaveHeader(FILE* fp);
i16 LoadGame(i16 slot, b16 keepField);
i16 SaveGame(i16 slot);
void RecordMarkInLeader(void);
i16 WriteSaveHeader(FILE* fp);
i16 ReadSaveSummary(i16 slot, GZ_ENUM_PARAM(SaveSummaryField, i16) field);

GZ_ENUM_BEGIN_SPLIT(SystemMenuChoiceStep, i16)
    SYSTEM_CHOICE_OPEN = 0,
    SYSTEM_CHOICE_POLL = 1
GZ_ENUM_END_SPLIT(SystemMenuChoiceStep)

// The system menu's game state (auto-mapping, auto-navigation, quit).
b16 RunSystemMenu(void);

// Set until the first save is loaded (the party then turns around and steps out).
extern b16 g_loadedBefore;

// The save loaders LoadGame chains; each returns its error count.
// @identity-TODO: label-only; named from what each reads.
i16 LoadCharacters(FILE* fp);   // the sixteen character records
i16 LoadFieldState(FILE* fp);   // g_party, member by member
i16 LoadAutomapAreas(FILE* fp); // the automap area store

struct Character;
i16 LoadCharacter(FILE* fp, struct Character* character);

// The save writers (charsave): nonzero on failure.
i16 WriteFieldState(FILE* fp);
i16 WriteCharacter(FILE* fp, struct Character* character);
i16 WriteCharacters(FILE* fp);

// The automap store writer (automapbits).
i16 WriteAutomapAreas(FILE* fp);

// character.c's lookup, for the roster load.
struct Character* FindCharacterById(i16 id);

// The action speed of a character (from its stats and equipment).
i16 ComputeActionSpeed(struct Character* character);

// Clamps a character's three affiliation bytes to -1..3 and compacts them.
void NormalizeAffiliations(struct Character* character);

#endif // GITEN_GAME_SAVEGAME_H
