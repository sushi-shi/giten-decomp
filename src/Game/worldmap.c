// @identity-TODO: the owning TU is unproven; this unit holds the world-map
// state's span until link-order evidence names it.

#include <rva.h>

#include <Game/Battle.h>
#include <Game/Clock.h>
#include <Game/Field.h>
#include <Game/FieldMain.h>
#include <Game/FieldMap.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/InfoBar.h>
#include <Game/ModeFlags.h>
#include <Game/Scene.h>
#include <Game/StateStack.h>
#include <Game/StatusDraw.h>
#include <Game/WaitState.h>
#include <Game/WorldMap.h>
#include <Gfx/Render.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/ScreenMode.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/VideoState.h>
#include <Gfx/Vram.h>
#include <Platform/GameCalls.h>
#include <Sound/Sound.h>
#include <Text/TextWindow.h>
#include <Ui/Hotspot.h>
#include <Ui/Message.h>

// Pending transition request: negative exits, positive marks the entry spot.
DATA(0x0007be38)
i16 g_worldMapRequest;

// @identity-TODO: the video state saved while the world map is shown; the
// Windows build's save/restore bodies are empty, so its layout is unknown.
DATA(0x0007bb60)
static u8 s_videoState[16];

// The spot marked when the world map is entered with a request.
DATA(0x0007be60)
static i16 s_savedSpotX;

DATA(0x0007be64)
static i16 s_savedSpotY;

// @identity-TODO: the map layer the travel routines work on.
DATA(0x0007be68)
static i16 s_mapLayer;

// Set while the party travels towards a picked destination.
DATA(0x0007be70)
static i16 s_traveling;

// Travel ticks since the last party step effect, cleared while idle.
DATA(0x0007be74)
static i16 s_idleFlag;

RVA(0x000197a0, 0x9)
i16 IsOddMapLayer(void) {
    return s_mapLayer & 1;
}

RVA(0x000197b0, 0x3b)
void SetWorldMapSpot(i16 layer, i16 x, i16 y) {
    MapCoord origin;
    s_mapLayer = layer;
    origin = GetLayerOrigin(layer);
    s_savedSpotX = origin.x + x;
    s_savedSpotY = origin.y + y;
}

// Runs one frame of the world map, by phase: 0-2 enter it, 3-4 draw the view
// around the party, 5 waits for a destination, 6 travels (advancing the clock
// and checking events and encounters), 7-8 leave it, 9 leaves for an event or
// an encounter's state, 10 re-enters after one.
RVA(0x000197f0, 0x620)
i16 RunWorldMap(void) {
    MapCoord origin;
    i16 steps;
    i16 state;

    SetPanelRenderMode();
    switch (GetGamePhase()) {
        case 0:
            ClearSceneSurfaces();
            NextGamePhase();
            s_traveling = 0;
            g_field.pos.area = 0xff;
            g_field.pos.level = 0;
            g_field.pos.x = 3;
            g_field.pos.y = 3;
            g_field.pos.direction = 0;
            LoadAreaMap(0xff, 0);
            SetModeFlags(MODE_WORLD_MAP);
            if (g_worldMapRequest > 0) {
                g_worldMapX = s_savedSpotX;
                g_worldMapY = s_savedSpotY;
                MarkWorldMapEventSpot(s_savedSpotX, s_savedSpotY);
            }
            LoadCommandMenuImage();
            LoadWorldMapPlaces();
            LoadWorldMapEvents();
            LoadEncounterTables();
            g_worldMapRequest = 0;
            SetFieldMenuMode(2);
        case 1:
            NextGamePhase();
            SaveVideoState(s_videoState);
        case 2:
            SetWorldMapActive(1);
            if (g_worldMapRequest < 0) {
                SetGamePhase(8);
                return 0;
            }
            if (g_worldMapRequest > 0) {
                g_worldMapX = s_savedSpotX;
                g_worldMapY = s_savedSpotY;
                MarkWorldMapEventSpot(s_savedSpotX, s_savedSpotY);
                g_worldMapRequest = 0;
            }
            NextGamePhase();
            PlayMusic(0, 1);
            ClearMaskView();
            ResetMask(1);
            SetFieldStatusBit11(1);
            ClearTextPlane(g_infoPlane);
            SetInfoBarLayout(0);
            RedrawScreen(0, 1);
            FlushStatusRedraw(1);
            StartScreenFadeAndWait(SCREEN_FADE_FROM_BLACK, 1);
            return 1;
        case 3:
            NextGamePhase();
            LoadWorldMapBlocks(GetWorldMapBlock(g_worldMapX, g_worldMapY));
            return 0;
        case 4:
            NextGamePhase();
            origin = GetWorldMapViewOrigin(g_worldMapX, g_worldMapY);
            ScrollWorldMapView(origin.x, origin.y);
            FlushStatusRedraw(1);
            RefreshInfoBar(1);
            ShowWorldMapPlaceName(g_worldMapX, g_worldMapY, 1);
            DiscardWorldMapScreenSave();
            return 0;
        case 5:
            AllowImmediateInput();
            if (g_worldMapRequest < 0) {
                SetGamePhase(7);
                return 0;
            }
            if (g_fieldRedrawRequest) {
                g_fieldRedrawRequest = 0;
                SetGamePhase(2);
                return 0;
            }
            if (FindAbleHumanMember() == -1) {
                CloseMessageWindow();
                PushFieldTextScene(0x2a, 0);
                return 0;
            }
            TrackWorldMapCursor(s_mapLayer);
            if (ProcessPartyCasualties()) {
                RequestStatusRedraw();
            }
            FlushStatusRedraw(0);
            s_idleFlag = 0;
            if (s_traveling) {
                NextGamePhase();
                if (g_tickElapsed >= CLOCK_UPDATE_MOON) {
                    DrawInfoBar(1, 1);
                }
                ShowWorldMapPlaceName(g_worldMapX, g_worldMapY, 0);
                return 0;
            }
            if (PickWorldMapDestination(s_mapLayer)) {
                s_traveling = 1;
                NextGamePhase();
                if (g_tickElapsed >= CLOCK_UPDATE_MOON) {
                    DrawInfoBar(1, 1);
                }
                ShowWorldMapPlaceName(g_worldMapX, g_worldMapY, 0);
                return 0;
            }
            if (g_tickElapsed >= CLOCK_UPDATE_MOON) {
                DrawInfoBar(1, 1);
            }
            ShowWorldMapPlaceName(g_worldMapX, g_worldMapY, 0);
            FireCountdownEvent();
            return 0;
        case 6:
            AllowImmediateInput();
            steps = StepWorldMapTravel(s_mapLayer, 2);
            if (steps == 0) {
                PrevGamePhase();
                s_traveling = 0;
            } else {
                AdvanceClock(steps * 5);
                if (TickStepCounter()) {
                    RequestStatusRedraw();
                }
            }
            if (ProcessPartyCasualties()) {
                RequestStatusRedraw();
            }
            FlushStatusRedraw(0);
            RefreshInfoBar(0);
            ShowWorldMapPlaceName(g_worldMapX, g_worldMapY, 0);
            if (FindAbleHumanMember() == -1) {
                CloseMessageWindow();
                PushFieldTextScene(0x2a, 0);
                return 0;
            }
            if (CheckWorldMapEvent(g_worldMapX, g_worldMapY)) {
                CloseMessageWindow();
                SetGamePhase(9);
                s_traveling = 0;
                SetGameStep(0x17);
                StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
                return 0;
            }
            if (RollWorldMapEncounter(g_worldMapX, g_worldMapY) > 0) {
                CloseMessageWindow();
                g_field.pos.x = 3;
                g_field.pos.y = 3;
                g_field.pos.direction = 0;
                SetGamePhase(9);
                SetGameStep(0x22);
                StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
                return 0;
            }
            break;
        case 7:
            NextGamePhase();
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            return 0;
        case 8:
            SetWorldMapActive(0);
            FreeWorldMapScreenSave();
            ResetWorldMapBlocks(1);
            FreeWorldMapPlaces();
            FreeCommandMenuImage();
            FreeFieldImageCache();
            RestoreVideoState(s_videoState);
            SetSubscreenActive(0);
            SetGameState(0x10);
            ClearModeFlags(MODE_WORLD_MAP);
            SetFieldMenuMode(0);
            RecordWarpInLeader();
            return 0;
        case 9:
            ClearLayerSurface(6);
            CancelLayerDrag();
            state = GetGameStep();
            NextGamePhase();
            SetWorldMapActive(0);
            FreeWorldMapScreenSave();
            ResetWorldMapBlocks(1);
            PushGameState(state);
            return 0;
        case 10:
            LoadWorldMapEvents();
            LoadEncounterTables();
            SetGamePhase(2);
            ClearSceneSurfaces();
            ClearSelectedHotspot();
            break;
    }
    return 0;
}

RVA(0x00019e10, 0x26)
i16 TickStepCounter(void) {
    s_idleFlag++;
    if (s_idleFlag < 50) {
        return 0;
    }
    s_idleFlag = 0;
    return TickPartySteps();
}
