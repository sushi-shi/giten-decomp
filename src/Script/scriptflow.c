// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/GameLoop.h>
#include <Game/GameState.h>
#include <Game/Party.h>
#include <Gfx/Sprite.h>
#include <Script/LongVar.h>
#include <Script/Script.h>
#include <Script/ScriptBlock.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptVars.h>
#include <Sound/Sound.h>
#include <Util/Debug.h>
#include <Util/Range.h>

#include <math.h>
#include <stdlib.h>

DATA(0x000911b0)
i16 g_scriptArgA;

DATA(0x000911b2)
i16 g_scriptArgB;

// The long-variable accumulator, the second operand and the variable index.
DATA(0x0008134c)
static i32 s_longAcc;

DATA(0x00081350)
static i32 s_longOperand;

DATA(0x00081354)
static i16 s_longVarIndex;

// The byte before the index is a kind byte the long ops ignore.
RVA(0x000335a0, 0x15)
i16 ReadLongVarIndex(void) {
    ReadScriptByte();
    s_longVarIndex = (i8)ReadScriptByte();
    return s_longVarIndex;
}

RVA(0x000335c0, 0x1d)
i32 ReadLongOperand(i16 inPlace) {
    if (inPlace == 0) {
        return ReadScriptValue();
    }
    return GetScriptLongVar(s_longVarIndex);
}

RVA(0x000335e0, 0x22)
void LoadLongOperands(i16 inPlace) {
    ReadLongVarIndex();
    s_longAcc = ReadLongOperand(inPlace);
    s_longOperand = ReadScriptValue();
}

RVA(0x00033610, 0x1c)
i32 StoreLongResult(void) {
    SetScriptLongVar(s_longVarIndex, s_longAcc);
    return s_longAcc;
}

RVA(0x00033630, 0x25)
void OpMulLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc *= s_longOperand;
    StoreLongResult();
}

RVA(0x00033660, 0x34)
void OpDivLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    if (s_longOperand == 0) {
        s_longOperand = 1;
    }
    s_longAcc /= s_longOperand;
    StoreLongResult();
}

RVA(0x000336a0, 0x24)
void OpAddLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc += s_longOperand;
    StoreLongResult();
}

RVA(0x000336d0, 0x24)
void OpSubLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc -= s_longOperand;
    StoreLongResult();
}

RVA(0x00033700, 0x24)
void OpAndLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc &= s_longOperand;
    StoreLongResult();
}

RVA(0x00033730, 0x24)
void OpOrLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc |= s_longOperand;
    StoreLongResult();
}

RVA(0x00033760, 0x24)
void OpXorLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc ^= s_longOperand;
    StoreLongResult();
}

RVA(0x00033790, 0x24)
void OpShlLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc <<= s_longOperand;
    StoreLongResult();
}

RVA(0x000337c0, 0x24)
void OpSarLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc >>= s_longOperand;
    StoreLongResult();
}

// The accumulator as a percentage of the operand.
RVA(0x000337f0, 0x3d)
void OpPercentLongVar(i16 inPlace) {
    i32 scaled;
    LoadLongOperands(inPlace);
    scaled = s_longAcc * 100;
    if (s_longOperand == 0) {
        s_longOperand = 1;
    }
    s_longAcc = scaled / s_longOperand;
    StoreLongResult();
}

RVA(0x00033830, 0x2b)
void OpSqrtLongVar(i16 inPlace) {
    ReadLongVarIndex();
    s_longAcc = (i32)sqrt(ReadLongOperand(inPlace));
    StoreLongResult();
}

RVA(0x00033860, 0x24)
void OpModLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc %= s_longOperand;
    StoreLongResult();
}

RVA(0x00033890, 0x1a)
void LoadLongVar(void) {
    ReadLongVarIndex();
    s_longAcc = GetScriptLongVar(s_longVarIndex);
}

RVA(0x000338b0, 0x17)
void StoreLongVar(void) {
    SetScriptLongVar(s_longVarIndex, s_longAcc);
}

// Sets a variable to a value. Setting variable 23 to 15 while slot 0 is empty
// and track 7 plays shows picture 0x73 in slot 31.
RVA(0x000338d0, 0x6d)
void OpSetLongVar(void) {
    ReadLongVarIndex();
    s_longAcc = ReadScriptValue();
    if (s_longVarIndex == 23 && s_longAcc == 15 && !IsSpritePlaced(0) && CurrentMusicTrack() == 7) {
        LoadSpriteImage(31, 0x73, 0);
        PlaceSprite(31, 31, 0, 40, 240);
        // 邪教のところ以外このメッセージをみたらお知らせください
        DebugTrace(
            "\n\216\327\213\263\202\314\202\306\202\261\202\353\210\310\212O\202\261\202\314\203"
            "\201\203b\203Z\201["
            "\203W\202\360\202\335\202\275\202\347\202\250\222m\202\347\202\271\202\255\202\276\202"
            "\263\202\242 S.Iseki \n"
        );
    }
    StoreLongVar();
}

RVA(0x00033940, 0x1f)
void OpSwapLongVars(void) {
    i16 first = ReadLongVarIndex();
    ReadLongVarIndex();
    SwapScriptLongVars(first, s_longVarIndex);
}

RVA(0x00033960, 0x1f)
void OpCopyLongVar(void) {
    i16 dst = ReadLongVarIndex();
    ReadLongVarIndex();
    CopyScriptLongVar(dst, s_longVarIndex);
}

RVA(0x00033980, 0x15)
void OpUnsetLongVar(void) {
    ReadLongVarIndex();
    ClearScriptLongVar(s_longVarIndex);
}

RVA(0x000339a0, 0x17)
void OpZeroLongVar(void) {
    ReadLongVarIndex();
    SetScriptLongVar(s_longVarIndex, 0);
}

RVA(0x000339c0, 0x16)
void OpNegLongVar(void) {
    LoadLongVar();
    s_longAcc = -s_longAcc;
    StoreLongVar();
}

RVA(0x000339e0, 0x16)
void OpNotLongVar(void) {
    LoadLongVar();
    s_longAcc = ~s_longAcc;
    StoreLongVar();
}

RVA(0x00033a00, 0x10)
void OpIncLongVar(void) {
    LoadLongVar();
    s_longAcc++;
    StoreLongVar();
}

RVA(0x00033a10, 0x10)
void OpDecLongVar(void) {
    LoadLongVar();
    s_longAcc--;
    StoreLongVar();
}

RVA(0x00033a20, 0x26)
void OpIncLongVarBelow(void) {
    LoadLongVar();
    s_longOperand = ReadScriptValue();
    if (s_longAcc < s_longOperand) {
        s_longAcc++;
    }
    StoreLongVar();
}

RVA(0x00033a50, 0x26)
void OpDecLongVarAbove(void) {
    LoadLongVar();
    s_longOperand = ReadScriptValue();
    if (s_longAcc > s_longOperand) {
        s_longAcc--;
    }
    StoreLongVar();
}

RVA(0x00033a80, 0x3c)
void OpClampLongVar(void) {
    LoadLongVar();
    s_longOperand = ReadScriptValue();
    if (s_longAcc < s_longOperand) {
        s_longAcc = s_longOperand;
    }
    s_longOperand = ReadScriptValue();
    if (s_longAcc > s_longOperand) {
        s_longAcc = s_longOperand;
    }
    StoreLongVar();
}

RVA(0x00033ac0, 0x35)
void OpRollLongVar(void) {
    i32 lo;
    i32 hi;
    ReadLongVarIndex();
    lo = ReadScriptValue();
    hi = ReadScriptValue();
    s_longAcc = RandomAverage(lo, hi, ReadScriptValue());
    StoreLongVar();
}

RVA(0x00033b00, 0x14)
void OpRandLongVar(void) {
    ReadLongVarIndex();
    s_longAcc = rand();
    StoreLongVar();
}

// Saves the system variables into the newest call frame.
RVA(0x00033b20, 0x26)
b16 StoreFrameLocals(void) {
    i32 frame = TopCallFrame(g_curScript);
    if (!frame) {
        return false;
    }
    TransferFrameVars(frame, 0);
    return true;
}

// Restores the system variables from the newest call frame.
RVA(0x00033b50, 0x26)
b16 LoadFrameLocals(void) {
    i32 frame = TopCallFrame(g_curScript);
    if (!frame) {
        return false;
    }
    TransferFrameVars(frame, 1);
    return true;
}

// Exchanges the system variables with the newest call frame's, through two
// scratch frames.
RVA(0x00033b80, 0x86)
b16 SwapFrameLocals(void) {
    i32 frame = TopCallFrame(g_curScript);
    i32 saved;
    i32 loaded;
    if (!frame) {
        return false;
    }
    saved = NewCallFrame();
    loaded = NewCallFrame();
    TransferFrameVars(saved, 0);
    TransferFrameVars(frame, 1);
    TransferFrameVars(loaded, 0);
    TransferFrameVars(saved, 1);
    TransferFrameVars(frame, 0);
    TransferFrameVars(loaded, 1);
    FreeCallFrames(loaded);
    FreeCallFrames(saved);
    return true;
}

// Reads two byte operands into words.
RVA(0x00033c10, 0x22)
void ReadScriptBytePair(i16* first, i16* second) {
    *first = ReadScriptByte();
    *second = ReadScriptByte();
}

RVA(0x00033c40, 0x10)
void ScriptJump(i16 pc) {
    g_curScript->pc = pc;
}

// Continues the script at `pc` of the code block `codeBase`.
RVA(0x00033c50, 0x1e)
b16 ScriptJumpTo(u32 codeBase, i16 pc) {
    g_curScript->codeBase = codeBase;
    ScriptJump(pc);
    return false;
}

// Continues the script at `pc` of `codeBase`, passing the two call arguments.
RVA(0x00033c70, 0x2a)
b16 ScriptJumpWithArgs(u32 codeBase, i16 pc, i16 argA, i16 argB) {
    g_scriptArgA = argA;
    g_scriptArgB = argB;
    return ScriptJumpTo(codeBase, pc);
}

// Where `entry` of script file `file` starts: files 0xe0..0xff are the
// current actor's field-layer scripts; any other file is taken from the cache,
// loading it on a miss.
RVA(0x00033ca0, 0xca)
ScriptEntry ResolveScriptEntry(i16 file, i16 entry) {
    ScriptEntry result;
    if (file >= 0xe0 && file <= 0xff) {
        i16 layer = FindLayerOfKind(g_curScript->actor->id);
        return FindLayerScriptEntry(layer + 1, file, entry);
    }
    result = FindCachedScriptEntry(file, entry);
    if (result.code) {
        return result;
    }
    return MakeScriptEntry(CacheScriptFile(file), entry);
}

// Continues the current script at `entry` of script file `file`, passing both
// as the call arguments.
// @identity-TODO: the three special cases (a redirected entry, a sprite reset
// and a picture shown at one map place) are unexplained.
RVA(0x00033d70, 0xc1)
void GotoScript(i16 file, i16 entry) {
    ScriptEntry target;
    if (file == 0xde && entry == 1) {
        entry = 9;
    } else if (file == 0x2c && entry == 4) {
        ResetSprites(SPRITE_LAYERS_PARTY_AND_TEXT);
    } else if (file == 0x5b && entry == 0x22 && g_field.pos.x == 1 && g_field.pos.y == 4
               && g_field.pos.level == 1 && g_field.pos.area == 0x83) {
        LoadSpriteImage(0, 0x42, 0);
        PlaceSprite(0, 0, 0, 40, 240);
    }
    target = ResolveScriptEntry(file, entry);
    ScriptJumpWithArgs(target.code, target.pc, file, entry);
}

// Calls `entry` of script file `file`, returning to the current position.
RVA(0x00033e40, 0x23)
void CallScript(i16 file, i16 entry) {
    PushCallFrame(g_curScript, 0);
    GotoScript(file, entry);
}

// Reads a script file and entry and jumps (or with `call`, calls) there.
RVA(0x00033e70, 0x49)
void OpJumpScript(i16 call) {
    i16 file;
    i16 entry;
    ReadScriptBytePair(&file, &entry);
    if (!call) {
        GotoScript(file, entry);
    } else {
        CallScript(file, entry);
    }
}

// Reads a jump offset; returns the position it names.
RVA(0x00033ec0, 0x13)
i16 ReadJumpTarget(void) {
    u16 offset = ReadScriptWord();
    return g_curScript->pc + offset;
}

RVA(0x00033ee0, 0xf)
void OpJump(void) {
    ScriptJump(ReadJumpTarget());
}

// Jumps to `pc` when `condition` is zero; passes `condition` through.
RVA(0x00033ef0, 0x1a)
i32 ScriptJumpUnless(i16 pc, i32 condition) {
    if (!condition) {
        ScriptJump(pc);
    }
    return condition;
}

// Pushes a call frame whose return position is the jump target read from the
// script, and continues after the operand.
RVA(0x00033f10, 0x38)
b16 OpPushReturnTarget(void) {
    i16 target = ReadJumpTarget();
    i16 next = g_curScript->pc;
    ScriptJump(target);
    PushCallFrame(g_curScript, 1);
    ScriptJump(next);
    return false;
}

RVA(0x00033f50, 0x14)
b16 DropCallFrame(void) {
    PopCallFrame(g_curScript, 1);
    return false;
}

RVA(0x00033f70, 0x12)
b16 SwapCallFrames(void) {
    SwapTopCallFrames(g_curScript);
    return false;
}

RVA(0x00033f90, 0x12)
b16 ClearCallStack(void) {
    UnwindCallFrames(g_curScript);
    return false;
}

// Returns from the current call; -1 when that ends the script.
RVA(0x00033fb0, 0x21)
i16 ReturnFromCall(void) {
    PopCallFrame(g_curScript, 0);
    if (g_curScript->codeBase) {
        return 0;
    }
    return -1;
}

// Discards every call frame and continues at `entry` of script file `file`.
RVA(0x00033fe0, 0x18)
void RestartScript(i16 file, i16 entry) {
    ClearCallStack();
    GotoScript(file, entry);
}

// Loads script file `file` entry `entry` (skipping files 0xe0..0xff); a shop
// entry of file 0x7f first shows its keeper, and traces which shop it should
// be.
RVA(0x00034000, 0x599)
void OpLoadRecord(void) {
    i16 file = ReadScriptValue() & 0xff;
    i16 entry = ReadScriptValue();

    if (file >= 0xe0 && file <= 0xff) {
        return;
    }
    if (file == 0x7f && entry >= 0xb0) {
        if (entry < 0xb7) {
            LoadSpriteImage(0x1f, 0x74, 0);
            PlaceSprite(0x1f, 0x1f, 0, 0x28, 0xf0);
            // マイシティ・シャンシャンシティ２・神田地下街・御茶ノ水・秋葉原・銀座地下街・恵比寿ガーデンプレイスの酒場でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\203}"
                "\203C\203V\203e\203B\201E\203V\203\203\203\223\203V\203\203\203\223\203V\203e\203B"
                "\202Q\201E\220_"
                "\223c\222n\211\272\212X\201E\214\344\222\203\203m\220\205\201E\217H\227t\214\264"
                "\201E\213\342\215\300\222n\211\272\212X\201E\214b\224\344\216\365\203K\201["
                "\203f\203\223\203v\203\214\203C\203X\202\314\216\360\217\352\202\305\202\310\202"
                "\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242"
                "\201BTakubo\012"
            );
        } else if (entry == 0xb7) {
            LoadSpriteImage(0x1f, 0xc7, 0);
            PlaceSprite(0x1f, 0x1f, 0, 0x28, 0xd5);
            // 大歓楽街の酒場でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\221\345\212\275\212y\212X\202\314\216\360\217\352\202\305\202\310\202\242\217"
                "\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242\201BTaku"
                "bo\012"
            );
        } else if (entry == 0xb8) {
            LoadSpriteImage(0x1f, 0x56, 0);
            PlaceSprite(0x1f, 0x1f, 0, 0x28, 0xf0);
            // 六本木の酒場でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\230Z\226{"
                "\226\330\202\314\216\360\217\352\202\305\202\310\202\242\217\352\215\207\202\315"
                "\230A\227\215\202\265\202\304\211\272\202\263\202\242\201BTakubo\012"
            );
        } else if (entry == 0x100 || (entry == 0x101 && g_field.pos.area == 0x83)) {
            LoadSpriteImage(0x1f, 0x79, 0);
            PlaceSprite(0x1f, 0x1f, 0, 0x28, 0xd3);
            // 初台以外の道具屋＆レジスタンス前線基地の薬屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\217\211\221\344\210\310\212O\202\314\223\271\213\357\211\256\201\225\203\214"
                "\203W\203X\203^"
                "\203\223\203X\221O\220\374\212\356\222n\202\314\226\362\211\256\202\305\202\310"
                "\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202"
                "\242\201BTakubo\012"
            );
        } else if (entry == 0x101 && g_field.pos.area == 0x2e && g_field.pos.x == 7) {
            LoadSpriteImage(0x1f, 0xc7, 1);
            PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xe1);
            // 六本木の防具屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\230Z\226{"
                "\226\330\202\314\226h\213\357\211\256\202\305\202\310\202\242\217\352\215\207\202"
                "\315\230A\227\215\202\265\202\304\211\272\202\263\202\242\201BTakubo\012"
            );
        } else if (entry == 0x101) {
            LoadSpriteImage(0x1f, 0x79, 1);
            if ((g_field.pos.area == 0x06 && g_field.pos.y == 0x12)
                || (g_field.pos.area == 0x0a && g_field.pos.y == 0x0d)
                || (g_field.pos.area == 0x13 && g_field.pos.y == 0x01)
                || (g_field.pos.area == 0x1b && g_field.pos.y == 0x05)
                || (g_field.pos.area == 0x8a && g_field.pos.y == 0x02)
                || (g_field.pos.area == 0x1f && g_field.pos.level == 2)
                || (g_field.pos.area == 0x1a && g_field.pos.x == 0x0a)
                || (g_field.pos.area == 0x34 && g_field.pos.level == 0) || g_field.pos.area == 0x2e
                || (g_field.pos.area == 0x25 && g_field.pos.level == 0)
                || (g_field.pos.area == 0x21 && g_field.pos.y == 0x09)
                || (g_field.pos.area == 0x30 && g_field.pos.y == 0x0b)) {
                PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xda);
                // 臨海コロシアム以外の武器屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\227\325\212C\203R\203\215\203V\203A\203\200\210\310\212O\202\314\225\220"
                    "\212\355\211\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215"
                    "\202\265\202\304\211\272\202\263\202\242\201BTakubo\012"
                );
            } else {
                PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xe6);
                // 臨海コロシアム・六本木以外の防具屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\227\325\212C\203R\203\215\203V\203A\203\200\201E\230Z\226{"
                    "\226\330\210\310\212O\202\314\226h\213\357\211\256\202\305\202\310\202\242\217"
                    "\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242\201B"
                    "Takubo\012"
                );
            }
        } else if (entry == 0x102 || entry == 0x104) {
            if (g_field.pos.area == 0x1a && g_field.pos.level == 3) {
                LoadSpriteImage(0x1f, 0x4f, 1);
                PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xd4);
                // 銀座地下街秘密区の薬屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\213\342\215\300\222n\211\272\212X\224\351\226\247\213\346\202\314\226\362"
                    "\211\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265"
                    "\202\304\211\272\202\263\202\242\201BTakubo\012"
                );
            } else if (g_field.pos.area == 0x06 || g_field.pos.area == 0x1b
                       || g_field.pos.area == 0x34 || g_field.pos.area == 0x25) {
                LoadSpriteImage(0x1f, 0x79, 2);
                PlaceSprite(0x1f, 0x1f, 2, 0x28, 0xda);
                // 新宿地下街・神田地下街・恵比寿ガーデン・浅草地下鉄ビルの薬屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\220V\217h\222n\211\272\212X\201E\220_"
                    "\223c\222n\211\272\212X\201E\214b\224\344\216\365\203K\201["
                    "\203f\203\223\201E\220\363\221\220\222n\211\272\223S\203r\203\213\202\314\226"
                    "\362\211\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202"
                    "\265\202\304\211\272\202\263\202\242\201BTakubo\012"
                );
            } else if (g_field.pos.area == 0x56) {
                LoadSpriteImage(0x1f, 0x2b, 3);
                PlaceSprite(0x1f, 0x1f, 3, 0x28, 0xd4);
                // 臨海コロシアムの薬屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\227\325\212C\203R\203\215\203V\203A\203\200\202\314\226\362\211\256\202"
                    "\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211"
                    "\272\202\263\202\242\201BTakubo\012"
                );
            } else {
                LoadSpriteImage(0x1f, 0x2b, 2);
                PlaceSprite(0x1f, 0x1f, 2, 0x28, 0xd4);
                // レジスタンス前線基地・新宿地下街・神田地下街・銀座地下街秘密区・恵比寿ガーデン・浅草地下街・臨海コロシアム以外の薬屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\203\214\203W\203X\203^"
                    "\203\223\203X\221O\220\374\212\356\222n\201E\220V\217h\222n\211\272\212X\201E"
                    "\220_"
                    "\223c\222n\211\272\212X\201E\213\342\215\300\222n\211\272\212X\224\351\226\247"
                    "\213\346\201E\214b\224\344\216\365\203K\201["
                    "\203f\203\223\201E\220\363\221\220\222n\211\272\212X\201E\227\325\212C\203R"
                    "\203\215\203V\203A\203\200\210\310\212O\202\314\226\362\211\256\202\305\202"
                    "\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202"
                    "\263\202\242\201BTakubo\012"
                );
            }
        } else if (entry == 0x103) {
            LoadSpriteImage(0x1f, 0x25, 3);
            PlaceSprite(0x1f, 0x1f, 3, 0x28, 0xd5);
            // 初台の道具屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\217\211\221\344\202\314\223\271\213\357\211\256\202\305\202\310\202\242\217"
                "\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242\201BTaku"
                "bo\012"
            );
        } else if (entry == 0x10a || entry == 0x10b) {
            LoadSpriteImage(0x1f, 0x2c, 4);
            PlaceSprite(0x1f, 0x1f, 4, 0x28, 0xd5);
            // コンピュータショップでない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\203R\203\223\203s\203\205\201[\203^"
                "\203V\203\207\203b\203v\202\305\202\310\202\242\217\352\215\207\202\315\230A\227"
                "\215\202\265\202\304\211\272\202\263\202\242\201BTakubo\012"
            );
        } else if (entry == 0x105) {
            if (g_field.pos.area == 0x21) {
                LoadSpriteImage(0x1f, 0x79, 2);
                PlaceSprite(0x1f, 0x1f, 2, 0x28, 0xda);
                // アメ屋プラザ２階の薬屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\203A\203\201\211\256\203v\203\211\203U\202Q\212K\202\314\226\362\211\256"
                    "\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304"
                    "\211\272\202\263\202\242\201BTakubo\012"
                );
            } else {
                LoadSpriteImage(0x1f, 0xb7, 1);
                PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xda);
                // 大歓楽街２Ｆの薬屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\221\345\212\275\212y\212X\202Q\202e\202\314\226\362\211\256\202\305\202"
                    "\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202"
                    "\263\202\242\201BTakubo\012"
                );
            }
        } else if (entry == 0x106) {
            if (g_field.pos.x == 1 && g_field.pos.y == 5) {
                LoadSpriteImage(0x1f, 0x7f, 3);
                PlaceSprite(0x1f, 0x1f, 3, 0x28, 0xd5);
                // 臨海コロシアムの道具屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\227\325\212C\203R\203\215\203V\203A\203\200\202\314\223\271\213\357\211"
                    "\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202"
                    "\304\211\272\202\263\202\242\201BTakubo\012"
                );
            } else {
                LoadSpriteImage(0x1f, 0x7f, 2);
                if (g_field.pos.y == 9) {
                    PlaceSprite(0x1f, 0x1f, 2, 0x28, 0xd5);
                    // 臨海コロシアムの武器屋でない場合は連絡して下さい。Takubo
                    DebugTrace(
                        "\012\227\325\212C\203R\203\215\203V\203A\203\200\202\314\225\220\212\355"
                        "\211\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202"
                        "\265\202\304\211\272\202\263\202\242\201BTakubo\012"
                    );
                } else {
                    PlaceSprite(0x1f, 0x1f, 2, 0x28, 0xe1);
                    // 臨海コロシアムの防具屋でない場合は連絡して下さい。Takubo
                    DebugTrace(
                        "\012\227\325\212C\203R\203\215\203V\203A\203\200\202\314\226h\213\357\211"
                        "\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265"
                        "\202\304\211\272\202\263\202\242\201BTakubo\012"
                    );
                }
            }
        } else if (entry == 0x110 || entry == 0x111) {
            LoadSpriteImage(0x1f, 0x2b, 2);
            PlaceSprite(0x1f, 0x1f, 2, 0x28, 0xd4);
            // 初台・マイシティ・御茶ノ水・ミレニアム病院の病院でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\217\211\221\344\201E\203}"
                "\203C\203V\203e\203B\201E\214\344\222\203\203m\220\205\201E\203~"
                "\203\214\203j\203A\203\200\225a\211@\202\314\225a\211@"
                "\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211"
                "\272\202\263\202\242\201BTakubo\012"
            );
        } else if (entry == 0x115) {
            if (g_field.pos.area == 0x21) {
                LoadSpriteImage(0x1f, 0x2c, 1);
                PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xf0);
                // アメ屋プラザの病院でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\203A\203\201\211\256\203v\203\211\203U\202\314\225a\211@"
                    "\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304"
                    "\211\272\202\263\202\242\201BTakubo\012"
                );
            } else if (g_field.pos.area == 0x56) {
                LoadSpriteImage(0x1f, 0x76, 1);
                PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xf0);
                // 臨海コロシアムの病院でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\227\325\212C\203R\203\215\203V\203A\203\200\202\314\225a\211@"
                    "\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304"
                    "\211\272\202\263\202\242\201BTakubo\012"
                );
            } else {
                LoadSpriteImage(0x1f, 0x78, 0);
                PlaceSprite(0x1f, 0x1f, 0, 0x28, 0xf0);
                // 神田地下街・銀座地下街の病院でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\220_"
                    "\223c\222n\211\272\212X\201E\213\342\215\300\222n\211\272\212X\202\314\225a"
                    "\211@"
                    "\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304"
                    "\211\272\202\263\202\242\201BTakubo\012"
                );
            }
        }
    }
    LoadCachedScriptFile(file, entry);
}

RVA(0x000345a0, 0x10)
void RequestQuit(void) {
    g_quitRequest = 1;
}

RVA(0x000345b0, 0x2c)
void OpChangeHp(i16 sign) {
    Character* character = ReadScriptObject();
    ChangePool(&character->pools.hp, ReadScriptValue() * sign);
    RequestFieldRefresh();
}

RVA(0x000345e0, 0x2c)
void OpChangeMp(i16 sign) {
    Character* character = ReadScriptObject();
    ChangePool(&character->pools.mp, ReadScriptValue() * sign);
    RequestFieldRefresh();
}

// Retail uses the MP pair as the input even when writing the HP result.
RVA(0x00034610, 0x6b)
void OpBoostPool(void) {
    Character* character = ReadScriptObject();
    i16 which = ReadScriptValue();
    i16 amount = ReadScriptValue();
    i16 mode = ReadScriptValue();
    i16 limit;
    i16 current;
    if (which == 0) {
        limit = character->pools.mp.max;
        current = character->pools.mp.cur;
    } else {
        limit = character->pools.mp.max;
        current = character->pools.mp.cur;
    }
    if (mode == 1) {
        limit *= 2;
    } else if (mode == 2) {
        limit = 0x7fff;
    }
    current = AddClampShort(current, amount, 0, limit);
    if (which == 0) {
        character->pools.hp.cur = current;
    } else {
        character->pools.mp.cur = current;
    }
}

RVA(0x00034680, 0x10)
i16 ReadBranchTarget(void) {
    return ReadJumpTarget();
}
