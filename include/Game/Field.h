#ifndef GITEN_GAME_FIELD_H
#define GITEN_GAME_FIELD_H

#include <rva.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Ints.h>

struct Character;

// Set during field battles; gates party turns, rewards and the FIGHT banner.
extern i16 g_fieldBattleActive;

// @identity-TODO: the field-map state entered through game state 0xb. Two
// sides (selected by the sign of an operand) each carry a countdown and a
// percentage rate; their game meaning (e.g. encounters) is not recovered.
void MarkFieldRefresh(void);
i16 ExchangeFieldOption(i16 option);
i16 SetFieldParams(i16 first, i16 second, i16 third);
// The entry route of a field map: none active, one entered from a cell event,
// or one entered by a script command.
GZ_ENUM_BEGIN_SPLIT(FieldMapMode, i16)
    FIELD_MAP_INACTIVE = -1,
    FIELD_MAP_CELL_EVENT = 0,
    FIELD_MAP_SCRIPT_EVENT = 1
GZ_ENUM_END_SPLIT(FieldMapMode)

b16 IsFieldModeAtLeast(b16 anyMode);
i16 GetFieldMarker(void);
void SetFieldPair(i16 first, i16 second);
void EnterFieldMap(
    i16 map,
    i16 countA,
    i16 rateA,
    i16 countB,
    i16 rateB,
    GZ_ENUM_PARAM(FieldMapMode, i16) mode
);
i16 GetFieldMap(void);
void SetFieldCounts(i16 countA, i16 countB);
i16 TickFieldCount(i16 side, b16 hold);
i32 ScaleByFieldRate(i16 first, i16 second, i32 value);
// The phases of a field encounter (RunFieldEncounter, RunFieldState): enter
// it, run the turns, back out, grant the rewards, show the level-ups and the
// analyze window, and tear it down.
GZ_ENUM_BEGIN(FieldEncounterPhase)
    FIELD_ENCOUNTER_PHASE_ENTER = 0,
    FIELD_ENCOUNTER_PHASE_TURNS = 1,
    FIELD_ENCOUNTER_PHASE_BACK_OUT = 2,
    FIELD_ENCOUNTER_PHASE_REWARDS = 3,
    FIELD_ENCOUNTER_PHASE_LEVEL_UPS = 4,
    FIELD_ENCOUNTER_PHASE_ANALYZE = 5,
    FIELD_ENCOUNTER_PHASE_TEAR_DOWN = 6
GZ_ENUM_END(FieldEncounterPhase)

// The entry phase's two steps: initialize the scene, then start its turns.
GZ_ENUM_BEGIN_SPLIT(FieldEncounterEntryStep, i16)
    FIELD_ENCOUNTER_STEP_SETUP = 0,
    FIELD_ENCOUNTER_STEP_START = 1
GZ_ENUM_END_SPLIT(FieldEncounterEntryStep)

b16 RunFieldEncounter(void);
// How a field map ended (LeaveFieldMap): won (the encounter's enemies are
// beaten), ended (time ran out or no objects remain), or lost (no party member
// can fight).
GZ_ENUM_BEGIN(FieldMapOutcome)
    FIELD_MAP_LOST = -1,
    FIELD_MAP_ENDED = 0,
    FIELD_MAP_WON = 1
GZ_ENUM_END(FieldMapOutcome)

GZ_ENUM_RETURN(FieldMapOutcome, i16) GetFieldEntryState(void);
void LeaveFieldMap(GZ_ENUM_PARAM(FieldMapOutcome, i16) result);

void ResetRosterFieldMarks(void);

b16 RunFieldState(void);

// @identity-TODO: +0x17e as the ready-to-act flag and +0x17f as the action wait are inferred
// from 0x3f5f0 (first member with +0x17e set) and OpSetActorAlert's +0x17f clamps; name those
// Character fields to settle it.
RVA_DECL(0x00006a00)
b16 ResetPartyTurnState(void);

// @identity-TODO: that kind 0/1 of 0xda40 are the two enemy groups (graphics slots of 0xe820)
// is inferred from the caller spawning s_fieldMap as kind 0 and s_fieldParamSecond as kind 1.
RVA_DECL(0x00007300)
void SpawnSecondGroupActor(i16 x, i16 y, b16 alternate);

// @identity-TODO: the caller pushes s_fieldMap but the body never reads its argument; confirm
// the declared parameter (or its absence) when the field TU is matched.
RVA_DECL(0x00007340)
i16 GetFacingWall(i16 map);

i16 FindFirstAblePartyMember(void);

void UpdatePartyActionWaits(void);
void TickPartyConditionActions(void);
void MarkActorActionReady(struct Character* actor);
// The tick argument is retained for the retail caller ABI but is unused.
i16 RunPartyTurn(i16 ticks);

i16 FindAbleHumanMember(void);

b32 IsPartyAt(i32 x, i32 y);

#endif // GITEN_GAME_FIELD_H
