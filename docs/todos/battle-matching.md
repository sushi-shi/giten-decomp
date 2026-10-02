# Battle matching boundaries

These functions have reconstructed behavior but retain an instruction-shape
difference. Use `giten walls diagnose <rva> --asm` for the live comparison.

| Function | Retail evidence | Remaining source question |
| --- | --- | --- |
| [DistributeGunRounds](../../src/Game/partyaction.c) (`0x0091f0`) | Both objects clear the same two arrays, read the same distribution handle, have the same seven branches and ordered referents, and distribute the same bounded shares. The current source models the retail address as the end of the selected raw-buffer row and indexes back into it. The source and retail instructions now agree through the row-address calculation and first loop entry. | Retail reloads `rounds` into `esi` before reading the distribution byte; the current object multiplies from its stack slot. The objects otherwise differ by one instruction and two bytes. Determine the legitimate source lifetime or translation-unit state that accounts for this load. |
| [ComputeSkillDamage](../../src/Game/skillattack.c) (`0x00ac50`) | Calls, branches, return paths, and all 23 ordered referents agree. Retail loads the cached skill value into `cx`, then adds the attacker's magic power directly from memory. The current object first loads magic power into `dx`, loads the skill value into `cx`, then adds `cx` to `dx`. | Determine which source or translation-unit boundary selects the retail operand schedule. Splitting the sum from `WearSkillValue`, splitting the two additions, and commuting operands did not change the compiled object. The matching item-damage sibling uses a split sum and wear call, but that spelling alone does not change this function. |
