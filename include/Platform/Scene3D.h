#ifndef GITEN_PLATFORM_SCENE3D_H
#define GITEN_PLATFORM_SCENE3D_H

#include <rva.h>

#include <Gfx/DDError.h>
#include <Gfx/Sprite.h>
#include <Platform/D3DApp.h>
#include <Platform/D3DMath.h>
#include <Platform/Direct3D.h>

#include <string.h>

// The Direct3D state of the platform layer (C++ only).

// The part of the 3D view whose z-buffer each frame clears.
extern D3DRECT g_viewClearRect;

// The FIGHT banner (w\fight.bmp, 224x48), drawn above the scene at x 208.
extern Picture g_fightBannerPicture;

// @identity-TODO: set when the billboards keep full brightness; its setter
// is unrecovered.
extern b32 g_fixedLighting;

// The direction the party faces (0..3).
extern i32 g_viewDirection;

// The horizontal axis billboards are laid along (perpendicular to the view).
// @identity-TODO: written by the turn step (0x44a970) from the sin/cos tables.
extern D3DVALUE g_billboardX;
extern D3DVALUE g_billboardZ;

// The treasure box being opened (its closed frame then shows open).
// @identity-TODO: set by 0x449fc0 with two other flags; the names are
// inferred.
extern b32 g_boxOpening;
extern struct TreasureBox* g_openingBox;

// The camera's eye and the point it looks at.
extern D3DVECTOR g_cameraFrom;
extern D3DVECTOR g_cameraAt;

extern D3DMATRIX g_viewMatrix;
extern D3DMATRIX g_projectionMatrix;

// The screen rectangle of the layer being dragged (HandleInput).
extern RECT g_dragRect;

// @identity-TODO: the colour StartScreenFade (0x449d60) fades through; owned
// by that TU.
extern D3DCOLOR g_fadeColor;

void DrawDragOutline(void);
void DrawScreenFade(void);

// @identity-TODO: blits the backdrop surface 0x484c54 over the 3D view in game
// states 9 and 12; what it shows is unrecovered.
void BlitFieldBackground(void);

#define GAME_STATE_SHOT 9
#define GAME_STATE_CLOSING_EFFECT 12
#define GAME_STATE_SCRIPT_ANIMATION 37

// Selects and marks the hotspot the party points at.
void DrawHotspotMarks(void);

// The target reticle: its size, half size, the frames it takes to close in,
// and the farthest point its centre is drawn at.
#define MARK_SIZE 96
#define MARK_HALF 48
#define MARK_STEPS 4
#define MARK_RIGHT 592
#define MARK_BOTTOM 280

// Blits the visible screen layers in [first, last) (by the draw-order table
// 0x46bc38) onto the render target.
// @identity-TODO: the flag bits (0x80000000 walks the table) are unrecovered.
RVA_DECL(0x0004e1d0)
void BlitScreenLayers(i32 first, i32 last, u32 flags);

// BlitScreenLayers' flag to walk the layers in draw order (table 0x46bc38).
#define BLIT_LAYERS_ORDERED 0x80000000

// Blits the text planes in [first, last) that are in use and visible onto the
// render target, except those whose bit is set in `skip`; returns how many.
RVA_DECL(0x0004e2d0)
i32 BlitTextPlanes(i32 first, i32 last, u32 skip);

// BlitTextPlanes' skip bit for `plane`.
#define SKIP_TEXT_PLANE(plane) (1u << (plane))

// @identity-TODO: the text plane the scene and event modes leave out; its
// role is unrecovered.
#define TEXT_PLANE_SCENE_HIDDEN 31

// The layers from the top down, as the layer code keeps them.
extern struct ScreenLayer* g_layerStack[SCREEN_LAYER_COUNT];

// The status screen's game state (pushed by the status command), and the
// status phase that hides the first layers.
// @identity-TODO: what phase 3 shows is unrecovered.
#define GAME_STATE_STATUS 0x19
#define STATUS_PHASE_NO_LAYERS 3

// Draws the sprites of the sprite table over the render target.
RVA_DECL(0x0004dcd0)
i32 DrawSprites(void);

// Draws the placed sprites back to front, each centred on its point and
// lifted by three quarters of its height.
void DrawSceneSprites(void);

// Blits the backdrop in game state 0x25.
void DrawSceneOverlay(void);

void RenderFieldView(BOOL draw);
void DrawMouseCursor(void);
void RenderStatusMode(BOOL draw);
void RenderLayersMode(BOOL draw);
void RenderNothing(BOOL draw);

// Renders and presents one frame of the current render mode.
void RenderFrame(void);

// The render mode's handler index (RenderFrame masks g_renderMode with it).
#define RENDER_MODE_MASK 0x0f

// The frame handlers of RENDER_MODE_EVENT, RENDER_MODE_VIEW and
// RENDER_MODE_PANEL.
// @identity-TODO: label-only until 0x44e910, 0x44e3d0 and 0x44ed80 are
// reconstructed.
RVA_DECL(0x0004e910)
void RenderEventMode(BOOL draw);
void RenderViewMode(BOOL draw);
void RenderPanelMode(BOOL draw);

// The world map's six screens, where each shows its part of the map, and the
// party marker's clamp.
#define MAP_SCREEN_COUNT 6
#define MAP_MARKER_MAX_X 636
#define MAP_MARKER_MAX_Y 324

struct MapScreenOffset {
    i16 x;
    i16 y;
};
extern MapScreenOffset g_mapScreenOffsets[MAP_SCREEN_COUNT];
void RenderFieldMode(BOOL draw);

// Pumps the joystick and mouse; returns the key state byte of 0x448e60.
// @identity-TODO: the returned byte's meaning is unrecovered.
RVA_DECL(0x000501e0)
u8 PollInput(void);

// The object and cell of the hotspot the cursor last moved to.
extern i32 g_hotspotObject;
extern i32 g_hotspotCellX;
extern i32 g_hotspotCellY;

// The mouse cursor and the busy cursor (32x32 pictures).
extern Picture g_cursorPicture;
extern Picture g_busyCursorPicture;

// The cursor clip rectangle ConfineCursor saved.
extern RECT g_savedClipRect;

class CMidiStream;
extern CMidiStream* g_midiStream;

// @identity-TODO: builds the 11x11 room geometry and the compass (per the
// FieldView.h note on RebuildViewScene); read from their bodies only.
RVA_DECL(0x0004f780)
void BuildRoomGeometry(void);

RVA_DECL(0x0004f6e0)
void ResetCamera(void);
void RenderTBox(void);
void RenderNPC(BOOL ownCellOnly);
void RenderEnemy(BOOL shade, BOOL anyCell, BOOL byDistance);

// @identity-TODO: the turn animation's step (0..30, written by the turn step
// 0x44a970); enemies switch image past step 15.
extern u32 g_turnStep;

// The 2D enemy pictures drawn when a texture is too wide for the device
// (512x256 each); the last one is the big enemy.
#define ENEMY_PICTURE_COUNT 6
#define ENEMY_PICTURE_BIG 5
extern Picture g_enemyPictures[ENEMY_PICTURE_COUNT];

struct Mesh;
i32 AnimateDoor(struct Mesh* mesh);

// The door leaves' mesh.
extern struct Mesh g_doorMesh;

// The room and wall meshes (see RenderViewMode).
extern struct Mesh g_roomMesh;
extern struct Mesh g_wallMesh;

// The cell codes DrawStairs draws: stairs and steps up (even) or down (odd);
// CELL_STAIRS_NEAR marks those half a cell nearer.
// @identity-TODO: how the "steps" pair (0x90, 0x91) differs from the stairs
// is unrecovered; the up/down texture split is DrawStairs' low-bit test.
#define CELL_STAIRS_UP 0x42
#define CELL_STAIRS_DOWN 0x43
#define CELL_STEPS_UP 0x90
#define CELL_STEPS_DOWN 0x91
#define CELL_STAIRS_NEAR 0x08

// The stairs quad: its half width and its distance ahead.
#define STAIRS_HALF_WIDTH 160
#define STAIRS_FAR 480

void DrawStairs(void);

// The walls BuildRoomGeometry lays out: the cells within ROOM_RADIUS of the
// party (ROOM_SPAN across), a cell's size, and the wall height.
#define ROOM_RADIUS 5
#define ROOM_SPAN 11
#define CELL_SIZE 320
#define WALL_TOP 324.0f

// Wall kinds: kind WALL_OPEN draws nothing (GetWallAt counts it as none);
// kinds up to 2 and WALL_PLAIN_ALT use the plain wall quarter of the atlas,
// the others the door quarter.
// @identity-TODO: what kinds 6 and 11 are is unrecovered.
#define WALL_OPEN 6
#define WALL_PLAIN_ALT 11

// The areas whose map wraps around its edges: across only, or both ways.
// @identity-TODO: which areas these are is unrecovered.
#define AREA_WRAP_X 0x36
#define AREA_WRAP_XY 0x38

// The world-map marker bitmaps (resources 0xf7..0xfa, then the party's
// 0xf6), and the cursor images.
#define MARKER_BITMAP_FIRST 0xf7
#define MARKER_BITMAP_PARTY 0xf6
#define CURSOR_IMAGE 0x2b5
#define BUSY_CURSOR_IMAGE 0x2b6

extern Picture g_targetPicture;
extern Picture g_whitePicture;
extern Picture g_mapMarkerPicture;
extern Picture g_statusPortraitPicture;
extern Picture g_centerFieldMessagePicture;
extern Picture g_leftFieldMessagePicture;
extern Picture g_rightFieldMessagePicture;
extern Picture g_effectFramePicture;
extern Picture g_backdropPicture;
extern Picture g_spritePicture;

#define DrawScreenQuad(device, vertices)                                                           \
    IDirect3DDevice2_DrawPrimitive(                                                                \
        device,                                                                                    \
        D3DPT_TRIANGLEFAN,                                                                         \
        D3DVT_TLVERTEX,                                                                            \
        vertices,                                                                                  \
        4,                                                                                         \
        D3DDP_DONOTUPDATEEXTENTS                                                                   \
    )

#define DrawLitQuad(vertices)                                                                      \
    IDirect3DDevice2_DrawPrimitive(                                                                \
        g_d3dDevice,                                                                               \
        D3DPT_TRIANGLEFAN,                                                                         \
        D3DVT_LVERTEX,                                                                             \
        vertices,                                                                                  \
        4,                                                                                         \
        D3DDP_DONOTUPDATEEXTENTS                                                                   \
    )

#define SetQuadColor(vertices, value)                                                              \
    ((vertices)[0].color = (vertices)[1].color = (vertices)[2].color = (vertices)[3].color =       \
         (value))

#define SetQuadSpecular(vertices, value)                                                           \
    ((vertices)[0].specular = (vertices)[1].specular = (vertices)[2].specular =                    \
         (vertices)[3].specular = (value))

#define TranslateBillboard(vertices, offsetX, offsetZ)                                             \
    do {                                                                                           \
        (vertices)[0].x += (offsetX);                                                              \
        (vertices)[1].x += (offsetX);                                                              \
        (vertices)[2].x += (offsetX);                                                              \
        (vertices)[3].x += (offsetX);                                                              \
        (vertices)[0].z += (offsetZ);                                                              \
        (vertices)[1].z += (offsetZ);                                                              \
        (vertices)[2].z += (offsetZ);                                                              \
        (vertices)[3].z += (offsetZ);                                                              \
    } while (0)

#define ProjectBillboardRect(rect, vertices)                                                       \
    do {                                                                                           \
        D3DVECTOR corner;                                                                          \
        D3DVECTOR screen;                                                                          \
        corner.x = (vertices)[0].x;                                                                \
        corner.y = (vertices)[0].y;                                                                \
        corner.z = (vertices)[0].z;                                                                \
        ProjectVector(&g_viewMatrix, &corner, &screen);                                            \
        (rect).left = static_cast<LONG>(screen.x);                                                 \
        (rect).top = static_cast<LONG>(screen.y);                                                  \
        corner.x = (vertices)[2].x;                                                                \
        corner.y = (vertices)[2].y;                                                                \
        corner.z = (vertices)[2].z;                                                                \
        ProjectVector(&g_viewMatrix, &corner, &screen);                                            \
        (rect).right = static_cast<LONG>(screen.x);                                                \
        (rect).bottom = static_cast<LONG>(screen.y);                                               \
    } while (0)

// Effect bitmap metadata stores signed horizontal and vertical offsets.
#define GetEffectBitmapOffsetX(bitmap) static_cast<i8>((bitmap)->file.bfReserved1)
#define GetEffectBitmapOffsetY(bitmap) static_cast<i8>((bitmap)->file.bfReserved1 >> 8)

#define InitEffectBlitFx(effect, imageCode)                                                        \
    do {                                                                                           \
        ZeroMemory(&(effect), sizeof((effect)));                                                   \
        (effect).dwSize = sizeof((effect));                                                        \
        if ((imageCode).mirrorHorizontal) {                                                        \
            (effect).dwDDFX |= DDBLTFX_MIRRORLEFTRIGHT;                                            \
        }                                                                                          \
        if ((imageCode).mirrorVertical) {                                                          \
            (effect).dwDDFX |= DDBLTFX_MIRRORUPDOWN;                                               \
        }                                                                                          \
    } while (0)

// Creates the pictures, meshes and textures of the display; FALSE on failure.
b32 LoadGraphics(void);

// The layer code (layer.cpp): creating, freeing and repainting the layers,
// hit-testing them and the navigation pad, the character panel's commands
// and the dragging of the panel layers.
b32 CreateScreenLayer(i32 slot);
void FreeScreenLayers(void);
void UpdateLayerPanels(void);
i32 LayerAtPoint(u32 x, u32 y);
i32 LayerIndexAtPoint(u32 x, u32 y);
b32 PadButtonAtPoint(u32 x, u32 y);
b32 ClickPanelCommand(u32 y);
void ReleasePartyPanel(i32 slot, b32 dragged);
void PlaceDraggedLayer(i32 slot);

// The screen, the 3D view's height, the navigation pad (96x96 in 32x32
// cells), the character panel's eight 24-pixel lines, and the drop areas of
// the party panels (two rows of three) and the layers dragged in the view.
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define VIEW_HEIGHT 328
#define PAD_SIZE 96
#define PAD_CELL 32
#define PANEL_LINES 8
#define PANEL_LINE_HEIGHT 24
#define PANEL_ROW_SPLIT 408
#define PANEL_COLUMN_SPLIT 216
#define PANEL_COLUMN_SPLIT2 424
#define DRAGGED_LAYER_FIRST 4
#define DRAGGED_LAYER_END 8

// The cursor's screen position, read every frame (PollInput).
extern POINT g_cursorPos;

// Where the dragged layer was grabbed, from its top left.
extern POINT g_dragOffset;

// Handles this frame's mouse input (PollInput's bits).
void HandleInput(u8 buttons);

// Releases every surface, texture and mesh of the display and the DirectX
// objects.
void ReleaseGraphics(void);

// The party's move under way: a move command (RunMoveCommand) starts it and
// AnimateMove runs one frame of it. The low nibble (MOVE_STATE_KIND) picks
// the step function, 0 for none; the high bits mark a step through a door,
// which AnimateDoor opens instead (ahead, back, left, right).
extern u32 g_moveState;

#define MOVE_STATE_STEP 1
#define MOVE_STATE_BACK 2
#define MOVE_STATE_LEFT 3
#define MOVE_STATE_RIGHT 4
#define MOVE_STATE_TURN_LEFT 5
#define MOVE_STATE_TURN_RIGHT 6
#define MOVE_STATE_TURN_AROUND 7
#define MOVE_STATE_KIND 0x0f
#define MOVE_STATE_DOOR_AHEAD 0x10
#define MOVE_STATE_DOOR_BACK 0x20
#define MOVE_STATE_DOOR_LEFT 0x40
#define MOVE_STATE_DOOR_RIGHT 0x80

// StepParty's directions relative to the party's facing, and its result for
// a plain step (0x10 when the step goes through a door).
#define STEP_FORWARD 0
#define STEP_RIGHT 1
#define STEP_BACK 2
#define STEP_LEFT 3
#define STEP_WALK 1

// The buttons of the navigation pad on layer SCREEN_LAYER_NAVIGATION.
#define PAD_FORWARD 1
#define PAD_BACK 2
#define PAD_LEFT 3
#define PAD_RIGHT 4

// Draws pad button `button` up or pressed on the navigation layer.
void PressPadButton(i32 button, BOOL pressed);

// Draws pad button `button` (1..4) of the navigation pad on `surface`.
b32 DrawPadButton(LPDIRECTDRAWSURFACE surface, i32 button, b32 pressed);

// g_heldPadButton once the button is let go.
#define PAD_RELEASED (-1)

extern i32 g_heldPadButton;
void RepeatPadMove(BOOL turn);

// The menu bar on layer SCREEN_LAYER_MENU_BAR: its buttons' band and width,
// and the buttons that open the automap and the field menu. Buttons 0..4
// toggle layers 3..7.
#define MENU_BAR_TOP 4
#define MENU_BAR_BOTTOM 28
#define MENU_BUTTON_WIDTH 24
#define MENU_BUTTON_COUNT 7
#define MENU_BUTTON_AUTOMAP 5
#define MENU_BUTTON_FIELD_MENU 6
#define MENU_LAYER_OFFSET 3

// A menu bar button's images, up and pressed.
struct MenuButtonImages {
    u16 up;
    u16 pressed;
};

i32 ClickMenuBar(u32 x, u32 y);

// Shared with the layer code: the menu bar's buttons and the compass images.
extern MenuButtonImages g_menuButtonImages[MENU_BUTTON_COUNT];
extern u32 g_menuButtonX[MENU_BUTTON_COUNT];
extern u16 g_compassImages[4];

// The image tables of the text and layer code, kept in winmain's data: the
// layers' images by slot, the party panels' by member state, the character
// panel's commands (up, pressed), the icon layer's, the pad buttons' (up,
// pressed), and the text planes' status, mark and icon images.
// @identity-TODO: the status image set roles remain unrecovered.
extern u16 g_layerImages[16];
extern u16 g_panelImages[8];
extern MenuButtonImages g_commandImages[10];
extern u16 g_iconLayerImages[28];
extern u16 g_padImages[4][2];
extern u16 g_statusImages[10];
extern u16 g_statBarMarkImages[5];
extern u16 g_fusionSummaryImages[17][3];
extern u16 g_mapTileImages[84];
extern u16 g_mapMarkImages[28];

// The text plane kinds' frame images (CreateTextPlane).
extern u16 g_textPlaneImages[40];

// The move commands (RunMoveCommand's table): each starts its move, first
// advancing the game phase when `nextPhase` is set, and returns whether the
// move started.
b32 MoveForwardCommand(i16 nextPhase);
b32 MoveRightCommand(i16 nextPhase);
b32 MoveBackCommand(i16 nextPhase);
b32 MoveLeftCommand(i16 nextPhase);
b32 TurnRightCommand(i16 nextPhase);
b32 TurnAroundCommand(i16 nextPhase);
b32 TurnLeftCommand(i16 nextPhase);

// How RenderEnemy spreads the objects sharing a cell: a pair to either side,
// more in slots of three (centre, right, left).
#define CELL_PAIR 2
#define SPREAD_SLOTS 3
#define SPREAD_SLOT_LEFT 2

// Billboard brightness remains full within two cells, then attenuates by distance.
#define SetDistanceLight(light, distance)                                                          \
    do {                                                                                           \
        if ((distance) < 640.0) {                                                                  \
            (light) = 1.0f;                                                                        \
        } else {                                                                                   \
            (light) = (320.0 - (distance) / 6.0) / ((distance) - 323.2);                           \
        }                                                                                          \
    } while (0)

#endif // GITEN_PLATFORM_SCENE3D_H
