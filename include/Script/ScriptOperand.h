#ifndef GITEN_SCRIPT_SCRIPTOPERAND_H
#define GITEN_SCRIPT_SCRIPTOPERAND_H

#include <rva.h>

#include <Ints.h>

// The macca / magnetite the last OpRollActorMacca / OpRollActorMagnetite
// rolled for the script actor.
extern i32 g_rolledMacca;
extern i32 g_rolledMagnetite;

// The unit OpPrintNumber appends to a printed value ("マッカ" or "ＭＡＧ"
// after the script read a macca or magnetite amount).
// @identity-TODO: its owner TU is unclaimed.
extern char g_numberUnit[];

// Reads one typed operand from the script and returns its value's slot.
i32* ReadScriptOperand(void);

void ReadContestValues(i16 stat, i32* own, i32* other, i16 swap);

// Macca, magnetite, twice the experience per able member, or first drop.
i32 GetBattleResultValue(i16 which);

// The last action's result, HP change, MP change, or experience drain.
i32 GetActionValue(i16 which);

#endif // GITEN_SCRIPT_SCRIPTOPERAND_H
