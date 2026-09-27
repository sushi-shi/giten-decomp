#ifndef GITEN_GAME_ITEMBONUS_H
#define GITEN_GAME_ITEMBONUS_H

#include <Game/Character.h>
#include <Game/GemItems.h>
#include <Ints.h>

// The item id of a slot's gem index (-1 stays -1).
i16 GemItemId(i16 index);

// Adds item `item`'s bonuses to the 24-entry derived-stat array `bonuses`
// (indexed like Character.battleStats); with `indexed` set, `item` is a
// slot's 5-bit index into the gem items instead of an id. Items whose
// equipment effect byte is negative add nothing.
// @identity-TODO: which stats the indices and the kinds (11..19) name, and
// what the record's code byte (+0x2c, 0x30..0x3b) stands for, are unrecovered.
void AddItemStatBonuses(i16 item, i16* bonuses, i16 indexed);

// Signed equipment bonuses (0 for item 0 or -1). Physical evasion applies
// to the weapon or gun column according to the item kind.
i16 GetItemMagicDefenseBonus(i16 item);
i16 GetItemMagicAccuracyBonus(i16 item);
i16 GetItemPhysicalEvasionBonus(i16 item);
i16 GetItemMagicPowerBonus(i16 item);

// Adds item `item`'s stat points to the eleven-stat array `stats`: one point
// to each stat its record's stat code (or, for a gem item, its own code
// byte) names.
// @identity-TODO: label-only until its body is claimed; the codes' meaning is
// unrecovered.
void AddItemStatPoints(i16 item, i16* stats);

#endif // GITEN_GAME_ITEMBONUS_H
