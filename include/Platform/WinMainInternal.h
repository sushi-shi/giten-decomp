#ifndef GITEN_PLATFORM_WINMAININTERNAL_H
#define GITEN_PLATFORM_WINMAININTERNAL_H

#include <Ints.h>
#include <Platform/Direct3D.h>

static b32 NoMoveCommand(i16 nextPhase);
static b32 SlideForward(D3DVALUE* progress);
static b32 SlideBack(D3DVALUE* progress);
static b32 SlideLeft(D3DVALUE* progress);
static b32 SlideRight(D3DVALUE* progress);
static b32 TurnLeftStep(D3DVALUE* progress);
static b32 TurnRightStep(D3DVALUE* progress);
static b32 TurnAroundStep(D3DVALUE* progress);
static b32 NoMoveStep(D3DVALUE* progress);

#endif // GITEN_PLATFORM_WINMAININTERNAL_H
