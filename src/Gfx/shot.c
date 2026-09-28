// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/GameState.h>
#include <Game/StateStack.h>
#include <Gfx/Motion.h>
#include <Gfx/Shot.h>

DATA(0x00078248)
static ShotFile s_shotData;

DATA(0x00078348)
static Vec3 s_shotPos;

// Velocity (8.8 fixed point) and its per-step change.
DATA(0x00078350)
static Body s_shotMotion;

DATA(0x00078360)
static Vec3 s_shotStart;

DATA(0x00078368)
static Vec3 s_shotTarget;

DATA(0x00078370)
static ShotTable s_shotTableData;

DATA(0x00078470)
static ShotTable* s_shotTable;

// Sideways drift per unit of depth travelled.
DATA(0x00078478)
static double s_shotSlope;

DATA(0x00078480)
static i16 s_shotPower;

RVA(0x00005480, 0x69)
void LoadShotTable(FILE* fp) {
    u16 i;
    s_shotTable = &s_shotTableData;
    ReadRawBlock(fp, &s_shotData);
    s_shotTable->count = s_shotData.table.count;
    for (i = 0; i < s_shotData.table.count; i++) {
        s_shotTable->kinds[i] = (Body*)(s_shotData.bytes + s_shotData.table.offsets[i]);
    }
}

RVA(0x000054f0, 0x127)
i16 SetShotPath(i16 fromX, i16 fromY, i16 toX, i16 toY) {
    MapCoord origin = GetMapCoord();
    i16 distance;
    i16 dx, dz;
    double lateral;
    RotateOffset(&fromX, &fromY, origin.x, origin.y, g_field.pos.direction);
    RotateOffset(&toX, &toY, origin.x, origin.y, g_field.pos.direction);
    distance = toY - fromY;
    CellToField(fromX, fromY, 4, &s_shotStart);
    CellToField(toX, toY, 4, &s_shotTarget);
    s_shotStart.z++;
    s_shotTarget.z += 2;
    s_shotStart.z = s_shotStart.z == 0 ? 1 : s_shotStart.z;
    s_shotTarget.z = s_shotTarget.z == 0 ? 1 : s_shotTarget.z;
    dx = s_shotTarget.x - s_shotStart.x;
    dz = s_shotTarget.z - s_shotStart.z;
    lateral = dx;
    if (dz != 0) {
        s_shotSlope = lateral / dz;
    } else {
        s_shotSlope = 0;
    }
    return distance;
}

// Places the shot at the start point and height with the kind's motion for
// the given strength, mirrored in depth when the target lies nearer.
RVA(0x00005620, 0xb3)
i16 StartShot(i16 kind, i16 strength, i16 height) {
    Body* motions;
    if (strength < 0) {
        strength = -strength;
    }
    motions = GetShotKindMotion(s_shotTable, kind);
    s_shotMotion = *motions;
    if (strength > 0) {
        s_shotMotion = GetShotKindMotion(s_shotTable, kind)[strength - 1];
    }
    s_shotPos.x = s_shotStart.x;
    s_shotPos.y = height;
    s_shotPos.z = s_shotStart.z;
    if (s_shotTarget.z <= s_shotStart.z) {
        s_shotMotion.pos.z = -s_shotMotion.pos.z;
        s_shotMotion.vel.z = -s_shotMotion.vel.z;
    }
    return strength;
}

// Returns 1 while the shot flies, 0 once it reached its target depth or left
// the field sideways/vertically (it is then clamped and put on the floor),
// and -1 when it left the field across x.
// @early-stop: block placement only. Retail branches into the bounds check
// (jl) and keeps the "return 1" block after the arrival block; goto,
// ternary and flag spellings all fall through into the check and place it
// inline.
RVA(0x000056e0, 0xdb)
i16 StepShot(void) {
    Vec3 before = s_shotPos;
    i16 result;
    MoveBySubVelocity(&s_shotPos, &s_shotMotion.pos);
    StepBody(&s_shotMotion);
    s_shotPos.x += (i16)((s_shotPos.z - before.z) * s_shotSlope);
    if (s_shotTarget.z - s_shotStart.z > 0) {
        if (s_shotPos.z >= s_shotTarget.z) {
            goto arrived;
        }
    } else if (s_shotPos.z <= s_shotTarget.z) {
        goto arrived;
    }
    result = CheckFieldBounds(s_shotPos.x, s_shotPos.y, s_shotPos.z);
    if (result == FIELD_OUT_X) {
        return result;
    }
    if (result != FIELD_OUT_Z && result != FIELD_OUT_Y) {
        return 1;
    }
arrived:
    ClampToField(&s_shotPos);
    s_shotPos.y = 0;
    return 0;
}

RVA(0x000057c0, 0x40)
Vec3* ShotRelativePoint(i16 x, i16 y, i16 depth, Vec3* out) {
    UnprojectPoint(x, y, depth, out);
    out->x += s_shotPos.x;
    out->y += s_shotPos.y;
    out->z = s_shotPos.z;
    return out;
}

RVA(0x00005800, 0xe)
void StopShot(void) {
    ClearBody(&s_shotMotion);
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00005810, 0x8b)
void GetShotScaledPosition(i16* x, i16* y, i16* z) {
    *x = s_shotPos.x * 450 / 320;
    *y = s_shotPos.y * 373 / 320;
    *z = s_shotPos.z * 450 / 320;
    if (*z == 0) {
        *z = 1;
    }
}

RVA(0x000058a0, 0x2a)
void GetShotPosition(i16* x, i16* y, i16* z) {
    *x = s_shotPos.x;
    *y = s_shotPos.y;
    *z = s_shotPos.z;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x000058d0, 0x24)
void SetShotPosition(i16 x, i16 y, i16 z) {
    s_shotPos.x = x;
    s_shotPos.y = y;
    s_shotPos.z = z;
}

RVA(0x00005900, 0x12)
void SetShotPower(i16 power) {
    if (power < 0) {
        power = -power;
    }
    s_shotPower = power;
}

RVA(0x00005920, 0x7)
i16 GetShotPower(void) {
    return s_shotPower;
}

RVA(0x00005930, 0x41)
void LoadEffectTables(void) {
    FILE* fp;
    InitEffectSlots();
    InitEffectImageSets();
    fp = OpenDataFile(0x10, 2, 0);
    LoadMotionTable(fp);
    LoadShotTable(fp);
    LoadEffectPalettes(fp);
    CloseDataFile(fp);
}

RVA(0x00005980, 0x42)
void LaunchShot(i16 effect, i16 mode, i16 rise, i16 fromX, i16 fromY, i16 toX, i16 toY) {
    i16 strength;
    PushGameState(9);
    strength = SetShotPath(fromX, fromY, toX, toY);
    StartEffect(effect, strength);
    SetShotPower(strength);
}

RVA(0x000059d0, 0x60)
b16 RunShotState(void) {
    i16 result;
    if (GetGamePhase() == 0) {
        ExchangeEffectSkipping(0);
        NextGamePhase();
    }
    result = StepShot();
    if (result == 0) {
        CloseEffect();
        ReturnFromGameState();
        return false;
    }
    StepEffectScript();
    if (GetEffectScript() == NULL || result < 0) {
        ReturnFromGameState();
        StopEffectScript();
        ReleaseEffectPalettes();
    }
    return false;
}

RVA(0x00005a30, 0x50)
b16 RunClosingEffectState(void) {
    if (GetEffectScript() != NULL) {
        ExchangeEffectSkipping(1);
        StepEffectScript();
        if (GetEffectScript() != NULL) {
            return false;
        }
    }
    FreeEffectImageSet(0);
    FreeEffectRecord(0);
    ReturnFromGameState();
    ReleaseEffectPalettes();
    return false;
}
