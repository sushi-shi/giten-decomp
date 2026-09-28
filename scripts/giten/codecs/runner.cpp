// Test boundary: the SDK supplies the COM layouts; callbacks provide memory surfaces.
#define CINTERFACE
#include <Win32.h>
#include <Gfx/Bitmap.h>
#include <Sound/MidiStream.h>
#include <Platform/D3DApp.h>
#include <Platform/InputSound.h>
#include <Platform/WinMain.h>
extern "C" {
#include <File/DataFile.h>
#include <Game/AreaMap.h>
#include <Game/ItemRecord.h>
#include <Mem/Handle.h>
#include <Gfx/DDraw.h>
#include <Platform/GameCalls.h>
#include <Sound/Sound.h>
#include <Text/TextPlane.h>
}
#include <stdio.h>
#include <stdlib.h>
#include <mbctype.h>
#include <string.h>

static BYTE* retail;
static BYTE* tableData;
extern "C" void* HandleReadPtr(i32) {
    return tableData;
}
HWND g_mainWindow;
static FARPROC seam(LPCSTR name, LPCSTR library);

static void fail(const char* message) {
    fprintf(stderr, "%s\n", message);
    exit(2);
}
static DWORD word(FILE* file) {
    DWORD value;
    if (fread(&value, 4, 1, file) != 1) {
        fail("truncated protocol");
    }
    return value;
}
static void put(FILE* file, DWORD value) {
    if (fwrite(&value, 4, 1, file) != 1) {
        fail("output write failed");
    }
}
static BYTE* readFile(const char* path, DWORD* size) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fail("cannot open file");
    }
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    rewind(file);
    BYTE* data = static_cast<BYTE*>(malloc(*size));
    if (!data || fread(data, 1, *size, file) != *size) {
        fail("file read failed");
    }
    fclose(file);
    return data;
}
static void hook(DWORD rva, void* target) {
    BYTE* code = retail + rva;
    code[0] = 0xe9;
    *reinterpret_cast<LONG*>(code + 1) = static_cast<BYTE*>(target) - code - 5;
}
static void mapRetail(const char* path) {
    DWORD size;
    BYTE* file = readFile(path, &size);
    IMAGE_DOS_HEADER* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(file);
    if (size < sizeof(*dos) || dos->e_magic != IMAGE_DOS_SIGNATURE
        || static_cast<DWORD>(dos->e_lfanew) > size - sizeof(IMAGE_NT_HEADERS)) {
        fail("bad DOS header");
    }
    IMAGE_NT_HEADERS* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(file + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.ImageBase != 0x400000) {
        fail("unexpected retail PE");
    }
    retail = static_cast<BYTE*>(VirtualAlloc(
        NULL,
        nt->OptionalHeader.SizeOfImage,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    ));
    if (!retail) {
        fail("cannot allocate retail image");
    }
    IMAGE_DATA_DIRECTORY relocDir =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (!relocDir.VirtualAddress || !relocDir.Size) {
        fail("requires the reviewed build/exe/DDS.EXE relocation image");
    }
    memcpy(retail, file, nt->OptionalHeader.SizeOfHeaders);
    IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt);
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (section[i].PointerToRawData + section[i].SizeOfRawData > size
            || section[i].VirtualAddress + section[i].SizeOfRawData
                   > nt->OptionalHeader.SizeOfImage) {
            fail("invalid PE section");
        }
        memcpy(
            retail + section[i].VirtualAddress,
            file + section[i].PointerToRawData,
            section[i].SizeOfRawData
        );
    }
    LONG delta = reinterpret_cast<LONG>(retail) - 0x400000;
    DWORD offset = relocDir.VirtualAddress;
    while (offset < relocDir.VirtualAddress + relocDir.Size) {
        IMAGE_BASE_RELOCATION* block = reinterpret_cast<IMAGE_BASE_RELOCATION*>(retail + offset);
        if (block->SizeOfBlock < 8
            || offset + block->SizeOfBlock > relocDir.VirtualAddress + relocDir.Size) {
            fail("bad relocation block");
        }
        WORD* entries = reinterpret_cast<WORD*>(block + 1);
        for (DWORD j = 0; j < (block->SizeOfBlock - 8) / 2; ++j) {
            DWORD type = entries[j] >> 12;
            if (type == IMAGE_REL_BASED_HIGHLOW) {
                *reinterpret_cast<LONG*>(retail + block->VirtualAddress + (entries[j] & 0xfff)) +=
                    delta;
            } else if (type != IMAGE_REL_BASED_ABSOLUTE) {
                fail("unsupported relocation");
            }
        }
        offset += block->SizeOfBlock;
    }
    // Bind imports for the memory-only WinMM reader; never call the game entry point.
    IMAGE_IMPORT_DESCRIPTOR* imp = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
        retail + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
    );
    for (; imp->Name; ++imp) {
        HMODULE dll = LoadLibraryA(reinterpret_cast<char*>(retail + imp->Name));
        if (!dll) {
            fail("retail import library unavailable");
        }
        DWORD* names =
            reinterpret_cast<DWORD*>(retail + reinterpret_cast<DWORD>(imp->OriginalFirstThunk));
        DWORD* slots = reinterpret_cast<DWORD*>(retail + reinterpret_cast<DWORD>(imp->FirstThunk));
        for (; *names; ++names, ++slots) {
            LPCSTR name = (*names & IMAGE_ORDINAL_FLAG)
                              ? reinterpret_cast<LPCSTR>(*names & 0xffff)
                              : reinterpret_cast<LPCSTR>(retail + *names + 2);
            FARPROC function = seam(name, reinterpret_cast<LPCSTR>(retail + imp->Name));
            if (!function) {
                function = GetProcAddress(dll, name);
            }
            if (!function) {
                fail("retail import unavailable");
            }
            *slots = reinterpret_cast<DWORD>(function);
        }
    }
    free(file);
    // Only the stdio boundary is redirected: both sides must use the same CRT FILE ABI.
    hook(0x5a9f0, reinterpret_cast<void*>(fread));
    hook(0x5c0b0, reinterpret_cast<void*>(fseek));
}
static int call2(DWORD rva, void* self, void* a, void* b) {
    void* fn = retail + rva;
    int result;
    __asm {
        push b
        push a
        mov ecx, self
        call fn
        mov result, eax
    }
    return result;
}
static int call3(DWORD rva, void* self, void* data, DWORD dataSize, int part) {
    void* fn = retail + rva;
    int result;
    __asm {
        push part
        push dataSize
        push data
        mov ecx, self
        call fn
        mov result, eax
    }
    return result;
}
class MidiProbe : public CMidiStream {
public:
    BOOL convert(MIDIHDR* dst, MIDIHDR* src) {
        return ConvertBuffer(dst, src);
    }
    DWORD read(bool candidate, BYTE* data, DWORD size, BYTE* out) {
        BOOL format = candidate ? ReadFormat(data, size, 0) : call3(0x55c90, this, data, size, 0);
        BOOL buffers = candidate ? ReadBuffers(data, size, 0) : call3(0x55e50, this, data, size, 0);
        if (!format || !buffers) {
            fail("MIDS reader failed");
        }
        memcpy(out, &m_formats[0], sizeof(MidsFormat));
        memcpy(out + 12, &m_bufferCounts[0], 4);
        DWORD used = 16;
        MidiStreamBuffer* buffer = m_buffers[0];
        for (DWORD index = 0; index < m_bufferCounts[0]; ++index) {
            DWORD size = buffer->header.dwBytesRecorded;
            if (size > m_formats[0].maxBuffer || used + 8 + size > 4 * 1024 * 1024) {
                fail("invalid loaded MIDS buffer");
            }
            if (buffer->header.lpData != buffer->events || buffer->header.dwUser
                || buffer->header.dwFlags || buffer->header.lpNext) {
                fail("MIDS header mismatch");
            }
            memcpy(out + used, &buffer->header.dwBufferLength, 4);
            memcpy(out + used + 4, &size, 4);
            memcpy(out + used + 8, buffer->header.lpData, size);
            used += 8 + size;
            buffer = GetNextMidiStreamBuffer(buffer);
        }
        return used;
    }
};

static DDSURFACEDESC surfaceDesc;
static DDCOLORKEY surfaceKey;
static DWORD calls[5];
static PALETTEENTRY createdPalette[256];
static IDirectDrawSurfaceVtbl surfaceMethods;
static IDirectDrawVtbl drawMethods;
static IDirectDrawSurface surface;
static IDirectDraw draw;
static IDirectDrawPalette palette;
static HRESULT WINAPI
lockSurface(IDirectDrawSurface*, LPRECT, LPDDSURFACEDESC desc, DWORD, HANDLE) {
    *desc = surfaceDesc;
    ++calls[0];
    return DD_OK;
}
static HRESULT WINAPI unlockSurface(IDirectDrawSurface*, LPVOID) {
    ++calls[1];
    return DD_OK;
}
static HRESULT WINAPI keySurface(IDirectDrawSurface*, DWORD flags, LPDDCOLORKEY key) {
    surfaceKey = *key;
    calls[2] = flags;
    return DD_OK;
}
static HRESULT WINAPI paletteSurface(IDirectDrawSurface*, LPDIRECTDRAWPALETTE value) {
    if (value != &palette) {
        fail("unexpected palette handle");
    }
    ++calls[3];
    return DD_OK;
}
static HRESULT WINAPI createPalette(
    IDirectDraw*,
    DWORD flags,
    LPPALETTEENTRY entries,
    LPDIRECTDRAWPALETTE* out,
    IUnknown*
) {
    memcpy(createdPalette, entries, sizeof(createdPalette));
    *out = &palette;
    calls[4] = flags;
    return DD_OK;
}
static void initSurface() {
    surfaceMethods.Lock = lockSurface;
    surfaceMethods.Unlock = unlockSurface;
    surfaceMethods.SetColorKey = keySurface;
    surfaceMethods.SetPalette = paletteSurface;
    surface.lpVtbl = &surfaceMethods;
    drawMethods.CreatePalette = createPalette;
    draw.lpVtbl = &drawMethods;
    g_ddraw = &draw;
    *reinterpret_cast<IDirectDraw**>(retail + 0x841e4) = g_ddraw;
}

static BYTE* waveInput;
static DWORD soundCapacity[2];
static DWORD soundTrace[8];
static BYTE soundBytes[500000];
static IDirectSoundBufferVtbl soundMethods;
static IDirectSoundVtbl deviceMethods;
static IDirectSoundBuffer soundBuffers[2];
static IDirectSound soundDevice;
static HRESULT WINAPI soundCooperate(IDirectSound*, HWND, DWORD) {
    return DS_OK;
}
static HRESULT WINAPI
soundCreateBuffer(IDirectSound*, LPCDSBUFFERDESC desc, LPDIRECTSOUNDBUFFER* out, IUnknown*) {
    unsigned index = desc->dwBufferBytes == 0x204cc ? 0 : 1;
    soundCapacity[index] = desc->dwBufferBytes;
    *out = &soundBuffers[index];
    return DS_OK;
}
static HRESULT WINAPI soundVolume(IDirectSoundBuffer*, LONG) {
    return DS_OK;
}
static HRESULT WINAPI soundStatus(IDirectSoundBuffer*, LPDWORD status) {
    *status = 0;
    return DS_OK;
}
static HRESULT WINAPI soundLock(
    IDirectSoundBuffer* self,
    DWORD offset,
    DWORD size,
    LPVOID* first,
    LPDWORD firstSize,
    LPVOID* second,
    LPDWORD secondSize,
    DWORD flags
) {
    unsigned index = self == &soundBuffers[0] ? 0 : 1;
    soundTrace[0] = index;
    soundTrace[1] = offset;
    soundTrace[2] = size;
    soundTrace[3] = flags;
    if (size > soundCapacity[index]) {
        fail("wave exceeds playback capacity");
    }
    *first = soundBytes;
    *firstSize = size;
    *second = NULL;
    *secondSize = 0;
    return DS_OK;
}
static HRESULT WINAPI
soundUnlock(IDirectSoundBuffer*, LPVOID first, DWORD size, LPVOID second, DWORD secondSize) {
    if (first != soundBytes || second || secondSize) {
        fail("wave unlock mismatch");
    }
    soundTrace[4] = size;
    return DS_OK;
}
static HRESULT WINAPI soundPosition(IDirectSoundBuffer*, DWORD position) {
    soundTrace[5] = position;
    return DS_OK;
}
static HRESULT WINAPI soundPlay(IDirectSoundBuffer*, DWORD reserved, DWORD priority, DWORD flags) {
    if (reserved || priority) {
        fail("wave play arguments differ");
    }
    soundTrace[6] = flags;
    return DS_OK;
}
static HRESULT WINAPI soundCreate(LPGUID, LPDIRECTSOUND* out, IUnknown*) {
    *out = &soundDevice;
    return DS_OK;
}
static HRSRC WINAPI findWave(HMODULE, LPCSTR name, LPCSTR type) {
    if (strcmp(type, "WAVE")) {
        fail("unexpected resource type");
    }
    soundTrace[7] = reinterpret_cast<DWORD>(name);
    return reinterpret_cast<HRSRC>(waveInput);
}
static HGLOBAL WINAPI loadWave(HMODULE, HRSRC resource) {
    return reinterpret_cast<HGLOBAL>(resource);
}
static LPVOID WINAPI lockWave(HGLOBAL resource) {
    return resource;
}
static FARPROC seam(LPCSTR name, LPCSTR library) {
    if (!_stricmp(library, "dsound.dll") && reinterpret_cast<DWORD>(name) == 1) {
        return reinterpret_cast<FARPROC>(soundCreate);
    }
    if (reinterpret_cast<DWORD>(name) < 65536) {
        return NULL;
    }
    if (!strcmp(name, "DirectSoundCreate")) {
        return reinterpret_cast<FARPROC>(soundCreate);
    }
    if (!strcmp(name, "FindResourceA")) {
        return reinterpret_cast<FARPROC>(findWave);
    }
    if (!strcmp(name, "LoadResource")) {
        return reinterpret_cast<FARPROC>(loadWave);
    }
    if (!strcmp(name, "LockResource")) {
        return reinterpret_cast<FARPROC>(lockWave);
    }
    return NULL;
}
static void bindCandidateSeams() {
    BYTE* image = reinterpret_cast<BYTE*>(GetModuleHandle(NULL));
    IMAGE_DOS_HEADER* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image);
    IMAGE_NT_HEADERS* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(image + dos->e_lfanew);
    IMAGE_IMPORT_DESCRIPTOR* imp = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
        image + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
    );
    for (; imp->Name; ++imp) {
        DWORD* names =
            reinterpret_cast<DWORD*>(image + reinterpret_cast<DWORD>(imp->OriginalFirstThunk));
        DWORD* slots = reinterpret_cast<DWORD*>(image + reinterpret_cast<DWORD>(imp->FirstThunk));
        for (; *names; ++names, ++slots) {
            LPCSTR name = (*names & IMAGE_ORDINAL_FLAG)
                              ? reinterpret_cast<LPCSTR>(*names & 0xffff)
                              : reinterpret_cast<LPCSTR>(image + *names + 2);
            FARPROC target = seam(name, reinterpret_cast<LPCSTR>(image + imp->Name));
            if (target) {
                DWORD protect;
                if (!VirtualProtect(slots, 4, PAGE_READWRITE, &protect)) {
                    fail("cannot patch harness IAT");
                }
                *slots = reinterpret_cast<DWORD>(target);
                VirtualProtect(slots, 4, protect, &protect);
            }
        }
    }
}
static void initSound() {
    deviceMethods.SetCooperativeLevel = soundCooperate;
    deviceMethods.CreateSoundBuffer = soundCreateBuffer;
    soundDevice.lpVtbl = &deviceMethods;
    soundMethods.SetVolume = soundVolume;
    soundMethods.GetStatus = soundStatus;
    soundMethods.Lock = soundLock;
    soundMethods.Unlock = soundUnlock;
    soundMethods.SetCurrentPosition = soundPosition;
    soundMethods.Play = soundPlay;
    soundBuffers[0].lpVtbl = soundBuffers[1].lpVtbl = &soundMethods;
    bindCandidateSeams();
    if (!InitDirectSound() || !reinterpret_cast<BOOL(__cdecl*)(void)>(retail + 0x488a0)()) {
        fail("sound setup failed");
    }
    g_soundEnabled = 1;
    *reinterpret_cast<b32*>(retail + 0x84270) = 1;
}

static BYTE output[4 * 1024 * 1024];
static BYTE snapshot[sizeof(output)];
static BYTE buffer[sizeof(output)];
static BYTE input[sizeof(output)];
static BYTE originalInput[sizeof(output)];
static char nameText[65536], descriptionText[65536];
static FILE* stream;
static DWORD jobKind, arg, inputSize;

// Outputs are canonical wire values, followed by bytes. Pointer results are checked in place.
static DWORD run(bool candidate) {
    if (jobKind == 1 || jobKind == 10) {
        u16 length = *reinterpret_cast<u16*>(input);
        rewind(stream);
        memset(buffer, 0xcd, length + 16);
        if (jobKind == 10) {
            if (candidate) {
                ReadRawBlock(stream, buffer);
            } else {
                reinterpret_cast<void(__cdecl*)(FILE*, void*)>(retail + 0x1d90)(stream, buffer);
            }
        } else if (arg == 0) {
            void* result = candidate
                               ? ReadCryptRecord(stream, buffer)
                               : reinterpret_cast<void*(__cdecl*)(FILE*, void*)>(retail + 0x1bf0)(
                                     stream,
                                     buffer
                                 );
            if (result != buffer) {
                fail("crypt return mismatch");
            }
        } else {
            fseek(stream, 2, SEEK_SET);
            if (candidate) {
                SetCryptKey(length);
            } else {
                reinterpret_cast<void(__cdecl*)(u16)>(retail + 0x1b20)(length);
            }
            unsigned pos = 0;
            while (pos < length) {
                if (arg == 2 && length - pos >= 2) {
                    u16 value =
                        candidate ? ReadCryptWord(stream)
                                  : reinterpret_cast<u16(__cdecl*)(FILE*)>(retail + 0x1b70)(stream);
                    memcpy(buffer + pos, &value, 2);
                    pos += 2;
                } else if (arg == 3) {
                    u16 amount = static_cast<u16>(length - pos > 17 ? 17 : length - pos);
                    u16 result =
                        candidate
                            ? ReadCryptBytes(stream, amount, buffer + pos)
                            : reinterpret_cast<u16(__cdecl*)(FILE*, u16, u8*)>(retail + 0x1ba0)(
                                  stream,
                                  amount,
                                  buffer + pos
                              );
                    if (result != amount) {
                        fail("crypt byte count mismatch");
                    }
                    pos += amount;
                } else {
                    buffer[pos++] =
                        candidate ? ReadCryptByte(stream)
                                  : reinterpret_cast<u8(__cdecl*)(FILE*)>(retail + 0x1b40)(stream);
                }
            }
        }
        if (ftell(stream) != length + 2) {
            fail("record stream position mismatch");
        }
        memcpy(output, buffer, length + 16);
        return length + 16;
    }
    if (jobKind == 2 || jobKind == 3) {
        bool resource = jobKind == 3;
        bool eight = arg == 8;
        BmpFile* bmp = reinterpret_cast<BmpFile*>(input);
        BitmapResource* dib = reinterpret_cast<BitmapResource*>(input);
        BITMAPINFOHEADER* info = resource ? &dib->info : &bmp->info;
        DWORD width = info->biWidth, height = info->biHeight;
        DWORD pitch = (width + 8) * (eight ? 1 : 2), rows = height + 4;
        DWORD bytes = pitch * rows;
        if (bytes + 4096 > sizeof(output)) {
            fail("surface too large");
        }
        memset(buffer, 0xcd, bytes);
        memset(calls, 0, sizeof(calls));
        memset(createdPalette, 0xcd, sizeof(createdPalette));
        memset(&surfaceKey, 0xcd, sizeof(surfaceKey));
        surfaceDesc.dwSize = sizeof(surfaceDesc);
        surfaceDesc.dwWidth = width;
        surfaceDesc.dwHeight = height;
        surfaceDesc.lPitch = pitch;
        surfaceDesc.lpSurface = buffer;
        g_redShift = arg == 555 ? 10 : 11;
        g_greenShift = 5;
        g_blueShift = 0;
        g_redLoss = 3;
        g_greenLoss = arg == 555 ? 3 : 2;
        g_blueLoss = 3;
        retail[0x84310] = g_redShift;
        retail[0x84314] = g_greenShift;
        retail[0x84318] = g_blueShift;
        retail[0x8431c] = g_redLoss;
        retail[0x84320] = g_greenLoss;
        retail[0x84324] = g_blueLoss;
        IDirectDrawSurface* target = &surface;
        IDirectDrawPalette* pal = NULL;
        PALETTEENTRY entries[256];
        memset(entries, 0xcd, sizeof(entries));
        DWORD key = 0xcdcdcdcd;
        b32 result;
        if (resource && eight) {
            result = candidate ? CopyResourceBitmap8(dib, &target, entries, &pal, 3, 2)
                               : reinterpret_cast<b32(__cdecl*)(
                                     BitmapResource*,
                                     IDirectDrawSurface**,
                                     PALETTEENTRY*,
                                     IDirectDrawPalette**,
                                     i32,
                                     i32
                                 )>(retail + 0x57120)(dib, &target, entries, &pal, 3, 2);
        } else if (resource) {
            result = candidate ? CopyResourceBitmap16(dib, &target, 3, 2)
                               : reinterpret_cast<b32(__cdecl*)(
                                     BitmapResource*,
                                     IDirectDrawSurface**,
                                     i32,
                                     i32
                                 )>(retail + 0x56f30)(dib, &target, 3, 2);
        } else if (eight) {
            result = candidate ? LoadBitmapToSurface8(bmp, &target, entries, &pal)
                               : reinterpret_cast<b32(__cdecl*)(
                                     BmpFile*,
                                     IDirectDrawSurface**,
                                     PALETTEENTRY*,
                                     IDirectDrawPalette**
                                 )>(retail + 0x56de0)(bmp, &target, entries, &pal);
        } else {
            result =
                candidate
                    ? LoadBitmapToSurface16(bmp, &target, &key)
                    : reinterpret_cast<
                          b32(__cdecl*)(BmpFile*, IDirectDrawSurface**, DWORD*)>(retail + 0x56bd0)(
                          bmp,
                          &target,
                          &key
                      );
        }
        DWORD* head = reinterpret_cast<DWORD*>(output);
        head[0] = result;
        head[1] = key;
        head[2] = surfaceKey.dwColorSpaceLowValue;
        head[3] = surfaceKey.dwColorSpaceHighValue;
        memcpy(head + 4, calls, sizeof(calls));
        memcpy(output + 36, entries, 1024);
        memcpy(output + 1060, createdPalette, 1024);
        memcpy(output + 2084, buffer, bytes);
        return 2084 + bytes;
    }
    if (jobKind == 4) {
        MIDIHDR src, dst;
        memset(&src, 0, sizeof(src));
        memset(&dst, 0, sizeof(dst));
        memset(buffer, 0xcd, arg + 16);
        src.lpData = reinterpret_cast<char*>(input);
        src.dwBytesRecorded = inputSize;
        dst.lpData = reinterpret_cast<char*>(buffer);
        dst.dwBufferLength = arg;
        dst.dwBytesRecorded = 0xcdcdcdcd;
        BOOL result;
        if (candidate) {
            MidiProbe probe;
            result = probe.convert(&dst, &src);
        } else {
            result = call2(0x55d50, NULL, &dst, &src);
        }
        *reinterpret_cast<DWORD*>(output) = result;
        *reinterpret_cast<DWORD*>(output + 4) = dst.dwBytesRecorded;
        memcpy(output + 8, buffer, arg + 16);
        return arg + 24;
    }
    if (jobKind == 5) {
        memset(buffer, 0xcd, 0x4000);
        AreaMap* map = reinterpret_cast<AreaMap*>(buffer);
        if (candidate) {
            DecodeAreaMap(map, input);
        } else {
            reinterpret_cast<void(__cdecl*)(AreaMap*, u8*)>(retail + 0x21470)(map, input);
        }
        // Same input and destination addresses on both calls; normalize only for the wire format.
        map->name = reinterpret_cast<char*>(map->name - reinterpret_cast<char*>(input));
        for (int i = 0; i < map->levelCount; ++i) {
            AreaLevel* level = map->levels[i];
            // The pointer fields are enumerated by the shared type, not scanned by value.
#define NORMALIZE(field)                                                                           \
    level->field = reinterpret_cast<type_##field>(reinterpret_cast<BYTE*>(level->field) - buffer)
            typedef u8* type_blockBits;
            typedef u16* type_walls;
            typedef WarpCell* type_warps;
            typedef BattleCell* type_battles;
            typedef LinkCell* type_links;
            typedef ObjectCell* type_objects;
            typedef MapSpawn* type_spawns;
            typedef ScriptCell* type_scripts;
            typedef DoorCell* type_doors;
            typedef ExitCell* type_exits;
            typedef TreasureBox* type_boxes;
            typedef u8* type_roomDoors;
            typedef u8* type_roomWalls;
            typedef u8* type_roomBits;
            NORMALIZE(blockBits);
            NORMALIZE(walls);
            NORMALIZE(warps);
            NORMALIZE(battles);
            NORMALIZE(links);
            NORMALIZE(objects);
            NORMALIZE(spawns);
            NORMALIZE(scripts);
            NORMALIZE(doors);
            NORMALIZE(exits);
            NORMALIZE(boxes);
            NORMALIZE(roomDoors);
            NORMALIZE(roomWalls);
            NORMALIZE(roomBits);
#undef NORMALIZE
            map->levels[i] = reinterpret_cast<AreaLevel*>(reinterpret_cast<BYTE*>(level) - buffer);
        }
        memcpy(output, buffer, 0x4000);
        return 0x4000;
    }
    if (jobKind == 6) {
        tableData = input;
        g_itemNameText = nameText;
        g_itemDescriptionText = descriptionText;
        *reinterpret_cast<char**>(retail + 0x800f4) = nameText;
        *reinterpret_cast<char**>(retail + 0x800f8) = descriptionText;
        ItemRecord record;
        memset(&record, 0xcd, sizeof(record));
        ItemRecord* result =
            candidate ? DecodeItemRecord(&record, static_cast<i16>(arg))
                      : reinterpret_cast<ItemRecord*(__cdecl*)(ItemRecord*, i16)>(retail + 0x22d40)(
                            &record,
                            static_cast<i16>(arg)
                        );
        if (result != &record || record.name != nameText || record.description != descriptionText) {
            fail("item pointer result mismatch");
        }
        DWORD fields = sizeof(record) - 2 * sizeof(char*);
        memcpy(output, &record, fields);
        DWORD nameSize = strlen(nameText) + 1, descSize = strlen(descriptionText) + 1;
        memcpy(output + fields, nameText, nameSize);
        memcpy(output + fields + nameSize, descriptionText, descSize);
        return fields + nameSize + descSize;
    }
    if (jobKind == 11) {
        DWORD size = 0;
        for (DWORD pos = 0; pos < inputSize; pos += arg) {
            u16 code = 0xcdcd;
            i16 next = candidate
                           ? ReadTextChar(&code, input + pos, 0)
                           : reinterpret_cast<i16(__cdecl*)(u16*, const u8*, i16)>(retail + 0x1aa0)(
                                 &code,
                                 input + pos,
                                 0
                             );
            memcpy(output + size, &next, 2);
            memcpy(output + size + 2, &code, 2);
            size += 4;
        }
        return size;
    }
    if (jobKind == 9) {
        waveInput = input + 4;
        memset(soundTrace, 0xcd, sizeof(soundTrace));
        memset(soundBytes, 0xcd, sizeof(soundBytes));
        if (candidate) {
            PlaySoundEffect(static_cast<i16>(arg));
        } else {
            reinterpret_cast<void(__cdecl*)(i16)>(retail + 0x489f0)(static_cast<i16>(arg));
        }
        memcpy(output, soundTrace, sizeof(soundTrace));
        if (soundTrace[0] > 1) {
            fprintf(
                stderr,
                "wave did not lock: candidate=%d sound=%lu enabled=%ld retail=%ld resource=%lx\n",
                candidate,
                arg,
                g_soundEnabled,
                *reinterpret_cast<b32*>(retail + 0x84270),
                soundTrace[7]
            );
            fail("missing wave lock");
        }
        DWORD bytes = soundCapacity[soundTrace[0]] + 16;
        memcpy(output + sizeof(soundTrace), soundBytes, bytes);
        return sizeof(soundTrace) + bytes;
    }
    if (jobKind == 8) {
        MidiProbe probe;
        return probe.read(candidate, input, inputSize, output);
    }
    if (jobKind == 7) {
        rewind(stream);
        u32 size = 0xcdcdcdcd;
        FILE* result =
            candidate ? SeekBitmap(stream, static_cast<i16>(arg), inputSize, &size)
                      : reinterpret_cast<FILE*(__cdecl*)(FILE*, i16, i32, u32*)>(retail + 0x56b00)(
                            stream,
                            static_cast<i16>(arg),
                            inputSize,
                            &size
                        );
        DWORD* head = reinterpret_cast<DWORD*>(output);
        head[0] = result == stream;
        head[1] = ftell(stream);
        head[2] = size;
        return 12;
    }
    fail("unknown job kind");
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        fail("runner retail.exe jobs.bin results.bin");
    }
    mapRetail(argv[1]);
    initSurface();
    initSound();
    if (_setmbcp(932)) {
        fail("cannot set Japanese code page");
    }
    memcpy(retail + 0x90c10, _mbctype, 257);
    // Handle storage is an explicit seam; no record decoding is replaced.
    hook(0x4680, reinterpret_cast<void*>(HandleReadPtr));
    FILE* jobs = fopen(argv[2], "rb");
    FILE* results = fopen(argv[3], "wb");
    if (!jobs || !results) {
        fail("cannot open protocol files");
    }
    if (word(jobs) != 0x424f4a47) {
        fail("bad job magic");
    }
    DWORD count = word(jobs);
    put(results, 0x53455247);
    put(results, count);
    char tempPath[MAX_PATH];
    if (strlen(argv[3]) + 8 >= sizeof(tempPath)) {
        fail("output path too long");
    }
    strcpy(tempPath, argv[3]);
    strcat(tempPath, ".stream");
    unsigned mismatches = 0;
    for (DWORD i = 0; i < count; ++i) {
        jobKind = word(jobs);
        arg = word(jobs);
        inputSize = word(jobs);
        if (inputSize > sizeof(input) - 65536 || arg > sizeof(buffer) - 65536) {
            fail("job too large");
        }
        DWORD inputExtent = (jobKind == 5 && inputSize < 0x4000 ? 0x4000 : inputSize) + 16;
        memset(input, 0, inputExtent);
        if (fread(input, 1, inputSize, jobs) != inputSize) {
            fail("truncated job");
        }
        stream = NULL;
        if (jobKind == 1 || jobKind == 7 || jobKind == 10) {
            stream = fopen(tempPath, "w+b");
            if (!stream) {
                fail("temporary stream failed");
            }
            if (fwrite(input, 1, inputSize, stream) != inputSize) {
                fail("tmpfile write failed");
            }
        }
        memcpy(originalInput, input, inputExtent);
        DWORD retailSize = run(false);
        if (memcmp(originalInput, input, inputExtent)) {
            fail("retail modified input");
        }
        memcpy(snapshot, output, retailSize);
        DWORD candidateSize = run(true);
        if (memcmp(originalInput, input, inputExtent)) {
            fail("candidate modified input");
        }
        bool equal = retailSize == candidateSize && memcmp(snapshot, output, retailSize) == 0;
        if (!equal) {
            fprintf(stderr, "mismatch job=%lu kind=%lu arg=%lu\n", i, jobKind, arg);
            ++mismatches;
        }
        put(results, equal);
        put(results, retailSize);
        if (fwrite(snapshot, 1, retailSize, results) != retailSize) {
            fail("short result write");
        }
        if (!equal) {
            put(results, 1);
            put(results, candidateSize);
            if (fwrite(output, 1, candidateSize, results) != candidateSize) {
                fail("short candidate result write");
            }
        }
        if (stream) {
            fclose(stream);
        }
    }
    if (fgetc(jobs) != EOF) {
        fail("trailing jobs");
    }
    remove(tempPath);
    fclose(jobs);
    if (fclose(results)) {
        fail("result close failed");
    }
    fprintf(stderr, "%lu jobs, %u candidate disagreements\n", count, mismatches);
    return mismatches ? 1 : 0;
}
