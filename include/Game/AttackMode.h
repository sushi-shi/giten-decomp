#ifndef GITEN_GAME_ATTACKMODE_H
#define GITEN_GAME_ATTACKMODE_H

#include <Enums.h>

// How an action attacks: a skill's two mode bits (ResolveSkillAttack runs
// weapon and gun skills as those attacks) and the resistance checks' mode.
GZ_ENUM_BEGIN(AttackMode)
    ATTACK_MAGIC = 0,
    ATTACK_WEAPON = 1,
    ATTACK_GUN = 2
GZ_ENUM_END(AttackMode)

#endif // GITEN_GAME_ATTACKMODE_H
