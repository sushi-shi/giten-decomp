// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Alignment.h>
#include <Game/CellTrap.h>
#include <Game/Character.h>
#include <Game/ConditionAge.h>
#include <Game/FieldMain.h>
#include <Game/FieldScreen.h>
#include <Game/Party.h>
#include <Game/Pool.h>
#include <Game/StatusDraw.h>
#include <Sound/Sound.h>
#include <Util/Range.h>

// Codegen constraint: the default joins the signed division with damage
// still zero; an early zero return changes the shared return path.
RVA(0x0001c920, 0xcc)
i32 GetCellTrapDamage(ExitCell* cell, i16 maxHp) {
    i32 damage = 0;
    i16 percent;
    switch (cell->head.code) {
        case CELL_CHUTE:
            SetPendingSound(0x6b);
            if (cell->secondaryDamagePercent < 1) {
                return 0;
            }
            percent = RandomAverage(1, cell->secondaryDamagePercent, 1);
            break;
        case 0x60:
            if (cell->trap.damagePercent < 1) {
                return 0;
            }
            percent = RandomAverage(1, cell->trap.damagePercent, 1);
            break;
        case 0x68:
        case 0x69:
        case 0x6a:
        case 0x6b:
        case 0x6c:
        case 0x6d:
        case 0x6e:
            // Alignment traps reuse the flag-index byte as the damage percentage.
            if (cell->disableFlag[1] < 1) {
                return 0;
            }
            percent = RandomAverage(1, cell->disableFlag[1], 1);
            break;
        default:
            goto done;
    }
    damage = maxHp * percent;
done:
    return damage / 100;
}

// @early-stop stack layout: retail reserves sixteen bytes while the local
// ten-byte ExitCell needs twelve. All operations and register assignments
// agree; the record stride and CopyExitAt forbid padding the cell type.
RVA(0x0001c9f0, 0xee)
void RunCellTrap(i16 mode, i16 x, i16 y) {
    ExitCell cell;
    Character* member;
    i32 damage;
    i16 hp;
    u8 alignmentMask;
    if (mode && CopyExitAt(x, y, &cell)) {
        for (mode = 0; mode < 6; mode++) {
            member = GetPartyCharacter(mode);
            if (member) {
                damage = GetCellTrapDamage(&cell, member->pools.hp.max);
                hp = member->pools.hp.cur;
                if (cell.head.code >= 0x68 && cell.head.code <= 0x6e) {
                    alignmentMask = 4;
                    alignmentMask >>= GetAlignmentClassB(member) + 1;
                    if (!(cell.trap.alignmentMask & alignmentMask)) {
                        continue;
                    }
                }
                ChangePool(&member->pools.hp, -damage);
                if (hp && !member->pools.hp.cur) {
                    ApplyEmptyPools(member);
                }
            }
        }
        if (cell.head.code == CELL_CHUTE) {
            PlaySoundEffect(0x57);
        } else {
            PlaySoundEffect(0x60);
        }
        FlushStatusRedraw(1);
    }
}
