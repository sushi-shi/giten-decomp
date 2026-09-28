#ifndef GITEN_GAME_SAVEGAME_H
#define GITEN_GAME_SAVEGAME_H

#include <rva.h>

#include <Ints.h>

#include <stdio.h>

i16 ReadSaveHeader(FILE* fp);
i16 LoadGame(i16 slot, i16 keepField);
i16 SaveGame(i16 slot);
void RecordMarkInLeader(void);
i16 WriteSaveHeader(FILE* fp);
i16 ReadSaveSummary(i16 slot, i16 field);

// The system menu's game state (auto-mapping, auto-navigation, quit).
b16 RunSystemMenu(void);

// Set until the first save is loaded (the party then turns around and steps out).
extern i16 g_loadedBefore;

// The save loaders LoadGame chains; each returns its error count.
// @identity-TODO: label-only; named from what each reads.
i16 LoadCharacters(FILE* fp);   // the sixteen character records
i16 LoadFieldState(FILE* fp);   // g_field, g_savedDirection, party, roster
i16 LoadAutomapAreas(FILE* fp); // the automap area store
i16 LoadFieldMemory(FILE* fp);  // the field memory handle and 0x47b740
i16 LoadScreenLayers(FILE* fp); // the screen layer records (font.cpp)

struct Character;
i16 LoadCharacter(FILE* fp, struct Character* character);

// The save writers (charsave): nonzero on failure.
i16 WriteFieldState(FILE* fp);
i16 WriteCharacter(FILE* fp, struct Character* character);
i16 WriteCharacters(FILE* fp);

// The automap store writer (automapbits).
i16 WriteAutomapAreas(FILE* fp);

// The screen-layer records writer (LoadScreenLayers' mirror).
// @identity-TODO: label-only.
RVA_DECL(0x00054bb0)
i16 SaveScreenLayers(FILE* fp);

// character.c's lookup, for the roster load.
struct Character* FindCharacterById(i16 id);

// The action speed of a character (from its stats and equipment).
// @identity-TODO: label-only.
i16 ComputeActionSpeed(struct Character* character);

// Clamps a character's three affiliation bytes to -1..3 and compacts them.
void NormalizeAffiliations(struct Character* character);

#endif // GITEN_GAME_SAVEGAME_H
