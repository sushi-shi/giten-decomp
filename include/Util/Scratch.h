#ifndef GITEN_UTIL_SCRATCH_H
#define GITEN_UTIL_SCRATCH_H

// @identity-TODO: a shared 0x3400-byte work buffer (file names, blit
// staging, save-file header text) used from many modules; its owning
// definition is not claimed yet.
extern char g_scratchBuffer[];

#endif // GITEN_UTIL_SCRATCH_H
