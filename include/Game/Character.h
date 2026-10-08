#ifndef GITEN_GAME_CHARACTER_H
#define GITEN_GAME_CHARACTER_H

#include <Game/CharacterCore.h>

typedef struct Character {
    CharacterCore core;
    AlignmentInfo alignmentA;
    AlignmentInfo alignmentB;
    // @identity-TODO: initialized to 10000, 0 and 1 and retained in saved
    // records; no interpreted reader of these fields is known.
    i32 unusedValue;
    u8 unusedBytes[2];
} Character;

static __inline CharacterCore* GetCharacterCore(Character* character) {
    return character ? &character->core : NULL;
}

// The character table: party members, loaded demons and scratch slots.
#define CHARACTER_SLOT_COUNT 16
// GetCharacter maps this out-of-range selector to scratch slot 15, where
// script lookups load the pending fusion result.
#define CHARACTER_SLOT_FUSION_RESULT (-1)

extern Character g_characters[CHARACTER_SLOT_COUNT];

Character* GetCharacters(void);
Character* GetCharacter(i16 slot);
i16 GetCharacterId(i16 slot);

void RecalcCharacterStats(CharacterCore* character);

RVA_DECL(0x000404f0)
void UnequipPart(i16 slot, GZ_ENUM_PARAM(EquipPart, i16) part);

void FullyRestoreCharacter(CharacterCore* character);

void InitCharacters(void);
void InitCharacterSlot(
    i16 slot,
    i16 id,
    const char* prefix,
    const char* name,
    GZ_ENUM_PARAM(Gender, i16) gender
);

Character* CopyCharacterCore(CharacterCore* source, Character* destination);
Character* LoadCharacterCore(i16 id, Character* destination);
i16 FindCharacter(i16 id);

#endif // GITEN_GAME_CHARACTER_H
