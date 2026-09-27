#ifndef GITEN_SOUND_MMIO_H
#define GITEN_SOUND_MMIO_H

#include <rva.h>

#include <Platform/WinMM.h>

#include <string.h>

// Thin C++ wrappers over the multimedia file I/O API. The platform layer is
// built /Ob0, so every in-class body below is emitted out of line.

class CMMChunk : public MMCKINFO {
protected:
    RVA(0x000568c0, 0x3)
    CMMChunk() {}
};

class CMMIdChunk : public CMMChunk {
public:
    RVA(0x00056880, 0x38)
    CMMIdChunk(char c0, char c1, char c2, char c3) {
        ckid = mmioFOURCC(c0, c1, c2, c3);
    }
};

class CMMTypeChunk : public CMMChunk {
public:
    RVA(0x000568d0, 0x39)
    CMMTypeChunk(char c0, char c1, char c2, char c3) {
        fccType = mmioFOURCC(c0, c1, c2, c3);
    }
};

class CMMIOInfo : public MMIOINFO {
public:
    RVA(0x00056950, 0x12)
    CMMIOInfo() {
        memset(this, 0, sizeof(MMIOINFO));
    }
};

class CMMMemoryIOInfo : public CMMIOInfo {
public:
    RVA(0x00056910, 0x31)
    CMMMemoryIOInfo(char* buffer, LONG size, DWORD minExpand) {
        pIOProc = NULL;
        fccIOProc = FOURCC_MEM;
        pchBuffer = buffer;
        cchBuffer = size;
        adwInfo[0] = minExpand;
    }
};

class CMMIO {
public:
    RVA(0x00056970, 0x13)
    CMMIO(CMMMemoryIOInfo& info) {
        Open(info);
    }

    void Open(char* name, DWORD flags);

    RVA(0x00056a50, 0x18)
    void Open(CMMMemoryIOInfo& info) {
        m_hmmio = mmioOpen(NULL, &info, MMIO_READWRITE);
    }

    RVA(0x00056990, 0x16)
    LONG Read(void* buffer, LONG size) {
        return mmioRead(m_hmmio, static_cast<HPSTR>(buffer), size);
    }

    RVA(0x000569b0, 0x16)
    MMRESULT Ascend(CMMChunk& chunk, UINT flags) {
        return mmioAscend(m_hmmio, &chunk, flags);
    }

    RVA(0x000569d0, 0x18)
    MMRESULT Descend(CMMChunk& chunk, UINT flags) {
        return mmioDescend(m_hmmio, &chunk, NULL, flags);
    }

    RVA(0x000569f0, 0x1b)
    MMRESULT Descend(CMMChunk& chunk, CMMChunk& parent, UINT flags) {
        return mmioDescend(m_hmmio, &chunk, &parent, flags);
    }

private:
    HMMIO m_hmmio;
};

#endif // GITEN_SOUND_MMIO_H
