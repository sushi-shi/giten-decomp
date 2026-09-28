#ifndef GITEN_GAME_FIELDHUD_H
#define GITEN_GAME_FIELDHUD_H

#include <rva.h>

#include <Game/GameState.h>
#include <Ints.h>
#include <Ui/Panel.h>

// Callees of the field-screen unit (Game/fieldscreen.c), label-only until
// their TUs are claimed. Kept out of the headers field.c and fieldmain.c
// include (TU state).

// @identity-TODO: whether a member who can act (the leader, member 10 or 11)
// is present on a cell that allows it; the talk, DDS and mapping commands
// and the automap check it.
b16 CanOpenAutomap(void);

// Reveals connected cells around x/y in the current automap block.
void RevealAutomapCells(i16 x, i16 y);

// The origin (+0x34/+0x36) of level `level`'s area-map block.
MapCoord GetAreaSize(i16 level);

// The world-map block the view shows (-1: none).
i16 GetWorldBlock(void);

// The world-map view's top-left world cell.
MapCoord GetWorldViewOrigin(void);

// The row handlers of the field command panel (fieldscreen.c) and the world
// panel (worldblocks.c).
i16 CommandRowHandler(PanelRow* row, i16 value, i16 op);
i16 ReorderRowHandler(PanelRow* row, i16 value, i16 op);
i16 StatusRowHandler(PanelRow* row, i16 value, i16 op);
i16 SkillRowHandler(PanelRow* row, i16 value, i16 op);
i16 ItemRowHandler(PanelRow* row, i16 value, i16 op);
b16 CanHumanMemberAct(void);
i16 DdsRowHandler(PanelRow* row, i16 value, i16 op);
i16 FightRowHandler(PanelRow* row, i16 value, i16 op);
i16 TalkRowHandler(PanelRow* row, i16 value, i16 op);
i16 MappingRowHandler(PanelRow* row, i16 value, i16 op);
i16 MenuRowHandler(PanelRow* row, i16 value, i16 op);
void SetFieldPanelImage(u32 image);
b32 RunFieldPanelRow(i16 row, i32 op, u16 clear, u16 set);
b32 SetFieldPanelRowChecked(i16 row, i16 on);
b32 IsFieldPanelRowChecked(i16 row);
void TalkCommand(void);
void FightCommand(i16 id);
void GunCommand(i16 id);
void SkillCommand(i16 id);
void ItemCommand(i16 id);
b16 CanMemberAct(i16 id);
void DefenceCommand(i16 id);
void ReturnCommand(i16 id);
void DdsCommand(void);
void StatusCommand(void);
i16 WorldRowHandler(PanelRow* row, i16 value, i16 op);

ub32 LoadMenuImage(i16 id);

// The Windows port reports no saved screen area for a panel.
MapCoord GetPanelSize(Panel* panel);

// @identity-TODO: helpers of the field view refresh: flushing the pending
// plane updates (0x445570) and redrawing the view at x/y facing `direction`
// (0x4140a0).

void FlushPlaneUpdates(void);

void RedrawFieldAt(i16 x, i16 y, i16 direction);

// @identity-TODO: callees of the field commands: the number of hotspots of `kind`
// (0x445680; kind 2 with a pending abort and `consume` set gives -1 and
// clears the abort), requesting the talk (0x412870 sets g_pendingTalk) and
// setting the item user (SetUseMemberId in ItemUse.h).
i16 CountHotspotsOfKind(i16 kind, i16 consume);

i16 RequestTalk(void);

// The status panel redraw (`force` redraws even without a pending request).
// @identity-TODO: the map position and word the held view keeps; the word
// is cleared with them and read nowhere else.
void RefreshStatusPanel(i16 force);

extern i16 g_viewX;
extern i16 g_viewY;
extern i16 g_viewReset;

b16 PrepareFieldRedraw(i16 force);

void ResetWorldCursorCell(void);
void FreeFieldImage(void);
b16 RebuildFieldView(void);

void InitWorldPanel(void);

#endif // GITEN_GAME_FIELDHUD_H
