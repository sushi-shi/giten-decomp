// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <File/DataFileKind.h>
#include <File/DataTableId.h>
#include <Game/Character.h>
#include <Game/Field.h>
#include <Game/GameState.h>
#include <Game/Skill.h>
#include <Game/SkillId.h>
#include <Game/SkillList.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Script/Script.h>
#include <Script/ScriptState.h>
#include <Script/TextState.h>
#include <Util/Range.h>
#include <Util/WordList.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// The id of the record held in the one-entry record cache (-1 = none).
DATA(0x00069108)
i16 g_cachedRecordId = -1;

DATA(0x00080d38)
static char s_skillViewName[0x100] = {0};

DATA(0x00080e38)
static SkillHeader s_cachedSkill = {0};

DATA(0x00080e50)
static i16 s_wordScratch[64] = {0};

DATA(0x00080ed0)
static SkillView s_skillView = {0};

DATA(0x00080ef0)
static char s_skillViewDescription[0x100] = {0};

// The stacked script states, newest first.
DATA(0x00080ff0)
static SavedScriptState* s_savedScripts = NULL;

// The handle of the loaded skill file.
DATA(0x00080ff4)
static i32 s_skillTable = 0;

// One byte per map area; bit 0 allows Traesto, Traport and Trafuri there.
DATA(0x00080ff8)
static u8* s_areaSkillFlags = NULL;

DATA(0x00080ffc)
static i16 s_skillCount = 0;

DATA(0x00081000)
static i16 s_valueA = 0;

DATA(0x00081004)
static i16 s_valueB = 0;

DATA(0x00081008)
i16 g_recordBaseValue = 0;

DATA(0x0008100c)
i16 g_recordValue = 0;

// How often the cached skill has been used.
DATA(0x00081010)
static i16 s_skillUses = 0;

static __inline void AllocWordArray(i16** words, i16 count) {
    if (count < 1) {
        *words = NULL;
        return;
    }
    *words = AllocCleared(count, sizeof(i16));
}

RVA(0x0002dbf0, 0x30)
void InitWordList(WordList* list, i16 count) {
    list->count = count;
    AllocWordArray(&list->words, count);
}

RVA(0x0002dc20, 0x20)
void FreeWordList(WordList* list) {
    if (list->words != NULL) {
        list->words = FreeBlock(list->words);
    }
    list->count = 0;
}

RVA(0x0002dc40, 0x20)
void ResetWordList(WordList* list, i16 count) {
    FreeWordList(list);
    InitWordList(list, count);
}

RVA(0x0002dc60, 0x3b)
i16* CopyWordArray(i16* words, i16 count) {
    i16* copy;
    i16 i;
    AllocWordArray(&copy, count);
    for (i = 0; i < count; i++) {
        copy[i] = words[i];
    }
    return copy;
}

RVA(0x0002dca0, 0x32)
i16 FindWord(WordList* list, i16 word) {
    i16 i;
    if (list) {
        for (i = 0; i < list->count; i++) {
            if (GetWord(list, i) == word) {
                return i;
            }
        }
    }
    return WORD_NONE;
}

RVA(0x0002dce0, 0x99)
i16 AddSkill(WordList* list, i16 skill) {
    i16 i;
    if (!list) {
        return WORD_NONE;
    }
    if (ContainsWord(list, skill)) {
        return WORD_NONE;
    }
    for (i = 0; i < list->count; i++) {
        s_wordScratch[i] = GetWord(list, i);
    }
    list->count++;
    ResetWordList(list, list->count);
    for (i = 0; i < list->count - 1; i++) {
        SetWord(list, i, s_wordScratch[i]);
    }
    SetWord(list, list->count - 1, skill);
    return list->count;
}

RVA(0x0002dd80, 0x82)
void MoveWord(WordList* list, i16 from, i16 to) {
    i16 value = list->words[from];
    i16 i;
    if (to == WORD_LAST) {
        to = list->count - 1;
    }
    if (from <= to) {
        for (i = from; i < to; i++) {
            SetWord(list, i, list->words[i + 1]);
        }
    } else {
        for (i = from; i > to; i--) {
            SetWord(list, i, list->words[i - 1]);
        }
    }
    SetWord(list, i, value);
}

RVA(0x0002de10, 0x90)
i16 RemoveWord(WordList* list, i16 word) {
    i16 index;
    i16 i;
    if (!list) {
        return WORD_NONE;
    }
    index = FindWord(list, word);
    if (index < 0) {
        return WORD_NONE;
    }
    MoveWord(list, index, WORD_LAST);
    list->count--;
    for (i = 0; i < list->count; i++) {
        s_wordScratch[i] = GetWord(list, i);
    }
    ResetWordList(list, list->count);
    for (i = 0; i < list->count; i++) {
        SetWord(list, i, s_wordScratch[i]);
    }
    return list->count;
}

RVA(0x0002dea0, 0x32)
void StripZeroWords(WordList* list) {
    while (ContainsWord(list, 0)) {
        RemoveWord(list, 0);
    }
}

RVA(0x0002dee0, 0x74)
i16 KeepFirstSixWords(WordList* list) {
    i16 words[6];
    i16 i;
    if (!list) {
        return -1;
    }
    if (list->count <= 6) {
        return -1;
    }
    for (i = 0; i < 6; i++) {
        words[i] = GetWord(list, i);
    }
    ResetWordList(list, 6);
    for (i = 0; i < 6; i++) {
        SetWord(list, i, words[i]);
    }
    StripZeroWords(list);
    return list->count;
}

RVA(0x0002df60, 0x4f)
void CopySkillList(Character* from, WordList* to) {
    i16 i;
    if (from) {
        ResetWordList(to, GetWordCount(GetCharacterSkills(from)));
        for (i = 0; i < GetWordCount(GetCharacterSkills(from)); i++) {
            SetWord(to, i, GetWord(GetCharacterSkills(from), i));
        }
    }
}

RVA(0x0002dfb0, 0x42)
b16 RollSkillLearning(Character* character, i16 skill) {
    i16 chance = character->level;
    chance = (chance - GetSkillLevel(skill)) * 10 + GetStatTotal(character, STAT_INTELLIGENCE);
    return chance > RandomAverage(10, 60, 2);
}

RVA(0x0002e000, 0x108)
i16 LearnLevelSkill(Character* character) {
    i16 skill = -1;
    i32 handle = BuildLearnableSkillRanks(character);
    WordList* skills;
    SkillRankList* ranks;
    i16 count;
    i16 i;
    if (!handle) {
        return 0;
    }
    skills = GetCharacterSkills(character);
    if (IsSkillListFull(skills)) {
        skill = HighestAffiliatedSkillLevel(character);
    }
    ranks = HandleReadPtr(handle);
    count = ranks->count;
    for (i = 0; i < count; i++) {
        if (GetSkillRank(ranks, i)->level >= skill) {
            break;
        }
    }
    if (i < count) {
        i16 index;
        for (index = i;; index++) {
            if (index >= count) {
                skill = 0;
                break;
            }
            ranks = HandleReadPtr(handle);
            skill = GetSkillRank(ranks, index)->skill;
            if (!ContainsWord(skills, skill) && RollSkillLearning(character, skill)) {
                if (IsSkillListFull(skills)) {
                    RemoveLowestAffiliatedSkill(character);
                }
                AddSkill(skills, skill);
                break;
            }
        }
    } else {
        skill = 0;
    }
    FreeHandle(handle);
    return skill;
}

RVA(0x0002e110, 0x82)
i16 RemoveLowestAffiliatedSkill(Character* character) {
    i16 lowestLevel = 1000;
    i16 lowestSkill = -1;
    WordList* list = GetCharacterSkills(character);
    i16 i;
    for (i = 0; i < GetWordCount(GetCharacterSkills(character)); i++) {
        i16 skill = GetWord(GetCharacterSkills(character), i);
        if (FindSkillAffiliation(character, skill) >= 0) {
            i16 level = GetSkillLevel(skill);
            if (level < lowestLevel) {
                lowestSkill = skill;
                lowestLevel = level;
            }
        }
    }
    if (lowestSkill >= 0) {
        RemoveWord(list, lowestSkill);
    }
    return lowestSkill;
}

// Which of the character's three affiliations (0..2) matches the family of
// skill `skill`; -1 when none does.
RVA(0x0002e1a0, 0x37)
i16 FindSkillAffiliation(Character* character, i16 skill) {
    i16 family = GetSkillFamily(skill);
    i16 i;
    for (i = 0; i < 3; i++) {
        if (GetCharacterAffiliation(character, i)
            && family == GetCharacterAffiliation(character, i)) {
            return i;
        }
    }
    return -1;
}

RVA(0x0002e1e0, 0x54)
i16 HighestAffiliatedSkillLevel(Character* character) {
    i16 best = -1;
    i16 i;
    for (i = 0; i < GetWordCount(GetCharacterSkills(character)); i++) {
        i16 skill = GetWord(GetCharacterSkills(character), i);
        if (FindSkillAffiliation(character, skill) >= 0) {
            i16 level = GetSkillLevel(skill);
            if (level > best) {
                best = level;
            }
        }
    }
    return best;
}

RVA(0x0002e240, 0x13b)
i32 BuildLearnableSkillRanks(Character* character) {
    i16 count = 0;
    i16 skill;
    i32 handle;
    SkillRankList* ranks;
    for (skill = 1; skill > 0; skill++) {
        if (!ContainsWord(GetCharacterSkills(character), skill)) {
            skill = FindSkill(
                skill,
                GetCharacterAffiliation(character, 0),
                GetCharacterAffiliation(character, 1),
                GetCharacterAffiliation(character, 2),
                character->level
            );
            if (skill != -1) {
                count++;
            }
        }
    }
    if (count == 0) {
        return 0;
    }
    handle = AllocHandle(offsetof(SkillRankList, entries) + count * sizeof(SkillRank));
    ranks = HandleWritePtr(handle);
    ranks->count = count;
    count = 0;
    for (skill = 1; skill > 0; skill++) {
        if (!ContainsWord(GetCharacterSkills(character), skill)) {
            i16 level;
            skill = FindSkill(
                skill,
                GetCharacterAffiliation(character, 0),
                GetCharacterAffiliation(character, 1),
                GetCharacterAffiliation(character, 2),
                character->level
            );
            if (skill == -1) {
                break;
            }
            level = GetSkillLevel(skill);
            ranks = HandleWritePtr(handle);
            GetSkillRank(ranks, count)->skill = skill;
            GetSkillRank(ranks, count)->level = level;
            count++;
        }
    }
    if (count == 0) {
        FreeHandle(handle);
        return 0;
    }
    ranks = HandleWritePtr(handle);
    ranks->count = count;
    qsort(GetSkillRank(ranks, 0), count, sizeof(SkillRank), CompareSkillRanks);
    return handle;
}

RVA(0x0002e380, 0x13)
int CompareSkillRanks(const void* left, const void* right) {
    const SkillRank* b = right;
    const SkillRank* a = left;
    return a->level - b->level;
}

// Loads the skill file and the per-area skill flags.
RVA(0x0002e3a0, 0x69)
void LoadSkillFiles(void) {
    SkillTable* table;
    FILE* fp = OpenDataFile(DATA_TABLE_SKILLS, DATA_FILE_TABLE, 0);
    s_skillTable = ReadCryptHandle(fp);
    CloseDataFile(fp);
    table = HandleReadPtr(s_skillTable);
    s_skillCount = table->count;
    fp = OpenDataFile(DATA_TABLE_AREA_SKILL_FLAGS, DATA_FILE_TABLE, 0);
    s_areaSkillFlags = ReadRawAlloc(fp);
    CloseDataFile(fp);
}

RVA(0x0002e410, 0x11)
char* GetSkillName(i16 id) {
    return GetSkillRecordText(GetSkill(id));
}

RVA(0x0002e430, 0x43)
SkillHeader* GetSkill(i16 id) {
    SkillTable* table = HandleReadPtr(s_skillTable);
    if (id < 0 || id >= table->count) {
        table = HandleReadPtr(s_skillTable);
        id = SKILL_AGI;
    }
    return OffsetBy(table, table->offsets[id]);
}

RVA(0x0002e480, 0x25)
char* GetSkillDescription(i16 id) {
    char* name = GetSkillRecordText(GetSkill(id));
    name += strlen(name) + 1;
    return name;
}

// The cached copy when `id` is the cached skill.
RVA(0x0002e4b0, 0x24)
SkillHeader* GetCachedSkill(i16 id) {
    if (g_cachedRecordId >= 0 && g_cachedRecordId == id) {
        return &s_cachedSkill;
    }
    return GetSkill(id);
}

RVA(0x0002e4e0, 0x1d)
SkillMessage* GetSkillMessage(i16 id, i16 after) {
    SkillHeader* skill = GetCachedSkill(id);
    if (!after) {
        return &skill->parameters.beforeMessage;
    }
    return &skill->afterMessage;
}

RVA(0x0002e500, 0x13)
i16 GetSkillCost(i16 id) {
    return GetSkillParameterCost(&GetCachedSkill(id)->parameters);
}

RVA(0x0002e520, 0x12)
i16 GetSkillCount(void) {
    SkillTable* table = HandleReadPtr(s_skillTable);
    return table->count;
}

// Copies a skill's header into `dst` (a new block when NULL).
RVA(0x0002e540, 0x37)
SkillHeader* CopySkillHeader(i16 id, SkillHeader* dst) {
    if (!dst) {
        dst = AllocCleared(1, sizeof(SkillHeader));
    }
    *dst = *GetCachedSkill(id);
    return dst;
}

// Traesto, Traport and Trafuri work only where the area allows them (and never while the
// field marker is set): 1 when usable here, -1 when the area forbids it.
RVA(0x0002e580, 0x42)
i16 CheckSkillArea(i16 id) {
    if (id != SKILL_TRAESTO && id != SKILL_TRAPORT && id != SKILL_TRAFURI) {
        return 1;
    }
    if (GetFieldMarker()) {
        return 0;
    }
    return (s_areaSkillFlags[g_party.field.pos.area] & 1) * 2 - 1;
}

// A record's signed cost byte: negative costs HP (which must not reach 0),
// positive costs MP; -128 and 127 spend the whole pool. Returns what would be left
// (negative: not affordable), or -1 with the pool already empty.
RVA(0x0002e5d0, 0x51)
i16 HpMpLeftAfterCost(i16 cost, Character* character) {
    i16 pool;
    if (cost < 0) {
        cost = -cost;
        pool = character->pools.hp.cur;
        if (pool == 0) {
            return -1;
        }
        if (cost == SKILL_COST_WHOLE_HP) {
            return 0;
        }
        return pool - cost - 1;
    }
    pool = character->pools.mp.cur;
    if (pool == 0) {
        return -1;
    }
    if (cost == SKILL_COST_WHOLE_MP) {
        return 0;
    }
    return pool - cost;
}

// -1 when the skill does not work in this mode, 0 when `character` cannot pay
// its cost, else CheckSkillArea's verdict.
RVA(0x0002e630, 0x53)
i16 CanUseSkill(i16 id, Character* character) {
    if (IsSkillUsableNow(GetSkillUseModes(GetCachedSkill(id))) < 0) {
        return -1;
    }
    if (HpMpLeftAfterCost(GetSkillCost(id), character) < 0) {
        return 0;
    }
    return CheckSkillArea(id);
}

// The skill `id` as a view whose name and description are copies.
RVA(0x0002e690, 0x8f)
SkillView* GetSkillView(i16 id) {
    // The copy spans the text-pointer slots; both are overwritten below.
    memcpy(&s_skillView, GetCachedSkill(id), sizeof(s_skillView));
    s_skillView.name = strcpy(s_skillViewName, GetSkillName(id));
    s_skillView.description = strcpy(s_skillViewDescription, GetSkillDescription(id));
    return &s_skillView;
}

RVA(0x0002e720, 0x13)
GZ_ENUM_RETURN(SkillKind, i32) GetSkillKind(i16 id) {
    return GetCachedSkill(id)->parameters.kind;
}

RVA(0x0002e740, 0x18)
GZ_ENUM_RETURN(AttackMode, u16) GetSkillMode(i16 id) {
    return GetCachedSkill(id)->parameters.mode;
}

RVA(0x0002e760, 0x13)
u16 GetSkillFamily(i16 id) {
    return GetCachedSkill(id)->parameters.family;
}

RVA(0x0002e780, 0x13)
u16 GetSkillLevel(i16 id) {
    return GetCachedSkill(id)->parameters.level;
}

// The first skill from `start` on of family `a`, `b` or `c` (0 never matches)
// whose level is at most `maxLevel`; -1 when none is.
RVA(0x0002e7a0, 0x73)
i16 FindSkill(i16 start, u16 a, u16 b, u16 c, i16 maxLevel) {
    i16 id;
    SkillHeader* skill;
    u16 family;
    for (id = start; id < s_skillCount; id++) {
        skill = GetCachedSkill(id);
        if (skill->parameters.family) {
            family = skill->parameters.family;
            if ((family == a || family == b || family == c)
                && skill->parameters.level <= maxLevel) {
                return id;
            }
        }
    }
    return -1;
}

RVA(0x0002e820, 0x16)
void ResetRecordCache(void) {
    g_cachedRecordId = -1;
    g_recordValue = g_recordBaseValue;
}

// Caches skill `id` with `value` as the record value.
RVA(0x0002e840, 0x58)
void CacheSkill(i16 id, i16 value) {
    ResetRecordCache();
    CopySkillHeader(id, &s_cachedSkill);
    s_valueA = GetSkillValueA(&s_cachedSkill);
    s_valueB = GetSkillValueB(&s_cachedSkill);
    g_cachedRecordId = id;
    g_recordBaseValue = value;
    g_recordValue = value;
    s_skillUses = 1;
}

static __inline i16 RollCachedSkillWearPercent(void) {
    i16 wear = s_skillUses * s_cachedSkill.parameters.wear;
    i16 percent = RandomPercent(wear, -20, 20);
    return min(percent, 100);
}

// @early-stop: retail keeps each discarded worn quotient's DWORD store to one
// stack slot; cl /Ox deletes them with their arithmetic for every non-volatile
// form tried (i32/i16 scalar, array, struct, union, address-taken local, inline
// out-parameter or returning helper, optimize("g"/"a"/"w"/"y", off) probes).
RVA(0x0002e8a0, 0xad)
void WearCachedSkill(void) {
    i16 percent;
    i16 wear;
    i32 worn;
    if (g_cachedRecordId == -1) {
        return;
    }
    percent = RollCachedSkillWearPercent();
    wear = (100 - percent) * s_valueA;
    worn = wear / 100;
    wear = (100 - percent) * s_valueB;
    worn = wear / 100;
    wear = (100 - percent) * g_recordBaseValue;
    worn = wear / 100;
    s_skillUses++;
}

RVA(0x0002e950, 0x7)
i16 GetRecordValue(void) {
    return g_recordValue;
}

// `value` less the wear of the cached skill's uses, kept in 1..30000.
RVA(0x0002e960, 0x66)
i32 WearSkillValue(i16 value) {
    i16 percent = RollCachedSkillWearPercent();
    i32 result;
    result = (100 - percent) * value / 100;
    if (result < 0) {
        return 1;
    }
    if (result > 30000) {
        return 30000;
    }
    return result;
}

// Stacks the current script and text state and leaves no script current.
RVA(0x0002e9d0, 0x53)
void SaveScriptState(void) {
    SavedScriptState* saved = AllocCleared(1, sizeof(SavedScriptState));
    saved->script = g_curScript;
    saved->textState = g_textState;
    saved->next = s_savedScripts;
    s_savedScripts = saved;
    g_curScript = NULL;
}

// Returns to the newest stacked script and text state.
RVA(0x0002ea30, 0x46)
void RestoreScriptState(void) {
    SavedScriptState* saved = s_savedScripts;
    s_savedScripts = saved->next;
    g_curScript = saved->script;
    g_textState = saved->textState;
    FreeBlock(saved);
}
