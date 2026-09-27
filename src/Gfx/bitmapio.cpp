// @identity-TODO: the original owning TU remains unproven.

#include <rva.h>

#include <Gfx/Bitmap.h>

#include <io.h>
#include <new.h>
#include <stdio.h>

RVA(0x00056a70, 0x70)
BmpFile* ReadBitmapFile(const char* path) {
    FILE* file = fopen(path, "rb");
    u32 size;
    BmpFile* bmp;
    if (file == NULL) {
        return NULL;
    }
    size = _filelength(_fileno(file));
    bmp = static_cast<BmpFile*>(operator new(size));
    if (fread(bmp, 1, size, file) != size) {
        operator delete(bmp);
        fclose(file);
        return NULL;
    }
    fclose(file);
    return bmp;
}

RVA(0x00056ae0, 0x1c)
void FreeBitmap(BmpFile** bmp) {
    if (*bmp != NULL) {
        operator delete(*bmp);
    }
    *bmp = NULL;
}
