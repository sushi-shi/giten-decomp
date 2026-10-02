// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Platform/WindowsX.h>
#include <Sound/MidiStream.h>
#include <Sound/Mmio.h>

#include <string.h>

RVA(0x00055a00, 0x79)
CMidiStream::CMidiStream()
    : m_stream(NULL),
      m_device(MIDI_MAPPER),
      m_playing(false),
      m_looping(false),
      m_prepared(false),
      m_state(MIDI_STATE_EMPTY),
      m_reserved1(0),
      m_reserved3(0),
      m_volume(0),
      m_channelVolumes(NULL),
      m_restart(false) {
    int i;

    m_part = MIDI_PART_INTRO;
    for (i = 0; i < MIDI_PART_COUNT; i++) {
        m_files[i][0] = '\0';
        m_bufferCounts[i] = 0;
        m_buffers[i] = NULL;
    }
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00055a80, 0x8e)
CMidiStream::CMidiStream(LPCSTR intro, LPCSTR loop)
    : m_stream(NULL),
      m_device(MIDI_MAPPER),
      m_playing(false),
      m_looping(false),
      m_prepared(false),
      m_state(MIDI_STATE_EMPTY),
      m_reserved1(0),
      m_reserved3(0),
      m_volume(0),
      m_channelVolumes(NULL),
      m_restart(false) {
    int i;

    m_part = MIDI_PART_INTRO;
    for (i = 0; i < MIDI_PART_COUNT; i++) {
        m_files[i][0] = '\0';
        m_bufferCounts[i] = 0;
        m_buffers[i] = NULL;
    }
    Open(intro, loop);
}

RVA(0x00055b10, 0x17)
CMidiStream::~CMidiStream() {
    CloseStream();
    FreeBuffers();
}

RVA(0x00055b30, 0x80)
b32 CMidiStream::CloseStream() {
    DWORD count;
    MidiStreamBuffer* buffer;

    if (m_stream == NULL || midiOutReset(GetMidiOutputHandle(m_stream)) != MMSYSERR_NOERROR) {
        return false;
    }
    buffer = m_buffers[m_part];
    for (count = m_bufferCounts[m_part]; count != 0; count--) {
        midiOutUnprepareHeader(GetMidiOutputHandle(m_stream), &buffer->header, sizeof(MIDIHDR));
        buffer = GetNextMidiStreamBuffer(buffer);
    }
    if (midiStreamClose(m_stream) != MMSYSERR_NOERROR) {
        return false;
    }
    m_stream = NULL;
    m_device = MIDI_MAPPER;
    return true;
}

RVA(0x00055bb0, 0x53)
b32 CMidiStream::Stop() {
    if (m_playing) {
        m_playing = false;
        m_prepared = false;
        m_restart = false;
        m_state = MIDI_STATE_STOPPED;
        if (midiStreamStop(m_stream) == MMSYSERR_NOERROR) {
            CloseStream();
            return true;
        }
        return false;
    }
    return true;
}

RVA(0x00055c10, 0x75)
void CMidiStream::FreeBuffers() {
    int i;

    Stop();
    for (i = 0; i < MIDI_PART_COUNT; i++) {
        if (m_buffers[i] != NULL) {
            HGLOBAL memory = GlobalHandle(m_buffers[i]);
            GlobalUnlock(memory);
            GlobalFree(memory);
            m_buffers[i] = NULL;
            m_files[i][0] = '\0';
        }
    }
    m_state = MIDI_STATE_EMPTY;
}

RVA(0x00055c90, 0xb7)
b32 CMidiStream::ReadFormat(void* data, DWORD size, int part) {
    if (data == NULL) {
        return false;
    }
    CMMMemoryIOInfo info(static_cast<char*>(data), size, 0);
    CMMIO mmio(info);
    CMMTypeChunk riff('M', 'I', 'D', 'S');
    mmio.Descend(riff, MMIO_FINDRIFF);
    CMMIdChunk chunk('f', 'm', 't', ' ');
    mmio.Descend(chunk, riff, MMIO_FINDCHUNK);
    mmio.Read(&m_formats[part], sizeof(MidsFormat));
    mmio.Ascend(chunk, 0);
    return true;
}

RVA(0x00055d50, 0xfe)
b32 CMidiStream::ConvertBuffer(MIDIHDR* dst, MIDIHDR* src) {
    // The pun: stream events are DWORD-granular.
    DWORD* in = reinterpret_cast<DWORD*>(src->lpData);
    DWORD* out = reinterpret_cast<DWORD*>(dst->lpData);
    DWORD inLeft = src->dwBytesRecorded;
    DWORD outLeft;
    outLeft = dst->dwBufferLength;

    if (inLeft & 3) {
        return false;
    }
    while (inLeft != 0) {
        DWORD event;
        DWORD length;

        if (outLeft < 3 * sizeof(DWORD)) {
            return false;
        }
        *out++ = *in++;
        inLeft -= sizeof(DWORD);
        *out++ = 0;
        outLeft -= 2 * sizeof(DWORD);
        if (inLeft == 0) {
            return false;
        }
        event = *in;
        length = 0;
        if (event & MEVT_F_LONG) {
            length = MEVT_EVENTPARM(event);
        }
        length = (length + 3) & ~3;
        *out++ = event;
        in++;
        inLeft -= sizeof(DWORD);
        outLeft -= sizeof(DWORD);
        if (length != 0) {
            if (length > inLeft || length > outLeft) {
                return false;
            }
            memcpy(out, in, length);
        }
        out += length / sizeof(DWORD);
        in += length / sizeof(DWORD);
        inLeft -= length;
        outLeft -= length;
    }
    // The pun: the byte count of the DWORD events written.
    dst->dwBytesRecorded = reinterpret_cast<char*>(out) - dst->lpData;
    return true;
}

RVA(0x00055e50, 0x262)
b32 CMidiStream::ReadBuffers(void* data, DWORD size, int part) {
    if (data == NULL) {
        return false;
    }
    CMMMemoryIOInfo info(static_cast<char*>(data), size, 0);
    CMMIO mmio(info);
    CMMTypeChunk riff('M', 'I', 'D', 'S');
    mmio.Descend(riff, MMIO_FINDRIFF);
    CMMIdChunk chunk('d', 'a', 't', 'a');
    mmio.Descend(chunk, riff, MMIO_FINDCHUNK);
    BYTE* blocks = static_cast<BYTE*>(GlobalAllocPtr(GMEM_MOVEABLE, chunk.cksize));
    if (blocks == NULL) {
        return false;
    }
    mmio.Read(&m_bufferCounts[part], sizeof(DWORD));
    mmio.Read(blocks, chunk.cksize - sizeof(DWORD));
    m_buffers[part] = static_cast<MidiStreamBuffer*>(GlobalAllocPtr(
        GMEM_MOVEABLE,
        (m_formats[part].maxBuffer + sizeof(MidiStreamBuffer)) * m_bufferCounts[part]
    ));
    if (m_buffers[part] == NULL) {
        GlobalFreePtr(blocks);
        return false;
    }
    MidiStreamBuffer* buffer = m_buffers[part];
    BYTE* next = blocks;
    for (int left = m_bufferCounts[part]; left > 0; left--) {
        buffer->header.lpData = buffer->events;
        buffer->header.dwBufferLength = m_formats[part].maxBuffer;
        buffer->header.dwFlags = 0;
        buffer->header.dwUser = 0;
        buffer->header.lpNext = NULL;
        // Byte-forced: MIDS records are read from a serialized byte buffer.
        MidsBuffer* block = reinterpret_cast<MidsBuffer*>(next);
        next = block->events;
        if (m_formats[part].flags & MDS_F_NOSTREAMID) {
            MIDIHDR source;
            // The pun: the events stay in the block buffer.
            source.lpData = reinterpret_cast<LPSTR>(next);
            source.dwBufferLength = source.dwBytesRecorded = block->byteCount;
            if (!ConvertBuffer(&buffer->header, &source)) {
                GlobalFreePtr(blocks);
                return false;
            }
        } else {
            buffer->header.dwBytesRecorded = block->byteCount;
            memcpy(buffer->header.lpData, next, block->byteCount);
        }
        next += block->byteCount;
        // Byte-forced: each prepared buffer owns a variable-length event payload.
        buffer = reinterpret_cast<MidiStreamBuffer*>(buffer->events + m_formats[part].maxBuffer);
    }
    GlobalFreePtr(blocks);
    return true;
}

RVA(0x000560c0, 0x10d)
b32 CMidiStream::LoadFile(LPCSTR path, int part) {
    b32 result = false;
    HANDLE file = CreateFile(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD size = GetFileSize(file, NULL);
    if (size != INVALID_FILE_SIZE) {
        void* data = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE, size);
        if (data != NULL) {
            DWORD read;
            if (ReadFile(file, data, size, &read, NULL) && read == size) {
                ReadFormat(data, size, part);
                ReadBuffers(data, size, part);
                GlobalFreePtr(data);
                strcpy(m_files[part], path);
                result = true;
            }
        }
    }
    CloseHandle(file);
    return result;
}

RVA(0x000561d0, 0x45)
b32 CMidiStream::Open(LPCSTR intro, LPCSTR loop) {
    FreeBuffers();
    if (intro == NULL) {
        return false;
    }
    if (!LoadFile(intro, MIDI_PART_INTRO)) {
        return false;
    }
    if (loop == NULL) {
        return true;
    }
    return LoadFile(loop, MIDI_PART_LOOP);
}

RVA(0x00056220, 0x8d)
b32 CMidiStream::OpenStream() {
    MIDIPROPTIMEDIV property;

    if (m_stream != NULL) {
        CloseStream();
    }
    // API-forced: midiStreamOpen takes the callback and instance as DWORDs.
    if (midiStreamOpen(
            &m_stream,
            &m_device,
            1,
            reinterpret_cast<DWORD>(StreamProc), // API-forced
            reinterpret_cast<DWORD>(this),       // API-forced
            CALLBACK_FUNCTION
        )
        != MMSYSERR_NOERROR) {
        return false;
    }
    property.cbStruct = sizeof(property);
    property.dwTimeDiv = m_formats[m_part].timeFormat;
    // API-forced: midiStreamProperty takes the property block as bytes.
    if (midiStreamProperty(
            m_stream,
            reinterpret_cast<LPBYTE>(&property),
            MIDIPROP_SET | MIDIPROP_TIMEDIV
        )
        != MMSYSERR_NOERROR) {
        CloseStream();
        return false;
    }
    return true;
}

RVA(0x000562b0, 0x83)
b32 CMidiStream::QueueBuffers() {
    MidiStreamBuffer* buffer = m_buffers[m_part];
    DWORD count;

    for (count = m_bufferCounts[m_part]; count != 0; count--) {
        if (midiOutPrepareHeader(GetMidiOutputHandle(m_stream), &buffer->header, sizeof(MIDIHDR))
                != MMSYSERR_NOERROR
            || midiStreamOut(m_stream, &buffer->header, sizeof(MIDIHDR)) != MMSYSERR_NOERROR) {
            Stop();
            return false;
        }
        buffer = GetNextMidiStreamBuffer(buffer);
    }
    return true;
}

RVA(0x00056340, 0x69)
b32 CMidiStream::Prepare() {
    if (!HasBuffers()) {
        return false;
    }
    if (m_playing) {
        if (!m_restart) {
            return true;
        }
        if (!Stop()) {
            return false;
        }
    }
    if (m_prepared && !Stop()) {
        return false;
    }
    if (!OpenStream()) {
        return false;
    }
    return QueueBuffers() ? static_cast<b32>(true) : static_cast<b32>(false);
}

// @early-stop register residue: in the all-channel loop retail builds the
// message in eax and loads the stream handle into edx; here the two swap.
// The stream handle is read for each send.
RVA(0x000563b0, 0xcd)
b32 CMidiStream::SetVolume(DWORD volume, DWORD* channelVolumes) {
    if (!m_playing) {
        return true;
    }
    m_volume = volume;
    if (channelVolumes != NULL) {
        m_channelVolumes = channelVolumes;
        for (int channel = 0; channel < 16; channel++) {
            if (m_channelVolumes[channel] != 0) {
                if (midiOutShortMsg(
                        GetMidiOutputHandle(m_stream),
                        PackMidiVolumeMessage(channel, m_volume * m_channelVolumes[channel] / 100)
                    )
                    != MMSYSERR_NOERROR) {
                    return false;
                }
            }
        }
    } else {
        for (int channel = 0; channel < 16; channel++) {
            if (midiOutShortMsg(
                    GetMidiOutputHandle(m_stream),
                    PackMidiVolumeMessage(channel, volume)
                )
                != MMSYSERR_NOERROR) {
                return false;
            }
        }
    }
    return true;
}

RVA(0x00056480, 0x128)
b32 CMidiStream::Play(
    LPCSTR intro,
    LPCSTR loop,
    BOOL looping,
    DWORD volume,
    DWORD* channelVolumes
) {
    if (strcmp(m_files[MIDI_PART_INTRO], intro) != 0
        || (loop != NULL && strcmp(m_files[MIDI_PART_LOOP], loop) != 0)) {
        if (!Open(intro, loop)) {
            return false;
        }
    }
    if (Prepare()) {
        m_doneCount = 0;
        if (midiStreamRestart(m_stream) != MMSYSERR_NOERROR) {
            Stop();
            return false;
        }
        m_looping = looping;
        m_state = MIDI_STATE_PLAYING;
        m_playing = true;
        m_volume = volume;
        m_channelVolumes = channelVolumes;
        if (!SetVolume(volume, channelVolumes)) {
            Stop();
            return false;
        }
        return true;
    }
    return false;
}

RVA(0x000565b0, 0x8f)
b32 CMidiStream::Replay(BOOL looping, DWORD volume, DWORD* channelVolumes) {
    if (Prepare()) {
        m_doneCount = 0;
        if (midiStreamRestart(m_stream) != MMSYSERR_NOERROR) {
            Stop();
            return false;
        }
        m_looping = looping;
        m_state = MIDI_STATE_PLAYING;
        m_playing = true;
        m_volume = volume;
        m_channelVolumes = channelVolumes;
        if (!SetVolume(volume, channelVolumes)) {
            Stop();
            return false;
        }
        return true;
    }
    return false;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00056640, 0xf0)
b32 CMidiStream::Restore() {
    char files[MIDI_PART_COUNT][MAX_PATH];
    int i;

    for (i = 0; i < MIDI_PART_COUNT; i++) {
        strcpy(files[i], m_files[i]);
    }
    switch (m_state) {
        case MIDI_STATE_EMPTY:
            break;
        case MIDI_STATE_STOPPED:
            Open(files[MIDI_PART_INTRO], files[MIDI_PART_LOOP]);
            Prepare();
            m_doneCount = 0;
            m_prepared = true;
            break;
        case MIDI_STATE_PLAYING:
            Open(files[MIDI_PART_INTRO], files[MIDI_PART_LOOP]);
            Replay(m_looping, m_volume, m_channelVolumes);
            return true;
        default:
            m_state = MIDI_STATE_EMPTY;
            return false;
    }
    return true;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00056730, 0x26)
b32 CMidiStream::Pause() {
    MidiPlayState state = m_state;
    if (!Stop()) {
        m_state = state;
        return false;
    }
    m_state = state;
    return true;
}

RVA(0x00056760, 0x24)
void CALLBACK
CMidiStream::StreamProc(HMIDIOUT out, UINT msg, DWORD instance, DWORD param1, DWORD param2) {
    if (instance != 0) {
        // API-forced: midiStreamOpen hands the player back as the instance DWORD.
        reinterpret_cast<CMidiStream*>(instance)->OnMessage(out, msg, param1, param2);
    }
}

RVA(0x00056790, 0xc8)
void CMidiStream::OnMessage(HMIDIOUT out, UINT msg, DWORD param1, DWORD param2) {
    if (msg != MOM_DONE) {
        return;
    }
    if (++m_doneCount < m_bufferCounts[m_part] || !m_playing) {
        return;
    }
    m_playing = false;
    if (m_part == MIDI_PART_INTRO && m_files[MIDI_PART_LOOP][0] != '\0') {
        m_part = MIDI_PART_LOOP;
    } else if (!m_looping) {
        return;
    }
    if (!QueueBuffers()) {
        return;
    }
    m_doneCount = 0;
    if (midiStreamRestart(m_stream) == MMSYSERR_NOERROR) {
        m_state = MIDI_STATE_PLAYING;
        m_playing = true;
        if (SetVolume(m_volume, m_channelVolumes)) {
            return;
        }
    }
    Stop();
}

// The scalar deleting destructor the vtable's only slot names.
RVA_COMPGEN(0x00056860, 0x1e, ??_GCMidiStream@@UAEPAXI@Z)

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00056a30, 0x1b)
void CMMIO::Open(char* name, DWORD flags) {
    m_hmmio = mmioOpen(name, NULL, flags);
}
