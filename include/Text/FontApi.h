#ifndef GITEN_TEXT_FONTAPI_H
#define GITEN_TEXT_FONTAPI_H

#include <rva.h>

#include <Ints.h>

#include <stdio.h>

// The C headers the text and layer TU (font.cpp) implements and calls into,
// with C linkage.
extern "C" {
#include <Gfx/Bitmap.h>
#include <Gfx/Picture.h>
#include <Text/Font.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Text/WindowText.h>
#include <Ui/Menu.h>
#include <Ui/Panel.h>
#include <Util/List.h>
#include <Game/FieldHud.h>
#include <Game/Party.h>
#include <Ui/FieldMenus.h>
}

#endif // GITEN_TEXT_FONTAPI_H
