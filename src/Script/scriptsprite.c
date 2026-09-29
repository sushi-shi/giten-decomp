// @identity-TODO: the owning TU is unproven; this unit holds the sprite opcodes'
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/GameState.h>
#include <Game/MapArea.h>
#include <Gfx/Sprite.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptSprite.h>
#include <Util/Debug.h>

// The shop a sprite loaded at a shop's map position belongs to; the sprite
// placement opcode moves the shopkeeper for it and clears it.
// @identity-TODO: the numbering of the shop kinds (1..9) is unrecovered.
DATA(0x000816a0)
i8 g_shopKind = 0;

// Loads an image into a sprite slot: operands slot, image and a load argument.
// The traces are the developer's checks that a shopkeeper image is only loaded
// at its own shop.
RVA(0x000397b0, 0xaa0)
b16 OpLoadSprite(void) {
    i16 slot = ReadScriptValue();
    i16 image = ReadScriptValue();
    i16 arg = ReadScriptValue();

    if (image == 0xe && g_party.field.pos.area == MAP_AREA_RESISTANCE_FRONTLINE_BASE
        && g_party.field.pos.level == 3 && g_party.field.pos.x == 4 && g_party.field.pos.y == 2) {
        g_shopKind = 1;
        slot = 1;
    } else if (slot == 2 && image == 2 && g_party.field.pos.area == MAP_AREA_HATSUDAI
               && g_party.field.pos.level == 8 && g_party.field.pos.x == 2
               && g_party.field.pos.y == 1) {
        g_shopKind = 2;
        slot = 0;
    } else if (slot == 1 && image == 0x18) {
        g_shopKind = 3;
    } else if (image == 0x25) {
        if (g_party.field.pos.area == MAP_AREA_HATSUDAI && g_party.field.pos.level == 6
            && g_party.field.pos.x == 7 && g_party.field.pos.y == 6) {
            g_shopKind = 5;
            // 初台の道具屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\217\211\221\344\202\314\223\271\213\357\211\256\202\305\202\310\202\242\217\352"
                "\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242\201BTakubo\n"
            );
        }
    } else if (image == 0x29) {
        if (g_party.field.pos.area == MAP_AREA_HATSUDAI && g_party.field.pos.level == 6
            && g_party.field.pos.x == 8 && g_party.field.pos.y == 2) {
            g_shopKind = 9;
            // 初台Ｂ６Ｆの食料庫でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\217\211\221\344\202a\202U\202e\202\314\220H\227\277\214\311\202\305\202\310\202"
                "\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242"
                "\201BTakubo\n"
            );
        }
    } else if (image == 0x2b) {
        if (g_party.field.pos.area == MAP_AREA_RINKAI_COLISEUM && g_party.field.pos.level == 1
            && g_party.field.pos.x == 0xb && g_party.field.pos.y == 9) {
            g_shopKind = 9;
            // 臨海コロシアムの薬屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\227\325\212C\203R\203\215\203V\203A\203\200\202\314\226\362\211\256\202\305\202"
                "\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263"
                "\202\242\201BTakubo\n"
            );
        } else if ((g_party.field.pos.area == MAP_AREA_HATSUDAI && g_party.field.pos.level == 8
                    && g_party.field.pos.x == 7 && g_party.field.pos.y == 7)
                   || (g_party.field.pos.area == MAP_AREA_MY_CITY && g_party.field.pos.level == 0
                       && g_party.field.pos.x == 5 && g_party.field.pos.y == 9)
                   || (g_party.field.pos.area == MAP_AREA_SHANSHAN_CITY
                       && g_party.field.pos.level == 0 && g_party.field.pos.x == 0xa
                       && g_party.field.pos.y == 3)
                   || (g_party.field.pos.area == MAP_AREA_OCHANOMIZU && g_party.field.pos.level == 6
                       && g_party.field.pos.x == 1 && g_party.field.pos.y == 5)
                   || (g_party.field.pos.area == MAP_AREA_AKIHABARA_STATION_BUILDING
                       && g_party.field.pos.level == 2 && g_party.field.pos.x == 8
                       && g_party.field.pos.y == 1)
                   || (g_party.field.pos.area == MAP_AREA_MILLENNIUM_HOSPITAL
                       && g_party.field.pos.level == 1 && g_party.field.pos.x == 6
                       && g_party.field.pos.y == 2)
                   || (g_party.field.pos.area == MAP_AREA_GINZA_UNDERGROUND
                       && g_party.field.pos.level == 2 && g_party.field.pos.x == 0x13
                       && g_party.field.pos.y == 6)
                   || (g_party.field.pos.area == MAP_AREA_ROPPONGI && g_party.field.pos.level == 1
                       && g_party.field.pos.x == 5 && g_party.field.pos.y == 1)
                   || (g_party.field.pos.area == MAP_AREA_AMEYA_PLAZA
                       && g_party.field.pos.level == 0 && g_party.field.pos.x == 5
                       && g_party.field.pos.y == 0)
                   || (g_party.field.pos.area == MAP_AREA_AMEYA_PLAZA
                       && g_party.field.pos.level == 1 && g_party.field.pos.x == 4
                       && g_party.field.pos.y == 4)) {
            g_shopKind = 9;
            // レジスタンス前線基地・新宿地下街・神田地下街・銀座地下街秘密区・恵比寿ガーデン・浅草地下街・臨海コロシアム以外の薬屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\203\214\203W\203X\203^"
                "\203\223\203X\221O\220\374\212\356\222n\201E\220V\217h\222n\211\272\212X\201E\220_"
                "\223c\222n\211\272\212X\201E\213\342\215\300\222n\211\272\212X\224\351\226\247\213"
                "\346\201E\214b\224\344\216\365\203K\201["
                "\203f\203\223\201E\220\363\221\220\222n\211\272\212X\201E\227\325\212C\203R\203"
                "\215\203V\203A\203\200\210\310\212O\202\314\226\362\211\256\202\305\202\310\202"
                "\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242"
                "\201BTakubo\n"
            );
        } else if ((g_party.field.pos.area == MAP_AREA_HATSUDAI && g_party.field.pos.level == 8
                    && g_party.field.pos.x == 7 && g_party.field.pos.y == 6)
                   || (g_party.field.pos.area == MAP_AREA_MY_CITY && g_party.field.pos.level == 5
                       && g_party.field.pos.x == 0 && g_party.field.pos.y == 1)
                   || (g_party.field.pos.area == MAP_AREA_OCHANOMIZU && g_party.field.pos.level == 6
                       && g_party.field.pos.x == 8 && g_party.field.pos.y == 3)
                   || (g_party.field.pos.area == MAP_AREA_MILLENNIUM_HOSPITAL
                       && g_party.field.pos.level == 1 && g_party.field.pos.x == 4
                       && g_party.field.pos.y == 4)
                   || (g_party.field.pos.area == MAP_AREA_MILLENNIUM_HEADQUARTERS
                       && g_party.field.pos.level == 0 && g_party.field.pos.x == 0
                       && g_party.field.pos.y == 5)) {
            g_shopKind = 9;
            // 初台・マイシティ・御茶ノ水・ミレニアム病院の病院でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\217\211\221\344\201E\203}"
                "\203C\203V\203e\203B\201E\214\344\222\203\203m\220\205\201E\203~"
                "\203\214\203j\203A\203\200\225a\211@\202\314\225a\211@"
                "\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211"
                "\272\202\263\202\242\201BTakubo\n"
            );
        } else if (g_party.field.pos.area == MAP_AREA_ICHIGAYA && g_party.field.pos.level == 3
                   && g_party.field.pos.x == 1 && g_party.field.pos.y == 4) {
            g_shopKind = 9;
            // 市ヶ谷の受付でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\216s\203\226\222J\202\314\216\363\225t\202\305\202\310\202\242\217\352\215\207"
                "\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242\201BTakubo\n"
            );
        }
    } else if (image == 0x2c) {
        if ((g_party.field.pos.area == MAP_AREA_MY_CITY && g_party.field.pos.level == 0
             && g_party.field.pos.x == 0 && g_party.field.pos.y == 8)
            || (g_party.field.pos.area == MAP_AREA_SHANSHAN_CITY && g_party.field.pos.level == 2
                && g_party.field.pos.x == 5 && g_party.field.pos.y == 4)
            || (g_party.field.pos.area == MAP_AREA_AKIHABARA_STATION_BUILDING
                && g_party.field.pos.level == 2 && g_party.field.pos.x == 4
                && g_party.field.pos.y == 0)
            || (g_party.field.pos.area == MAP_AREA_GINZA_UNDERGROUND && g_party.field.pos.level == 2
                && g_party.field.pos.x == 0x13 && g_party.field.pos.y == 8)
            || (g_party.field.pos.area == MAP_AREA_ROPPONGI && g_party.field.pos.level == 0
                && g_party.field.pos.x == 0xb && g_party.field.pos.y == 1)) {
            g_shopKind = 5;
            // コンピュータショップでない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\203R\203\223\203s\203\205\201[\203^"
                "\203V\203\207\203b\203v\202\305\202\310\202\242\217\352\215\207\202\315\230A\227"
                "\215\202\265\202\304\211\272\202\263\202\242\201BTakubo\n"
            );
        } else if (g_party.field.pos.area == MAP_AREA_AMEYA_PLAZA && g_party.field.pos.level == 0
                   && g_party.field.pos.x == 1 && g_party.field.pos.y == 1) {
            g_shopKind = 4;
            // アメ屋プラザの病院でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\203A\203\201\211\256\203v\203\211\203U\202\314\225a\211@"
                "\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211"
                "\272\202\263\202\242\201BTakubo\n"
            );
        }
    } else if (image == 0x4f) {
        if (g_party.field.pos.area == MAP_AREA_GINZA_UNDERGROUND && g_party.field.pos.level == 3
            && g_party.field.pos.x == 8 && g_party.field.pos.y == 5) {
            g_shopKind = 9;
            // 銀座地下街秘密区の薬屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\213\342\215\300\222n\211\272\212X\224\351\226\247\213\346\202\314\226\362\211"
                "\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304"
                "\211\272\202\263\202\242\201BTakubo\n"
            );
        }
    } else if (image == 0x56) {
        if (g_party.field.pos.area == MAP_AREA_ROPPONGI && g_party.field.pos.level == 0
            && g_party.field.pos.x == 5 && g_party.field.pos.y == 3) {
            g_shopKind = 4;
            // 六本木の酒場でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\230Z\226{"
                "\226\330\202\314\216\360\217\352\202\305\202\310\202\242\217\352\215\207\202\315"
                "\230A\227\215\202\265\202\304\211\272\202\263\202\242\201BTakubo\n"
            );
        }
    } else if (image == 0x71) {
        if (g_party.field.pos.area == MAP_AREA_SHINAGAWA_HOTEL_ILLUSION
            && g_party.field.pos.level == 0 && g_party.field.pos.x == 4
            && g_party.field.pos.y == 1) {
            g_shopKind = 9;
            // 品川ホテル（幻）のフロントでない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\225i\220\354\203z\203e\203\213\201i\214\266\201j\202\314\203t\203\215\203\223"
                "\203g\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304"
                "\211\272\202\263\202\242\201BTakubo\n"
            );
        }
    } else if (image == 0x74) {
        if ((g_party.field.pos.area == MAP_AREA_MY_CITY && g_party.field.pos.level == 0
             && g_party.field.pos.x == 3 && g_party.field.pos.y == 6)
            || (g_party.field.pos.area == MAP_AREA_SHANSHAN_CITY && g_party.field.pos.level == 2
                && g_party.field.pos.x == 4 && g_party.field.pos.y == 1)
            || (g_party.field.pos.area == MAP_AREA_KANDA_UNDERGROUND && g_party.field.pos.level == 5
                && g_party.field.pos.x == 6 && g_party.field.pos.y == 8)
            || (g_party.field.pos.area == MAP_AREA_OCHANOMIZU && g_party.field.pos.level == 6
                && g_party.field.pos.x == 2 && g_party.field.pos.y == 0)
            || (g_party.field.pos.area == MAP_AREA_AKIHABARA_STATION_BUILDING
                && g_party.field.pos.level == 2 && g_party.field.pos.x == 3
                && g_party.field.pos.y == 4)
            || (g_party.field.pos.area == MAP_AREA_GINZA_UNDERGROUND && g_party.field.pos.level == 2
                && g_party.field.pos.x == 0xd && g_party.field.pos.y == 1)
            || (g_party.field.pos.area == MAP_AREA_EBISU_GARDEN && g_party.field.pos.level == 0
                && g_party.field.pos.x == 0xd && g_party.field.pos.y == 0xc)) {
            g_shopKind = 4;
            // マイシティ・シャンシャンシティ２・神田地下街・御茶ノ水・秋葉原・銀座地下街・恵比寿ガーデンプレイスの酒場でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\203}"
                "\203C\203V\203e\203B\201E\203V\203\203\203\223\203V\203\203\203\223\203V\203e\203B"
                "\202Q\201E\220_"
                "\223c\222n\211\272\212X\201E\214\344\222\203\203m\220\205\201E\217H\227t\214\264"
                "\201E\213\342\215\300\222n\211\272\212X\201E\214b\224\344\216\365\203K\201["
                "\203f\203\223\203v\203\214\203C\203X\202\314\216\360\217\352\202\305\202\310\202"
                "\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242"
                "\201BTakubo\n"
            );
        }
    } else if (image == 0x76) {
        if (g_party.field.pos.area == MAP_AREA_RINKAI_COLISEUM && g_party.field.pos.level == 1
            && g_party.field.pos.x == 2 && g_party.field.pos.y == 0) {
            g_shopKind = 4;
            // 臨海コロシアムの病院でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\227\325\212C\203R\203\215\203V\203A\203\200\202\314\225a\211@"
                "\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211"
                "\272\202\263\202\242\201BTakubo\n"
            );
        }
    } else if (image == 0x78) {
        if ((g_party.field.pos.area == MAP_AREA_KANDA_UNDERGROUND && g_party.field.pos.level == 5
             && g_party.field.pos.x == 2 && g_party.field.pos.y == 9)
            || (g_party.field.pos.area == MAP_AREA_GINZA_UNDERGROUND && g_party.field.pos.level == 2
                && g_party.field.pos.x == 0x13 && g_party.field.pos.y == 4)) {
            g_shopKind = 4;
            // 神田地下街・銀座地下街の病院でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\220_"
                "\223c\222n\211\272\212X\201E\213\342\215\300\222n\211\272\212X\202\314\225a\211@"
                "\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211"
                "\272\202\263\202\242\201BTakubo\n"
            );
        }
    } else if (image == 0x79) {
        if ((g_party.field.pos.area == MAP_AREA_SHINJUKU_UNDERGROUND && g_party.field.pos.level == 1
             && g_party.field.pos.x == 7 && g_party.field.pos.y == 0x12)
            || (g_party.field.pos.area == MAP_AREA_MY_CITY && g_party.field.pos.level == 5
                && g_party.field.pos.x == 4 && g_party.field.pos.y == 0xd)
            || (g_party.field.pos.area == MAP_AREA_SHANSHAN_CITY && g_party.field.pos.level == 0
                && g_party.field.pos.x == 0xf && g_party.field.pos.y == 1)
            || (g_party.field.pos.area == MAP_AREA_SHANSHAN_CITY && g_party.field.pos.level == 5
                && g_party.field.pos.x == 0xf && g_party.field.pos.y == 1)
            || (g_party.field.pos.area == MAP_AREA_KANDA_UNDERGROUND && g_party.field.pos.level == 5
                && g_party.field.pos.x == 1 && g_party.field.pos.y == 5)
            || (g_party.field.pos.area == MAP_AREA_OCHANOMIZU && g_party.field.pos.level == 5
                && g_party.field.pos.x == 4 && g_party.field.pos.y == 2)
            || (g_party.field.pos.area == MAP_AREA_AKIHABARA_STATION_BUILDING
                && g_party.field.pos.level == 2 && g_party.field.pos.x == 9
                && g_party.field.pos.y == 4)
            || (g_party.field.pos.area == MAP_AREA_GINZA_UNDERGROUND && g_party.field.pos.level == 2
                && g_party.field.pos.x == 0xa && g_party.field.pos.y == 9)
            || (g_party.field.pos.area == MAP_AREA_EBISU_GARDEN && g_party.field.pos.level == 0
                && g_party.field.pos.x == 0xc && g_party.field.pos.y == 9)
            || (g_party.field.pos.area == MAP_AREA_ROPPONGI && g_party.field.pos.level == 0
                && g_party.field.pos.x == 9 && g_party.field.pos.y == 1)
            || (g_party.field.pos.area == MAP_AREA_ASAKUSA_SUBWAY_BUILDING
                && g_party.field.pos.level == 0 && g_party.field.pos.x == 1
                && g_party.field.pos.y == 4)
            || (g_party.field.pos.area == MAP_AREA_AMEYA_PLAZA && g_party.field.pos.level == 0
                && g_party.field.pos.x == 5 && g_party.field.pos.y == 9)
            || (g_party.field.pos.area == MAP_AREA_SHIBUYA && g_party.field.pos.level == 2
                && g_party.field.pos.x == 9 && g_party.field.pos.y == 0xd)) {
            g_shopKind = 7;
            // 臨海コロシアム以外の武器屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\227\325\212C\203R\203\215\203V\203A\203\200\210\310\212O\202\314\225\220\212"
                "\355\211\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265"
                "\202\304\211\272\202\263\202\242\201BTakubo\n"
            );
        } else if ((g_party.field.pos.area == MAP_AREA_SHINJUKU_UNDERGROUND
                    && g_party.field.pos.level == 1 && g_party.field.pos.x == 7
                    && g_party.field.pos.y == 0x14)
                   || (g_party.field.pos.area == MAP_AREA_MY_CITY && g_party.field.pos.level == 5
                       && g_party.field.pos.x == 3 && g_party.field.pos.y == 7)
                   || (g_party.field.pos.area == MAP_AREA_SHANSHAN_CITY
                       && g_party.field.pos.level == 0 && g_party.field.pos.x == 0xf
                       && g_party.field.pos.y == 5)
                   || (g_party.field.pos.area == MAP_AREA_KANDA_UNDERGROUND
                       && g_party.field.pos.level == 5 && g_party.field.pos.x == 0
                       && g_party.field.pos.y == 9)
                   || (g_party.field.pos.area == MAP_AREA_OCHANOMIZU && g_party.field.pos.level == 4
                       && g_party.field.pos.x == 4 && g_party.field.pos.y == 4)
                   || (g_party.field.pos.area == MAP_AREA_AKIHABARA_STATION_BUILDING
                       && g_party.field.pos.level == 1 && g_party.field.pos.x == 9
                       && g_party.field.pos.y == 4)
                   || (g_party.field.pos.area == MAP_AREA_GINZA_UNDERGROUND
                       && g_party.field.pos.level == 2 && g_party.field.pos.x == 9
                       && g_party.field.pos.y == 3)
                   || (g_party.field.pos.area == MAP_AREA_ROPPONGI && g_party.field.pos.level == 0
                       && g_party.field.pos.x == 7 && g_party.field.pos.y == 1)
                   || (g_party.field.pos.area == MAP_AREA_ASAKUSA_SUBWAY_BUILDING
                       && g_party.field.pos.level == 1 && g_party.field.pos.x == 6
                       && g_party.field.pos.y == 6)
                   || (g_party.field.pos.area == MAP_AREA_AMEYA_PLAZA
                       && g_party.field.pos.level == 0 && g_party.field.pos.x == 7
                       && g_party.field.pos.y == 0xd)
                   || (g_party.field.pos.area == MAP_AREA_SHIBUYA && g_party.field.pos.level == 2
                       && g_party.field.pos.x == 9 && g_party.field.pos.y == 0xb)) {
            g_shopKind = 8;
            // 臨海コロシアム・六本木以外の防具屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\227\325\212C\203R\203\215\203V\203A\203\200\201E\230Z\226{"
                "\226\330\210\310\212O\202\314\226h\213\357\211\256\202\305\202\310\202\242\217\352"
                "\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242\201BTakubo\n"
            );
        } else if ((g_party.field.pos.area == MAP_AREA_SHINJUKU_UNDERGROUND
                    && g_party.field.pos.level == 1 && g_party.field.pos.x == 4
                    && g_party.field.pos.y == 0xf)
                   || (g_party.field.pos.area == MAP_AREA_MY_CITY && g_party.field.pos.level == 0
                       && g_party.field.pos.x == 0 && g_party.field.pos.y == 4)
                   || (g_party.field.pos.area == MAP_AREA_SHANSHAN_CITY
                       && g_party.field.pos.level == 0 && g_party.field.pos.x == 0x11
                       && g_party.field.pos.y == 1)
                   || (g_party.field.pos.area == MAP_AREA_KANDA_UNDERGROUND
                       && g_party.field.pos.level == 5 && g_party.field.pos.x == 0
                       && g_party.field.pos.y == 7)
                   || (g_party.field.pos.area == MAP_AREA_OCHANOMIZU && g_party.field.pos.level == 5
                       && g_party.field.pos.x == 5 && g_party.field.pos.y == 5)
                   || (g_party.field.pos.area == MAP_AREA_AKIHABARA_STATION_BUILDING
                       && g_party.field.pos.level == 2 && g_party.field.pos.x == 0xc
                       && g_party.field.pos.y == 4)
                   || (g_party.field.pos.area == MAP_AREA_GINZA_UNDERGROUND
                       && g_party.field.pos.level == 2 && g_party.field.pos.x == 8
                       && g_party.field.pos.y == 9)
                   || (g_party.field.pos.area == MAP_AREA_EBISU_GARDEN
                       && g_party.field.pos.level == 0 && g_party.field.pos.x == 0xe
                       && g_party.field.pos.y == 6)
                   || (g_party.field.pos.area == MAP_AREA_ROPPONGI && g_party.field.pos.level == 0
                       && g_party.field.pos.x == 0xd && g_party.field.pos.y == 1)
                   || (g_party.field.pos.area == MAP_AREA_ASAKUSA_SUBWAY_BUILDING
                       && g_party.field.pos.level == 0 && g_party.field.pos.x == 6
                       && g_party.field.pos.y == 5)
                   || (g_party.field.pos.area == MAP_AREA_ASAKUSA_SUBWAY_BUILDING
                       && g_party.field.pos.level == 3 && g_party.field.pos.x == 6
                       && g_party.field.pos.y == 6)
                   || (g_party.field.pos.area == MAP_AREA_AMEYA_PLAZA
                       && g_party.field.pos.level == 0 && g_party.field.pos.x == 2
                       && g_party.field.pos.y == 0xd)
                   || (g_party.field.pos.area == MAP_AREA_RESISTANCE_FRONTLINE_BASE
                       && g_party.field.pos.level == 3 && g_party.field.pos.x == 0xd
                       && g_party.field.pos.y == 8)) {
            g_shopKind = 5;
            // 初台以外の道具屋＆レジスタンス前線基地の薬屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\217\211\221\344\210\310\212O\202\314\223\271\213\357\211\256\201\225\203\214"
                "\203W\203X\203^"
                "\203\223\203X\221O\220\374\212\356\222n\202\314\226\362\211\256\202\305\202\310"
                "\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202"
                "\242\201BTakubo\n"
            );
        } else if ((g_party.field.pos.area == MAP_AREA_SHINJUKU_UNDERGROUND
                    && g_party.field.pos.level == 1 && g_party.field.pos.x == 4
                    && g_party.field.pos.y == 0x11)
                   || (g_party.field.pos.area == MAP_AREA_KANDA_UNDERGROUND
                       && g_party.field.pos.level == 5 && g_party.field.pos.x == 5
                       && g_party.field.pos.y == 4)
                   || (g_party.field.pos.area == MAP_AREA_EBISU_GARDEN
                       && g_party.field.pos.level == 0 && g_party.field.pos.x == 6
                       && g_party.field.pos.y == 5)
                   || (g_party.field.pos.area == MAP_AREA_ASAKUSA_SUBWAY_BUILDING
                       && g_party.field.pos.level == 0 && g_party.field.pos.x == 5
                       && g_party.field.pos.y == 7)) {
            g_shopKind = 7;
            // 新宿地下街・神田地下街・恵比寿ガーデン・浅草地下鉄ビルの薬屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\220V\217h\222n\211\272\212X\201E\220_"
                "\223c\222n\211\272\212X\201E\214b\224\344\216\365\203K\201["
                "\203f\203\223\201E\220\363\221\220\222n\211\272\223S\203r\203\213\202\314\226\362"
                "\211\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202"
                "\304\211\272\202\263\202\242\201BTakubo\n"
            );
        }
    } else if (image == 0x7f) {
        if (g_party.field.pos.area == MAP_AREA_RINKAI_COLISEUM && g_party.field.pos.level == 1
            && g_party.field.pos.x == 1 && g_party.field.pos.y == 5) {
            g_shopKind = 5;
            // 臨海コロシアムの道具屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\227\325\212C\203R\203\215\203V\203A\203\200\202\314\223\271\213\357\211\256\202"
                "\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272"
                "\202\263\202\242\201BTakubo\n"
            );
        } else if (g_party.field.pos.area == MAP_AREA_RINKAI_COLISEUM
                   && g_party.field.pos.level == 1 && g_party.field.pos.x == 1
                   && g_party.field.pos.y == 9) {
            g_shopKind = 5;
            // 臨海コロシアムの武器屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\227\325\212C\203R\203\215\203V\203A\203\200\202\314\225\220\212\355\211\256\202"
                "\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272"
                "\202\263\202\242\201BTakubo\n"
            );
        } else if (g_party.field.pos.area == MAP_AREA_RINKAI_COLISEUM
                   && g_party.field.pos.level == 1 && g_party.field.pos.x == 6
                   && g_party.field.pos.y == 1) {
            g_shopKind = 6;
            // 臨海コロシアムの防具屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\227\325\212C\203R\203\215\203V\203A\203\200\202\314\226h\213\357\211\256\202"
                "\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272"
                "\202\263\202\242\201BTakubo\n"
            );
        }
    } else if (image == 0xc7) {
        if (g_party.field.pos.area == MAP_AREA_ENTERTAINMENT_DISTRICT
            && g_party.field.pos.level == 0 && g_party.field.pos.x == 6
            && g_party.field.pos.y == 1) {
            g_shopKind = 5;
            // 大歓楽街の酒場でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\221\345\212\275\212y\212X\202\314\216\360\217\352\202\305\202\310\202\242\217"
                "\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242\201BTaku"
                "bo\n"
            );
        } else if (g_party.field.pos.area == MAP_AREA_ROPPONGI && g_party.field.pos.level == 0
                   && g_party.field.pos.x == 7 && g_party.field.pos.y == 1) {
            g_shopKind = 6;
            // 六本木の防具屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\n\230Z\226{"
                "\226\330\202\314\226h\213\357\211\256\202\305\202\310\202\242\217\352\215\207\202"
                "\315\230A\227\215\202\265\202\304\211\272\202\263\202\242\201BTakubo\n"
            );
        }
    } else if (image == -0x1d9e && arg == 3 && g_party.field.pos.area == MAP_AREA_OHANAYASHIKI
               && g_party.field.pos.x == 5 && g_party.field.pos.y == 0xb) {
        image = -0x176e;
        arg = 0;
        // 御花屋敷のオサキ狐イベントでない場合は連絡して下さい。Takubo
        DebugTrace(
            "\n\214\344\211\324\211\256\225~"
            "\202\314\203I\203T\203L\214\317\203C\203x\203\223\203g\202\305\202\310\202\242\217\352"
            "\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242\201BTakubo\n"
        );
    }

    LoadSpriteImage(slot, image, arg);
    return false;
}

RVA(0x0003a250, 0xf8)
b16 OpPlaceSprite(i16 variant) {
    i16 slot = ReadScriptValue();
    i16 frame = ReadScriptValue();
    i16 x = ReadScriptValue();
    i16 y = ReadScriptValue();
    i16 image = ReadScriptValue();
    switch (g_shopKind) {
        case 1:
            image = slot = 1;
            g_shopKind = 0;
            break;
        case 2:
            image = slot = 0;
            g_shopKind = 0;
            break;
        case 3:
            if (slot == 1 && image == 1 && x == 40 && y == 235) {
                y = 190;
            }
            g_shopKind = 0;
            break;
        case 4:
            x = 40;
            y = 240;
            g_shopKind = 0;
            break;
        case 5:
            x = 40;
            y = 213;
            g_shopKind = 0;
            break;
        case 6:
            x = 40;
            y = 225;
            g_shopKind = 0;
            break;
        case 7:
            x = 40;
            y = 218;
            g_shopKind = 0;
            break;
        case 8:
            x = 40;
            y = 230;
            g_shopKind = 0;
            break;
        case 9:
            x = 40;
            y = 212;
            g_shopKind = 0;
            break;
    }
    PlaceSprite(image, slot, frame, x, y);
    return false;
}
