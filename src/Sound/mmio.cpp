// @identity-TODO: the original source filename is unproven.

#include <Sound/Mmio.h>

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00056a30, 0x1b)
void CMMIO::Open(char* name, DWORD flags) {
    m_hmmio = mmioOpen(name, NULL, flags);
}

RVA(0x00056a50, 0x18)
void CMMIO::Open(CMMMemoryIOInfo& info) {
    m_hmmio = mmioOpen(NULL, &info, MMIO_READWRITE);
}
