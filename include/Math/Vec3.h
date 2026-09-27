#ifndef GITEN_MATH_VEC3_H
#define GITEN_MATH_VEC3_H

#include <Ints.h>
#include <Math/Coord.h>

// A point or offset in the 3D field space (x across, y up, z into the view).
typedef struct Vec3 {
    i16 x;
    i16 y;
    i16 z;
} Vec3;

// A moving point: position and per-step velocity.
typedef struct Body {
    Vec3 pos;
    Vec3 vel;
} Body;

// A projected screen position, returned by value.
typedef Coord ScreenPoint;

// The field volume bodies are kept in, and CheckFieldBounds' results.
#define FIELD_MIN_X -2475
#define FIELD_MAX_X 2475
#define FIELD_MIN_Y 0
#define FIELD_MAX_Y 373
#define FIELD_MIN_Z 281
#define FIELD_MAX_Z 2306
#define FIELD_OUT_X -1
#define FIELD_OUT_Y -2
#define FIELD_OUT_Z -3

// The projection's focal length; the near plane sits at the same depth.
#define FOCAL_LENGTH 281

static __inline i16 UnprojectAxis(i16 value, i16 depth) {
    return value * depth / FOCAL_LENGTH;
}

ScreenPoint ProjectPoint(i16 x, i16 y, i16 z);
ScreenPoint ProjectFloorPoint(i16 x, i16 z);
i16 MoveBySubVelocity(Vec3* pos, const Vec3* velocity);
void StepBody(Body* body);
i16 CheckFieldBounds(i16 x, i16 y, i16 z);
void ClearBody(Body* body);
void ClampToField(Vec3* pos);
Vec3* CellToField(i16 column, i16 row, i16 cell, Vec3* out);
Vec3* UnprojectPoint(i16 x, i16 y, i16 depth, Vec3* out);
void RotateOffset(i16* x, i16* y, i16 originX, i16 originY, i16 facing);

#endif // GITEN_MATH_VEC3_H
