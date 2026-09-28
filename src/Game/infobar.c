// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/AreaMap.h>
#include <Game/Clock.h>
#include <Game/FieldSupport.h>
#include <Game/GameState.h>
#include <Game/InfoBar.h>
#include <Game/ModeFlags.h>
#include <Game/WorldMap.h>
#include <Gfx/ScreenLayer.h>
#include <Platform/GameCalls.h>
#include <Text/Font.h>
#include <Util/Scratch.h>

#include <stdio.h>
#include <string.h>

DATA(0x00068c30)
static i32 s_shownMagnetite = -1;
DATA(0x00068c34)
static i32 s_shownMacca = -1;
DATA(0x00068c38)
static i16 s_shownMoonPhase = -1;
DATA(0x00068c3c)
static i16 s_nextLayout = 1;

RVA(0x0001e720, 0x12)
i16 SetInfoBarLayout(i16 layout) {
    i16 previous = s_nextLayout;
    s_nextLayout = layout;
    return previous;
}

RVA(0x0001e740, 0x4b)
void DrawMoneyCounters(i16 mode) {
    DrawMoneyCounter(mode, 8, RosterMemberAt(0)->magnetite, 0);
    s_shownMagnetite = RosterMemberAt(0)->magnetite;
    DrawMoneyCounter(mode, 11, RosterMemberAt(0)->macca, 1);
    s_shownMacca = RosterMemberAt(0)->macca;
}

RVA(0x0001e790, 0xcf)
void DrawMoneyCounter(i16 mode, i16 row, i32 value, i16 currency) {
    i32 attr = 0xffffb400;
    if (!currency) {
        DrawLayerText(SCREEN_LAYER_CURRENCY, 8, 8, "       ", 0xb400);
        if (value < 1) {
            attr = 0xffffb500;
        }
        sprintf(g_scratchBuffer, "%7ld", value);
        DrawLayerText(SCREEN_LAYER_CURRENCY, 8, 8, g_scratchBuffer, attr);
        DrawLayerText(SCREEN_LAYER_CURRENCY, 72, 8, "MAG", attr);
    } else {
        DrawLayerText(SCREEN_LAYER_CURRENCY, 40, 32, "       ", 0xb400);
        if (value < 1) {
            attr = 0xffffb500;
        }
        sprintf(g_scratchBuffer, "%7ld", value);
        DrawLayerText(SCREEN_LAYER_CURRENCY, 8, 32, "\\", attr);
        DrawLayerText(SCREEN_LAYER_CURRENCY, 40, 32, g_scratchBuffer, attr);
    }
}

RVA(0x0001e860, 0x81)
b16 DrawInfoBar(i16 layout, i16 partial) {
    DrawIconLayerImage(g_clock.moonPhase);
    sprintf(g_scratchBuffer, "%2d", g_clock.moonPhase + 1);
    DrawLayerText(SCREEN_LAYER_MOON_PHASE, 8, 8, g_scratchBuffer, 0xb400);
    s_shownMoonPhase = g_clock.moonPhase;
    DrawMoneyCounters(layout);
    if (!partial || TestModeFlags(MODE_WORLD_MAP)) {
        DrawAreaInfo();
    }
    return false;
}

RVA(0x0001e8f0, 0xf5)
void DrawAreaInfo(void) {
    i16 floor;
    i16 x;
    ClearLocationCaption();
    if (!TestModeFlags(MODE_WORLD_MAP)) {
        strcpy(g_scratchBuffer, GetAreaName());
    } else {
        FormatWorldMapLocation();
    }
    DrawLayerText(SCREEN_LAYER_LOCATION, 8, 8, g_scratchBuffer, 0x3400);
    floor = GetLevelFloor();
    DrawLayerText(SCREEN_LAYER_LOCATION, 176, 8, "    ", 0xb400);
    if (floor) {
        x = 176;
        if (floor < 0) {
            sprintf(g_scratchBuffer, "B%1dF", -floor);
            if (floor > -10) {
                x = 184;
            }
        } else {
            sprintf(g_scratchBuffer, " %2dF", floor);
        }
        DrawLayerText(SCREEN_LAYER_LOCATION, x, 8, g_scratchBuffer, 0xb400);
    }
}

RVA(0x0001e9f0, 0x60)
b16 RefreshInfoBar(i16 force) {
    if (force) {
        DrawInfoBar(s_nextLayout, 1);
    } else if (s_shownMoonPhase != g_clock.moonPhase
               || s_shownMagnetite != RosterMemberAt(0)->magnetite
               || s_shownMacca != RosterMemberAt(0)->macca) {
        DrawInfoBar(s_nextLayout, 1);
    }
    s_nextLayout = 1;
    return false;
}

RVA(0x0001ea50, 0x34)
b16 UpdateInfoBar(void) {
    if (g_fieldRedrawRequest) {
        DrawInfoBar(0, 0);
        return false;
    }
    if (g_tickElapsed >= CLOCK_UPDATE_MOON) {
        DrawInfoBar(1, 0);
    }
    return false;
}
