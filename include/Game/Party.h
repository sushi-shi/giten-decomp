#ifndef GITEN_GAME_PARTY_H
#define GITEN_GAME_PARTY_H

#include <rva.h>

#include <Game/Character.h>
#include <Ints.h>

// Installs `character` as roster entry `slot` and returns the previous entry.
Character* SetRosterEntry(i16 slot, Character* character);
i16 GetRosterId(i16 slot);
i16 GetEquipItem(Character* character, i16 part);
void SetPartySlot(i16 index, i16 slot);
void ClearPartyPosition(i16 index);
void RemoveFromParty(i16 slot);
i16 CountPartyMembers(i16 skipDisabled);
i16 FindRosterSlotById(i16 id);

static __inline b32 RosterContainsId(i16 id) {
    return FindRosterSlotById(id) != -1;
}

i16 RosterSlotOfId(i16 id);
b16 SortRoster(void);
i16 FilterPartyMember(i16 slot, i16 mode);
i16 DamageParty(i16 percent, i16 skipId13);
i16 HealParty(i16 percent);
Character* CopyCharacter(Character* src, Character* dst);
i32 AddMacca(Character* character, i32 amount);
i32 AddMagnetite(Character* character, i32 amount);
i16 CompareMacca(i16 who, i32 amount);
i16 GetPartyMemberId(i16 index);
void SwapPartySlots(i16 a, i16 b);
void LoadEquipTable(void);
i16 AddToRoster(Character* character);
i16 RemoveFromRoster(i16 slot);
i16 AddToParty(i16 slot);

// The equipment part (0..7) of an item of kind 11..19.
// @identity-TODO: label-only; for other kinds it returns its argument (no
// return statement on that path), which callers only test for a sign.
RVA_DECL(0x00040290)
i16 EquipPartOfItem(struct ItemRecord* item);

// charpool's pool change (drain for a negative amount). Codegen constraint:
// declared here; in <Game/Stats.h> or <Game/CharInfo.h> it flips charpool's
// CalcMagicAccuracyStat.
void ChangePool(CurMax* pool, i32 amount);

// Copies `from`'s skill list into `to` (resized to fit).
// @identity-TODO: label-only until its TU is claimed.
void CopySkillList(Character* from, WordList* to);
b16 CanGroupEquip(i16 group, i16 item);
i16 GetGunAmmoType(Character* character);
i16 CanEquipItem(Character* character, i16 item);
void NormalizeEquipSlots(Character* character);
void NormalizeItemSlot(ItemSlot* slot);

ItemSlot GetRosterEquipSlot(i16 slot, i16 part);

ItemSlot SwapEquipSlot(i16 slot, ItemSlot item, i16* result);
// @identity-TODO: `count` is ReadBagEntry's count; for ammunition it is kept
// in the character's `ammoCounts` entry.
ItemSlot EquipItem(i16 slot, ItemSlot item, i16 count, i16 index);

// The index into Character `ammoCounts` that equipping ammunition writes:
// a read-only -1 in retail, so the store never runs.
extern const i16 g_ammoCountIndex;

// Sets equipment part `part` of roster member `slot`. When `check` is set,
// changing the gun unequips ammunition that no longer fits it.
i16 SetEquipSlot(i16 slot, i16 part, ItemSlot item, i16 check);

void SetPartySlot(i16 index, i16 slot);

// Adds `amount` magnetite to `character`, clamped to 0..9999999.
RVA_DECL(0x00040920)
i32 AddMagnetite(Character* character, i32 amount);

#endif // GITEN_GAME_PARTY_H
