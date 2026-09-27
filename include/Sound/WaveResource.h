#ifndef GITEN_SOUND_WAVERESOURCE_H
#define GITEN_SOUND_WAVERESOURCE_H

#include <Ints.h>
#include <Platform/WinMM.h>

// Embedded PCM resources have a 16-byte format chunk followed by sample data.
struct WaveResource {
    char riffTag[4];
    DWORD riffSize;
    char waveTag[4];
    char formatTag[4];
    DWORD formatSize;
    PCMWAVEFORMAT format;
    char dataTag[4];
    DWORD dataSize;
    u8 samples[];
};

#endif // GITEN_SOUND_WAVERESOURCE_H
