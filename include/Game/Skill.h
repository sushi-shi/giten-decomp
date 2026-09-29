#ifndef GITEN_GAME_SKILL_H
#define GITEN_GAME_SKILL_H

#include <rva.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Ints.h>

#include <Game/SkillMessage.h>

// clang-format off
GZ_ENUM_BEGIN(SkillUseModes)
    SKILL_USE_FIELD = 1,
    SKILL_USE_WORLD_MAP = 2,
    SKILL_USE_FIELD_BATTLE = 4
GZ_ENUM_END(SkillUseModes);
// clang-format on

// Parameters shared by the file header and the menu's skill view.
typedef struct SkillParameters {
    // @identity-TODO: the low six bits and the top two of the first byte are
    // read by separate accessors, and the combat resolver compares the whole
    // byte (`type`); roles unrecovered.
    union {
        struct {
            u8 kind : 6;
            u8 mode : 2;
        };
        u8 type;
    };
    u8 family; // @identity-TODO: FindSkill matches it against up to three codes
    u8 level;  // FindSkill takes skills up to a given level
    i8 cost;   // negative: HP, positive: MP (see HpMpLeftAfterCost)
    GZ_ENUM_STORAGE(SkillUseModes, u8) usable;
    u8 targetArea; // @identity-TODO: CollectTargets' area code
    // @identity-TODO: target-picker combinations 0x10/0x11/0x30 remain unnamed.
    u8 targetFlags;
    u8 targetCounts; // Low nibble: hits; high nibble: target count or selection mode.
    u8 attackRange;  // Packed minimum/maximum distance; also selects the target-picker range icon.
    u8 valueA;       // @identity-TODO: the two values a use of the skill wears down
    u8 valueB;
    u8 wear; // percent lost per use (randomized by 20 either way)
    u8 attackAttribute;
    u8 inflictedCondition;
    u8 effectCode; // Effect selector interpreted according to the skill kind.
    u8 effect;     // the shot LaunchShot flies for it
    SkillMessage beforeMessage;
} SkillParameters;

// The cost byte's extremes spend the whole pool; an HP cost is negated first.
#define SKILL_COST_WHOLE_MP 0x7f
#define SKILL_COST_WHOLE_HP 0x80

#define GetSkillParameterCost(parameters) ((parameters)->cost)

#define SkillCostsFullPool(parameters)                                                             \
    (GetSkillParameterCost(parameters) == -SKILL_COST_WHOLE_HP                                     \
     || GetSkillParameterCost(parameters) == SKILL_COST_WHOLE_MP)

// A file record's name and description follow its twenty-byte header.
typedef struct SkillHeader {
    SkillParameters parameters;
    SkillMessage afterMessage;
} SkillHeader;

static __inline void* GetSkillRecordText(SkillHeader* record) {
    return record + 1;
}

// The menu copies the shared parameters and supplies decoded text pointers.
typedef struct SkillView {
    SkillParameters parameters;
    char* name;
    char* description;
} SkillView;

// The loaded skill file: a record count, then each record's byte offset.
typedef struct SkillTable {
    i16 count;
    u16 offsets[1];
} SkillTable;

SkillHeader* GetSkill(i16 id);
char* GetSkillName(i16 id);
char* GetSkillDescription(i16 id);
SkillHeader* GetCachedSkill(i16 id);

static __inline u8 GetSkillTargetFlags(i16 id) {
    return GetCachedSkill(id)->parameters.targetFlags;
}

#define GetSkillShotId(id) (GetCachedSkill(id)->parameters.effect)

#define GetSkillValueA(record) ((record)->parameters.valueA)

#define GetSkillValueB(record) ((record)->parameters.valueB)

static __inline u8 GetSkillAttackAttribute(SkillHeader* record) {
    return record->parameters.attackAttribute;
}

static __inline i16 GetSkillEffectCode(SkillHeader* record) {
    return record->parameters.effectCode;
}

static __inline u8 GetSkillInflictedCondition(SkillHeader* record) {
    return record->parameters.inflictedCondition;
}

#define GetSkillUseModes(record) ((record)->parameters.usable)

#define GetSkillAttackRange(id) (GetCachedSkill(id)->parameters.attackRange)

SkillMessage* GetSkillMessage(i16 id, i16 after);
i16 GetSkillCost(i16 id);
i16 GetSkillCount(void);
SkillHeader* CopySkillHeader(i16 id, SkillHeader* dst);
void CacheSkill(i16 id, i16 value);
i32 WearSkillValue(i16 value);
void ResetRecordCache(void);
i16 GetRecordValue(void);
i16 HpMpLeftAfterCost(i16 cost, struct Character* character);
SkillView* GetSkillView(i16 id);
void LoadSkillFiles(void);
i16 CheckSkillArea(i16 id);
i16 CanUseSkill(i16 id, struct Character* character);
i32 GetSkillKind(i16 id);
u16 GetSkillMode(i16 id);
u16 GetSkillFamily(i16 id);
u16 GetSkillLevel(i16 id);
i16 FindSkill(i16 start, u16 a, u16 b, u16 c, i16 maxLevel);

// Applies the effect of skill `skill` used by `user` on `target`.
void ApplySkillEffect(i16 skill, struct Character* user, struct Character* target);

// Evaluates wear for the cached skill and advances its use count.
void WearCachedSkill(void);

// 1 when a skill with these `usable` bits works in the current mode, else -1.
i16 IsSkillUsableNow(GZ_ENUM_PARAM(SkillUseModes, u16) usable);

#endif // GITEN_GAME_SKILL_H
