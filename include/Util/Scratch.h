#ifndef GITEN_UTIL_SCRATCH_H
#define GITEN_UTIL_SCRATCH_H

// Shared text and file-name work buffer. The next distinct retail object
// starts 0x200 bytes after it.
extern char g_scratchBuffer[0x200];

#endif // GITEN_UTIL_SCRATCH_H
