#ifndef GITEN_SOUND_MIDISTREAM_H
#define GITEN_SOUND_MIDISTREAM_H

#include <rva.h>

#include <Enums.h>
#include <Platform/WinMM.h>

// A .mds file's 'fmt ' chunk.
struct MidsFormat {
    DWORD timeFormat;
    DWORD maxBuffer;
    DWORD flags;
};

// A serialized buffer in the MIDS data chunk, followed by its event bytes.
struct MidsBuffer {
    DWORD tickOffset;
    DWORD byteCount;
    BYTE events[];
};

// A prepared WinMM header and its owned event payload in one allocation.
struct MidiStreamBuffer {
    MIDIHDR header;
    char events[];
};

#define GetNextMidiStreamBuffer(buffer)                                                            \
    reinterpret_cast<MidiStreamBuffer*>((buffer)->events + (buffer)->header.dwBufferLength)

// MidsFormat.flags: the buffers hold 8-byte events without a stream id.
#define MDS_F_NOSTREAMID 0x00000001

#define GetMidiOutputHandle(stream) reinterpret_cast<HMIDIOUT>(stream)

#define PackMidiVolumeMessage(channel, volume) (((volume) << 16) | (0xb0 + (channel)) | (7 << 8))

// clang-format off
GZ_ENUM_BEGIN(MidiPart)
    MIDI_PART_INTRO = 0,
    MIDI_PART_LOOP = 1,
    MIDI_PART_COUNT = 2
GZ_ENUM_END(MidiPart);

GZ_ENUM_BEGIN(MidiPlayState)
    MIDI_STATE_EMPTY = 0,
    MIDI_STATE_STOPPED = 1,
    MIDI_STATE_PLAYING = 2
GZ_ENUM_END(MidiPlayState);
// clang-format on

// @identity-TODO: a MIDI stream player for .mds files: an intro part and an
// optional loop part, each preloaded into stream buffers. The class name is
// not recovered.
class CMidiStream {
public:
    CMidiStream();
    CMidiStream(LPCSTR intro, LPCSTR loop);
    virtual ~CMidiStream();

    b32 Open(LPCSTR intro, LPCSTR loop);
    b32 Play(LPCSTR intro, LPCSTR loop, BOOL looping, DWORD volume, DWORD* channelVolumes);
    b32 Replay(BOOL looping, DWORD volume, DWORD* channelVolumes);
    b32 Stop();
    b32 Pause();
    b32 Restore();
    b32 SetVolume(DWORD volume, DWORD* channelVolumes);

protected:
    b32 CloseStream();
    void FreeBuffers();
    b32 ReadFormat(void* data, DWORD size, int part);
    b32 ConvertBuffer(MIDIHDR* dst, MIDIHDR* src);
    b32 ReadBuffers(void* data, DWORD size, int part);
    b32 LoadFile(LPCSTR path, int part);
    b32 OpenStream();
    b32 QueueBuffers();
    b32 Prepare();

    RVA(0x00056a10, 0x16)
    b32 HasBuffers() {
        return m_buffers[m_part] != NULL;
    }

    static void CALLBACK
    StreamProc(HMIDIOUT out, UINT msg, DWORD instance, DWORD param1, DWORD param2);
    void OnMessage(HMIDIOUT out, UINT msg, DWORD param1, DWORD param2);

    MidiPart m_part;
    char m_files[MIDI_PART_COUNT][MAX_PATH];
    HMIDISTRM m_stream;
    UINT m_device;
    MidsFormat m_formats[MIDI_PART_COUNT];
    DWORD m_bufferCounts[MIDI_PART_COUNT];
    MidiStreamBuffer* m_buffers[MIDI_PART_COUNT];
    DWORD m_doneCount;
    BOOL m_playing;
    BOOL m_looping;
    BOOL m_prepared;
    MidiPlayState m_state;
    DWORD m_reserved1;
    DWORD m_reserved2[2];
    DWORD m_reserved3;
    DWORD m_volume;
    DWORD* m_channelVolumes;
    BOOL m_restart;
};

#endif // GITEN_SOUND_MIDISTREAM_H
