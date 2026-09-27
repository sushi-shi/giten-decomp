// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Math/Vec3.h>
#include <Util/Range.h>

// Perspective projection onto the 640x400 screen (centre 320, horizon 114).
RVA(0x0000d3f0, 0x4f)
ScreenPoint ProjectPoint(i16 x, i16 y, i16 z) {
    ScreenPoint point;
    point.x = x * FOCAL_LENGTH / z + 320;
    point.y = (233 - y) * FOCAL_LENGTH / z + 114;
    return point;
}

RVA(0x0000d440, 0x15)
ScreenPoint ProjectFloorPoint(i16 x, i16 z) {
    return ProjectPoint(x, 0, z);
}

// Moves by the 8.8 fixed-point velocity; returns whether any axis moved.
RVA(0x0000d460, 0xa2)
i16 MoveBySubVelocity(Vec3* pos, const Vec3* velocity) {
    i16 moved = 0;
    i16 before;
    before = pos->x;
    pos->x += ClampDelta(before, velocity->x >> 8, -32768, 32767);
    if (before != pos->x) {
        moved = 1;
    }
    before = pos->y;
    pos->y += ClampDelta(before, velocity->y >> 8, -32768, 32767);
    if (before != pos->y) {
        moved = 1;
    }
    before = pos->z;
    pos->z += ClampDelta(before, velocity->z >> 8, -32768, 32767);
    if (before != pos->z) {
        moved = 1;
    }
    return moved;
}

RVA(0x0000d510, 0x65)
void StepBody(Body* body) {
    body->pos.x += ClampDelta(body->pos.x, body->vel.x, -32768, 32767);
    body->pos.y += ClampDelta(body->pos.y, body->vel.y, -32768, 32767);
    body->pos.z += ClampDelta(body->pos.z, body->vel.z, -32768, 32767);
}

// @identity-TODO: returns an FIELD_OUT_* code for the first axis outside the
// field; inside it the function reaches its end without a return value (the
// callers only test for the three codes).
RVA(0x0000d580, 0x41)
i16 CheckFieldBounds(i16 x, i16 y, i16 z) {
    if (x < FIELD_MIN_X || x > FIELD_MAX_X) {
        return FIELD_OUT_X;
    }
    if (y < FIELD_MIN_Y || y > FIELD_MAX_Y) {
        return FIELD_OUT_Y;
    }
    if (z < FIELD_MIN_Z || z > FIELD_MAX_Z) {
        return FIELD_OUT_Z;
    }
}

RVA(0x0000d5d0, 0x1e)
void ClearBody(Body* body) {
    body->vel.x = 0;
    body->pos.x = 0;
    body->vel.y = 0;
    body->pos.y = 0;
    body->vel.z = 0;
    body->pos.z = 0;
}

RVA(0x0000d5f0, 0x53)
void ClampToField(Vec3* pos) {
    pos->x = ClampShort(pos->x, FIELD_MIN_X, FIELD_MAX_X);
    pos->y = ClampShort(pos->y, FIELD_MIN_Y, FIELD_MAX_Y);
    pos->z = ClampShort(pos->z, FIELD_MIN_Z, FIELD_MAX_Z);
}

// Floor position of sub-cell `cell` (3x3) of map cell (column, row).
RVA(0x0000d650, 0x5d)
Vec3* CellToField(i16 column, i16 row, i16 cell, Vec3* out) {
    out->x = (column * 6 + cell % 3) * 75 - 75;
    out->z = 356 - (row * 6 + cell / 3) * 75;
    return out;
}

RVA(0x0000d6b0, 0x4e)
Vec3* UnprojectPoint(i16 x, i16 y, i16 depth, Vec3* out) {
    out->z = depth;
    out->x = UnprojectAxis(x, depth);
    out->y = UnprojectAxis(y, depth);
    return out;
}

// Makes (*x, *y) relative to the origin and rotates it into the frame of an
// observer facing `facing` (0..3).
// Makes (*x, *y) relative to the origin and rotates it into the frame of an
// observer facing `facing` (0..3); the origin arguments are reused for the
// result, so other facings store them back.
RVA(0x0000d700, 0x84)
void RotateOffset(i16* x, i16* y, i16 originX, i16 originY, i16 facing) {
    *x -= originX;
    *y -= originY;
    switch (facing) {
        case 0:
            originX = *x;
            originY = *y;
            break;
        case 1:
            originX = *y;
            originY = -*x;
            break;
        case 2:
            originX = -*x;
            originY = -*y;
            break;
        case 3:
            originX = -*y;
            originY = *x;
            break;
    }
    *x = originX;
    *y = originY;
}
