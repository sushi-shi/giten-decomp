// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/Condition.h>
#include <Game/ConditionAge.h>
#include <Game/EquipEffect.h>
#include <Game/EquipScreen.h>
#include <Game/FieldMain.h>
#include <Game/FieldSupport.h>
#include <Game/GameState.h>
#include <Game/Guest.h>
#include <Game/ItemBag.h>
#include <Game/ItemEffect.h>
#include <Game/ItemRecord.h>
#include <Game/LevelUp.h>
#include <Game/Party.h>
#include <Game/PartyPick.h>
#include <Game/SkillUse.h>
#include <Game/Stats.h>
#include <Gfx/ScreenLayer.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Script/Script.h>
#include <Util/BitSet.h>
#include <Util/Range.h>

#include <stddef.h>
#include <stdlib.h>

// Per character group, the 40-bit set of items the group can equip.
DATA(0x00083b38)
static i32 s_equipTable;

// Returns `slot` when it is in the party and `mode` bit 0 is set, or when it is
// not and bit 1 is set; -1 otherwise.
RVA(0x0003f780, 0x38)
i16 FilterPartyMember(i16 slot, i16 mode) {
    i16 index = FindPartySlot(slot);
    if (index != -1 && (mode & 1)) {
        return slot;
    }
    if (index == -1 && (mode & 2)) {
        return slot;
    }
    return -1;
}

// Installs `character` in a free roster slot; a human member (id below 32)
// also joins the party. Non-humans are refused (-1) once 26 entries exist.
// Returns the slot, or -1.
RVA(0x0003f7c0, 0x8a)
i16 AddToRoster(Character* character) {
    i16 slot;
    if (!IsHumanCharacter(character) && CountRosterEntries(0) >= 26) {
        return -1;
    }
    slot = FindEmptySlot(0);
    if (slot != -1) {
        SetRosterEntry(slot, character);
        if (IsHumanCharacter(character)) {
            StripZeroWords(GetCharacterSkills(character));
            AddToParty(slot);
            RaiseExperienceToLevel(character);
            return slot;
        }
        KeepFirstSixWords(GetCharacterSkills(character));
        RaiseExperienceToLevel(character);
    }
    return slot;
}

// Takes roster slot `slot` out of the party and the roster; a non-human's
// record is freed. Returns the slot, or -1 when it was empty.
RVA(0x0003f850, 0x5f)
i16 RemoveFromRoster(i16 slot) {
    i16 index;
    Character* character = GetRosterEntry(slot);
    if (!character) {
        return -1;
    }
    index = FindPartySlot(slot);
    SetPartySlot(index, -1);
    if (!IsHumanCharacter(character)) {
        FreeWordList(GetCharacterSkills(character));
        FreeBlock(character);
    }
    SetRosterEntry(slot, NULL);
    return slot;
}

// Puts roster slot `slot` into an empty party position; when the party is
// full, a human member replaces the first non-human one.
RVA(0x0003f8b0, 0x5a)
i16 AddToParty(i16 slot) {
    i16 index = FindEmptySlot(1);
    if (index == -1) {
        i16 id = GetRosterId(slot);
        if (id >= 0 && id < 32) {
            for (index = 0; index < 6; index++) {
                if (GetPartyRosterId(index) >= 32) {
                    break;
                }
            }
            if (index == 6) {
                index = -1;
            }
        } else {
            index = -1;
        }
    }
    return ExchangePartySlot(index, slot);
}

RVA(0x0003f910, 0x25)
i16 ExchangePartySlot(i16 index, i16 slot) {
    i16 prev = GetPartySlot(index);
    SetPartySlot(index, slot);
    return prev;
}

RVA(0x0003f940, 0x10)
void ClearPartyPosition(i16 index) {
    SetPartySlot(index, -1);
}

// Takes roster slot `slot` out of the party.
RVA(0x0003f950, 0x17)
void RemoveFromParty(i16 slot) {
    ClearPartyPosition(FindPartySlot(slot));
}

// The members present; with `skipDisabled`, only those without a disabling
// condition.
RVA(0x0003f970, 0x40)
i16 CountPartyMembers(i16 skipDisabled) {
    i16 count = 0;
    i16 i;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (character) {
            if (!skipDisabled || !GetDisablingCondition(GetCharacterConditions(character))) {
                count++;
            }
        }
    }
    return count;
}

// Ages every member's conditions by one and rolls them for recovery; nonzero
// when any wore off.
RVA(0x0003f9b0, 0x40)
i16 TickPartyConditions(void) {
    i16 recovered = 0;
    i16 i;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (character) {
            AgeConditions(GetCharacterConditions(character), 1);
            recovered |= RecoverConditions(character);
        }
    }
    return recovered;
}

static __inline i32 PoolPercentAmount(const CurMax* pool, i16 percent) {
    i32 amount = pool->max * percent / 100;
    amount = max(1, amount);
    return amount;
}

// Takes `percent` of each living member's maximum HP (at least 1; a negative
// `percent` takes that many points), then applies what an empty pool brings.
// With `skipId13`, character 13 is spared. Returns how many were hit.
// @identity-TODO: who character 13 is is unrecovered.
RVA(0x0003f9f0, 0xa1)
i16 DamageParty(i16 percent, i16 skipId13) {
    i16 hit = 0;
    i16 i;
    i32 amount;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (character && !GetFatalCondition(GetCharacterConditions(character))) {
            if (skipId13 && character->id == 13) {
                continue;
            }
            if (percent < 0) {
                ChangePool(&character->pools.hp, percent);
            } else {
                amount = PoolPercentAmount(&character->pools.hp, percent);
                ChangePool(&character->pools.hp, -amount);
            }
            ApplyEmptyPools(character);
            hit++;
        }
    }
    return hit;
}

// Restores `percent` of each living member's maximum HP (at least 1; a
// negative `percent` restores that many points). Returns how many were healed.
RVA(0x0003faa0, 0x90)
i16 HealParty(i16 percent) {
    i16 healed = 0;
    i16 i;
    i32 amount;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (character && !GetFatalCondition(GetCharacterConditions(character))) {
            if (percent < 0) {
                ChangePool(&character->pools.hp, -percent);
            } else {
                amount = PoolPercentAmount(&character->pools.hp, percent);
                ChangePool(&character->pools.hp, amount);
            }
            ApplyEmptyPools(character);
            healed++;
        }
    }
    return healed;
}

// Ends a battle for every roster member: clears its battle conditions and
// tally, its shield and its field mark.
RVA(0x0003fb30, 0x4a)
void ResetRosterBattleState(void) {
    i16 slot;
    Character* character;
    for (slot = 0; slot < 32; slot++) {
        character = GetRosterCharacter(slot);
        if (character) {
            ClearBattleConditions(GetCharacterConditions(character));
            ResetBattleTally(character);
            ClearActionWait(GetCharacterActionWait(character));
        }
    }
}

// Clears every roster member's conditions and recomputes its stats.
RVA(0x0003fb80, 0x35)
void ClearRosterConditions(void) {
    i16 slot;
    Character* character;
    for (slot = 0; slot < 32; slot++) {
        character = GetRosterCharacter(slot);
        if (character) {
            ClearAllConditions(GetCharacterConditions(character));
            RecalcCharacterStats(character);
        }
    }
}

// Copies `src` into `dst` (a new record when NULL; an existing one loses its
// skill list first) with its own copy of the skill list. Returns `dst`.
RVA(0x0003fbc0, 0x70)
Character* CopyCharacter(Character* src, Character* dst) {
    if (!src) {
        return dst;
    }
    if (!dst) {
        dst = AllocCleared(1, sizeof(Character));
    } else {
        FreeWordList(GetCharacterSkills(dst));
    }
    *dst = *src;
    InitEmptyWordList(&dst->skills);
    CopySkillList(src, GetCharacterSkills(dst));
    return dst;
}

// @identity-TODO: That 0x556b0 (word 0x90b10 while layer 1 is shown) is the character the
// layer-1 panel shows is inferred; also hides layer 1 as a side effect.
// Counts the fallen human members; hides layer 1 when the character it shows
// has fallen or left.
RVA(0x0003fc30, 0x74)
i16 CountFallenHumans(void) {
    i16 shown = GetShownPanelCharacter();
    i16 fallen = 0;
    b16 found = false;
    i16 i;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (character) {
            if (!GetFatalCondition(GetCharacterConditions(character))) {
                if (character->id == shown) {
                    found = true;
                }
            } else if (IsHumanCharacter(character)) {
                fallen++;
            }
        }
    }
    if (shown != -1 && !found) {
        HideScreenLayer(1);
    }
    return fallen;
}

RVA(0x0003fcb0, 0x2c)
i16 IsPartyMemberFallen(i16 index) {
    Character* character = GetPartyCharacter(index);
    if (!character) {
        return -1;
    }
    return GetFatalCondition(GetCharacterConditions(character)) != 0;
}

// Handles fallen and emptied members after a fight: a fallen non-human (not
// the guest) leaves the party; a member whose pool ran out gets its
// conditions, and leaves too unless human. Returns how many were handled.
// @identity-TODO: what CheckPickTarget does for them is only partly decoded.
RVA(0x0003fce0, 0x8e)
i16 ProcessPartyCasualties(void) {
    i16 count = 0;
    i16 i;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (!character) {
            continue;
        }
        if (GetFatalCondition(GetCharacterConditions(character))) {
            if (IsHumanCharacter(character) || IsGuestIndex(i) >= 0) {
                continue;
            }
            CheckPickTarget(i);
            SetPartySlot(i, -1);
        } else {
            if (!ApplyEmptyPools(character) || IsGuestIndex(i) >= 0) {
                continue;
            }
            CheckPickTarget(i);
            if (!IsHumanCharacter(character)) {
                SetPartySlot(i, -1);
            }
        }
        count++;
    }
    return count;
}

RVA(0x0003fd70, 0xbb)
i16 TickPartySteps(void) {
    i16 changed = 0;
    i16 index;
    Character* character;
    for (index = 0; index < 6; index++) {
        character = GetPartyCharacter(index);
        if (character) {
            ApplyEquipmentRegen(character);
            ApplyEquipmentEffects(character, EQUIP_EFFECT_STEP_TICK);
            if (character->id != 13) {
                if (HasCondition(GetCharacterConditions(character), CONDITION_SEVERE_POISON)
                    && !IsConditionResisted(character, 32)) {
                    changed |= 1;
                    DrainPool(&character->pools.hp, 4);
                }
                if (HasCondition(GetCharacterConditions(character), CONDITION_POISON)
                    && !IsConditionResisted(character, 15)) {
                    changed |= 1;
                    DrainPool(&character->pools.hp, 1);
                }
            }
        }
    }
    RedrawPartyStatus();
    return changed;
}

// @identity-TODO: an identity conversion every script-object lookup ends in
// (for the party table and the pointer slots alike).
RVA(0x0003fe30, 0x5)
Character* AsCharacter(Character* character) {
    return character;
}

RVA(0x0003fe40, 0x6)
Character* GetRosterLeader(void) {
    return RosterMemberAt(0);
}

RVA(0x0003fe50, 0x1e)
Character* GetRosterEntry(i16 slot) {
    if (slot >= 0 && slot < 32) {
        return RosterMemberAt(slot);
    }
    return NULL;
}

RVA(0x0003fe70, 0x17)
Character* GetRosterCharacter(i16 slot) {
    return AsCharacter(GetRosterEntry(slot));
}

RVA(0x0003fe90, 0x20)
i16 FindPartySlot(i16 slot) {
    i16 i;
    for (i = 0; i < 6; i++) {
        if (PartySlotAt(i) == slot) {
            return i;
        }
    }
    return -1;
}

RVA(0x0003feb0, 0x1a)
i16 GetRosterId(i16 slot) {
    Character* character = GetRosterCharacter(slot);
    if (character == NULL) {
        return -1;
    }
    return character->id;
}

RVA(0x0003fed0, 0x1e)
Character* SetRosterEntry(i16 slot, Character* character) {
    Character* prev = GetRosterEntry(slot);
    g_roster[slot] = character;
    return prev;
}

RVA(0x0003fef0, 0x21)
i16 GetPartySlot(i16 index) {
    if (index >= 0 && index < 6) {
        return PartySlotAt(index);
    }
    return -1;
}

RVA(0x0003ff20, 0x17)
Character* GetPartyEntry(i16 index) {
    return GetRosterEntry(GetPartySlot(index));
}

RVA(0x0003ff40, 0x17)
Character* GetPartyCharacter(i16 index) {
    return AsCharacter(GetPartyEntry(index));
}

// Puts roster slot `slot` (-1 empties it) into party position `index`; a
// member displaced from a non-guest position loses its leave conditions.
RVA(0x0003ff60, 0x6b)
void SetPartySlot(i16 index, i16 slot) {
    Character* character;
    if (index < 0 || index >= 6) {
        return;
    }
    if (slot < -1 || slot >= 32) {
        return;
    }
    if (PartySlotAt(index) != -1) {
        character = GetPartyCharacter(index);
        if (character != NULL && IsGuestIndex(index) < 0) {
            ClearLeaveConditions(GetCharacterConditions(character));
        }
    }
    g_party[index] = slot;
}

RVA(0x0003ffd0, 0x17)
i16 GetPartyRosterId(i16 index) {
    return GetRosterId(GetPartySlot(index));
}

// The roster slot holding character `id`; -1 when none does.
RVA(0x0003fff0, 0x2b)
i16 FindRosterSlotById(i16 id) {
    i16 slot;
    for (slot = 0; slot < 32; slot++) {
        if (GetRosterId(slot) == id) {
            return slot;
        }
    }
    return -1;
}

RVA(0x00040020, 0x18)
i16 RosterSlotOfId(i16 id) {
    if (id == -1) {
        return id;
    }
    return FindRosterSlotById(id);
}

RVA(0x00040040, 0x17)
Character* GetCharacterById(i16 id) {
    return GetRosterEntry(RosterSlotOfId(id));
}

// The party position of character `id`; -1 when not in the party.
RVA(0x00040060, 0x21)
i16 FindPartyPositionOfId(i16 id) {
    i16 slot = RosterSlotOfId(id);
    if (slot == -1) {
        return slot;
    }
    return FindPartySlot(slot);
}

// Sorts the roster by character id (empty slots last) and puts the party
// back on the same characters.
// Codegen constraint: the search for the insertion point leaves through
// `goto next`; as a for loop cl rotates it and the layout differs.
RVA(0x00040090, 0xcd)
b16 SortRoster(void) {
    i16 ids[6];
    i16 i;
    i16 j;
    i16 id;
    i16 other;
    for (i = 0; i < 6; i++) {
        ids[i] = GetPartyRosterId(i);
    }
    for (i = 0; i < 32; i++) {
        id = GetRosterId(i);
        while (id >= 0 && id < 32) {
            j = 0;
            while (1) {
                if (j >= i) {
                    goto next;
                }
                other = GetRosterId(j);
                if (other < 0 || other >= 32 || other > id) {
                    break;
                }
                j++;
            }
            SetRosterEntry(i, SetRosterEntry(j, GetRosterEntry(i)));
            i = j;
            id = GetRosterId(i);
        }
    next:;
    }
    for (i = 0; i < 6; i++) {
        if (ids[i] == -1) {
            SetPartySlot(i, -1);
        } else {
            SetPartySlot(i, FindRosterSlotById(ids[i]));
        }
    }
    return false;
}

// The id of the character at party position `index` (unchecked).
RVA(0x00040160, 0x17)
i16 GetPartyMemberId(i16 index) {
    return GetRosterId(PartySlotAt(index));
}

// Swaps two party slots; when the front three end up empty, the back three
// move forward.
RVA(0x00040180, 0x96)
void SwapPartySlots(i16 a, i16 b) {
    i16 slot;
    if (a != b) {
        slot = PartySlotAt(a);
        g_party[a] = PartySlotAt(b);
        g_party[b] = slot;
        if (PartySlotAt(0) == -1 && PartySlotAt(1) == -1 && PartySlotAt(2) == -1) {
            g_party[0] = PartySlotAt(3);
            g_party[1] = PartySlotAt(4);
            g_party[2] = PartySlotAt(5);
            g_party[3] = -1;
            g_party[4] = -1;
            g_party[5] = -1;
        }
    }
}

// Loads the equipment table: per character group, a 40-bit set of the items
// the group can equip.
RVA(0x00040220, 0x2a)
void LoadEquipTable(void) {
    FILE* fp = OpenDataFile(6, 12, 0);
    s_equipTable = ReadRawHandle(fp);
    CloseDataFile(fp);
}

// Nonzero when characters of `group` can equip item `item`.
RVA(0x00040250, 0x39)
b16 CanGroupEquip(i16 group, i16 item) {
    u8* table;
    if (item < 0) {
        return false;
    }
    table = HandleReadPtr(s_equipTable);
    return TestBit(table + group * 5, item) != 0;
}

RVA(0x00040290, 0x74)
i16 EquipPartOfItem(ItemRecord* item) {
    i16 part;
    i16 kind = item->kind;
    kind -= ITEM_KIND_WEAPON;
    switch (kind) {
        case 0:
            part = EQUIP_PART_WEAPON;
            break;
        case 1:
            part = EQUIP_PART_GUN;
            break;
        case 2:
            part = EQUIP_PART_AMMO;
            break;
        case 4:
            part = EQUIP_PART_HEAD;
            break;
        case 3:
        case 5:
            part = EQUIP_PART_BODY;
            break;
        case 6:
            part = EQUIP_PART_ARMS;
            break;
        case 7:
            part = EQUIP_PART_LEGS;
            break;
        case 8:
            part = EQUIP_PART_ACCESSORY;
            break;
    }
    return part;
}

RVA(0x00040310, 0x90)
ItemSlot GetEquipSlot(Character* character, i16 part) {
    ItemSlot none = {-1, -1, 0};
    if (character != NULL) {
        switch (part) {
            case EQUIP_PART_WEAPON:
                return GetCharacterEquipment(character)[5];
            case EQUIP_PART_GUN:
                return GetCharacterEquipment(character)[6];
            case EQUIP_PART_AMMO:
                return GetCharacterEquipment(character)[7];
            case EQUIP_PART_HEAD:
                return GetCharacterEquipment(character)[0];
            case EQUIP_PART_BODY:
                return GetCharacterEquipment(character)[1];
            case EQUIP_PART_ARMS:
                return GetCharacterEquipment(character)[2];
            case EQUIP_PART_LEGS:
                return GetCharacterEquipment(character)[3];
            case EQUIP_PART_ACCESSORY:
                return GetCharacterEquipment(character)[4];
        }
    }
    return none;
}

RVA(0x000403a0, 0x1c)
ItemSlot GetRosterEquipSlot(i16 slot, i16 part) {
    return GetEquipSlot(GetRosterCharacter(slot), part);
}

RVA(0x000403c0, 0x1b)
i16 GetEquipItem(Character* character, i16 part) {
    return GetEquipSlot(character, part).item;
}

RVA(0x000403e0, 0x110)
i16 SetEquipSlot(i16 slot, i16 part, ItemSlot item, i16 check) {
    Character* character = GetRosterCharacter(slot);
    if (!character) {
        return -1;
    }
    switch (part) {
        case EQUIP_PART_WEAPON:
            GetCharacterEquipment(character)[5] = item;
            break;
        case EQUIP_PART_GUN:
            GetCharacterEquipment(character)[6] = item;
            if (check && CanEquipItem(character, GetCharacterEquipment(character)[7].item) < 1) {
                UnequipPart(slot, EQUIP_PART_AMMO);
            }
            break;
        case EQUIP_PART_AMMO:
            GetCharacterEquipment(character)[7] = item;
            break;
        case EQUIP_PART_HEAD:
            GetCharacterEquipment(character)[0] = item;
            break;
        case EQUIP_PART_BODY:
            GetCharacterEquipment(character)[1] = item;
            break;
        case EQUIP_PART_ARMS:
            GetCharacterEquipment(character)[2] = item;
            break;
        case EQUIP_PART_LEGS:
            GetCharacterEquipment(character)[3] = item;
            break;
        case EQUIP_PART_ACCESSORY:
            GetCharacterEquipment(character)[4] = item;
            break;
        default:
            part = -1;
    }
    return part;
}

RVA(0x000404f0, 0x93)
void UnequipPart(i16 slot, i16 part) {
    ItemSlot empty;
    ItemSlot item;
    ClearItemSlot(&empty);
    item = GetRosterEquipSlot(slot, part);
    SetEquipSlot(slot, part, empty, 1);
    if (item.item != -1 && item.item != 0) {
        if (item.quantity < 1) {
            item.quantity = 1;
        }
        StoreBagItem(item.item, item.quantity, item.attachment);
    }
}

RVA(0x00040590, 0x4d)
i16 AttachEquipItem(i16 member, i16 part, i16 index) {
    ItemSlot item = GetRosterEquipSlot(member, part);
    i16 previous = item.attachment;
    item.attachment = index;
    SetEquipSlot(member, part, item, 0);
    return previous;
}

// The ammunition type of the gun in equipment slot 6 (item parameter 0x27);
// -1 without a character or a gun.
// @identity-TODO: that slot 6 holds the gun is inferred from this use.
RVA(0x000405e0, 0x36)
i16 GetGunAmmoType(Character* character) {
    if (!character) {
        return -1;
    }
    if (GetCharacterEquipment(character)[6].item < 1) {
        return -1;
    }
    return GetItemAmmoType(GetLoadedRecord(GetCharacterEquipment(character)[6].item));
}

// The equipment part item `item` goes in when `character` can equip it, else
// -1. Ammunition (kind 13) must match the equipped gun; anything else must be
// allowed for the character's equipment group.
RVA(0x00040620, 0x82)
i16 CanEquipItem(Character* character, i16 item) {
    ItemRecord* record;
    i16 part;
    if (item < 1) {
        return -1;
    }
    record = GetLoadedRecord(item);
    part = EquipPartOfItem(record);
    if (part < 0) {
        return -1;
    }
    if (record->kind != ITEM_KIND_AMMO) {
        if (!CanGroupEquip(character->equipGroup, GetItemEquipCode(record))) {
            return -1;
        }
    } else {
        i16 ammo = GetItemAmmoType(record);
        if (GetGunAmmoType(character) != ammo) {
            return -1;
        }
    }
    return part;
}

// Puts `item` on its equipment part of roster member `slot` (SetEquipSlot's
// result in `*result`) and returns the item slot it replaces.
RVA(0x000406b0, 0x52)
ItemSlot SwapEquipSlot(i16 slot, ItemSlot item, i16* result) {
    i16 part = EquipPartOfItem(GetLoadedRecord(item.item));
    ItemSlot old = GetRosterEquipSlot(slot, part);
    *result = SetEquipSlot(slot, part, item, 1);
    return old;
}

// Equips `item` from bag entry `index` on roster member `slot`, taking it out
// of the bag and putting the replaced item back; returns the replaced slot.
// Ammunition (kind 13) records `count` for the character; kind 14 clears
// parts 3, 5 and 6.
RVA(0x00040710, 0x11a)
ItemSlot EquipItem(i16 slot, ItemSlot item, i16 count, i16 index) {
    i16 result;
    ItemSlot old = SwapEquipSlot(slot, item, &result);
    if (result != -1) {
        Character* character = GetRosterCharacter(slot);
        if (item.item != -1) {
            i16 kind = GetItemKind(item.item);
            if (kind != 13) {
                TakeBagItemsAt(index, item.item, item.quantity);
                if (kind == ITEM_KIND_FULL_BODY_ARMOR) {
                    UnequipPart(slot, EQUIP_PART_HEAD);
                    UnequipPart(slot, EQUIP_PART_ARMS);
                    UnequipPart(slot, EQUIP_PART_LEGS);
                }
            } else {
                TakeBagItems(item.item, item.quantity);
                if (character && g_ammoCountIndex >= 0) {
                    i16 at = g_ammoCountIndex;
                    character->ammoCounts[at] = (u8)count;
                }
            }
        }
        if (old.item != -1) {
            if (old.quantity < 1) {
                old.quantity = 1;
            }
            StoreBagItem(old.item, old.quantity, old.attachment);
        }
        CompactBag();
    }
    return old;
}

// Normalises all eight equipment slots.
RVA(0x00040830, 0x7f)
void NormalizeEquipSlots(Character* character) {
    NormalizeItemSlot(&GetCharacterEquipment(character)[0]);
    NormalizeItemSlot(&GetCharacterEquipment(character)[1]);
    NormalizeItemSlot(&GetCharacterEquipment(character)[2]);
    NormalizeItemSlot(&GetCharacterEquipment(character)[3]);
    NormalizeItemSlot(&GetCharacterEquipment(character)[4]);
    NormalizeItemSlot(&GetCharacterEquipment(character)[5]);
    NormalizeItemSlot(&GetCharacterEquipment(character)[6]);
    NormalizeItemSlot(&GetCharacterEquipment(character)[7]);
}

// Empties a slot without an item; a non-ammunition item gets quantity 1.
RVA(0x000408b0, 0x39)
void NormalizeItemSlot(ItemSlot* slot) {
    if (slot->item < 1) {
        ClearItemSlot(slot);
        return;
    }
    if (GetItemKind(slot->item) != ITEM_KIND_AMMO) {
        slot->quantity = 1;
    }
}

// Adds `amount` to the character's macca, kept in 0..9999999; returns the new
// total (0 without a character).
RVA(0x000408f0, 0x2a)
i32 AddMacca(Character* character, i32 amount) {
    if (!character) {
        return 0;
    }
    return character->macca = AddClampInt(character->macca, amount, 0, 9999999);
}

// The same for magnetite.
RVA(0x00040920, 0x2a)
i32 AddMagnetite(Character* character, i32 amount) {
    if (!character) {
        return 0;
    }
    return character->magnetite = AddClampInt(character->magnetite, amount, 0, 9999999);
}

// -1 when script object `who` has less macca than `amount`, 0 when exactly
// that much, else 1.
RVA(0x00040950, 0x28)
i16 CompareMacca(i16 who, i32 amount) {
    i32 macca = ResolveScriptObject(who)->macca;
    if (macca < amount) {
        return -1;
    }
    return macca != amount;
}
