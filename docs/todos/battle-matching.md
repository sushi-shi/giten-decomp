# Battle matching boundaries

These functions have reconstructed behavior but retain an instruction-shape
difference. Use `giten walls diagnose <rva> --asm` for the live comparison.

| Function | Retail evidence | Remaining source question |
| --- | --- | --- |
| [DistributeGunRounds](../../src/Game/partyaction.c) (`0x0091f0`) | Both objects clear the same two arrays, read the same distribution handle, have the same seven branches and ordered referents, and distribute the same bounded shares. Retail computes the table address from `count * 15`, then reads at `index - 15`; the current source names the preceding row and reads at `index`. | Recover the original pointer boundary without manufacturing an out-of-bounds view. Moving the row assignment before the loop changes MSVC's control flow and loses the measured improvement. |
| [ComputeSkillDamage](../../src/Game/skillattack.c) (`0x00ac50`) | Calls, branches, return paths, and all 23 ordered referents agree. Retail loads the cached skill value into `cx`, then adds the attacker's magic power directly from memory. The current object first loads magic power into `dx`, loads the skill value into `cx`, then adds `cx` to `dx`. | Determine which source or translation-unit boundary selects the retail operand schedule. Splitting the sum from `WearSkillValue`, splitting the two additions, and commuting operands did not change the compiled object. The matching item-damage sibling uses a split sum and wear call, but that spelling alone does not change this function. |

