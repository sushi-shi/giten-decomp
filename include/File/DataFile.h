#ifndef GITEN_FILE_DATAFILE_H
#define GITEN_FILE_DATAFILE_H

#include <rva.h>

#include <Ints.h>

#include <stdio.h>

void SetCryptKey(u16 seed);
u8 ReadCryptByte(FILE* fp);
u16 ReadCryptBytes(FILE* fp, u16 count, u8* buf);
void* ReadCryptRecord(FILE* fp, void* out);
void SkipBytes(FILE* fp, u16 count);
void ReadRawBlock(FILE* fp, void* buf);
FILE* OpenDataFile(i16 id, i32 kind, i16 variant);
b32 CloseDataFile(FILE* fp);

// Reads a little-endian encrypted word.
u16 ReadCryptWord(FILE* fp);

// Reads a word (the decryption seed and length), then that many encrypted
// bytes into `out`.
void* ReadCryptRecord(FILE* fp, void* out);

// Reads a word count, then that many encrypted bytes into a new memory handle.
i32 ReadCryptHandle(FILE* fp);

// Skips a block that starts with its word length.
void SkipRawBlock(FILE* fp);

// Reads a word count, then that many plain bytes into a new memory handle.
i32 ReadRawHandle(FILE* fp);

// Reads a word count, then that many plain bytes into a new block.
void* ReadRawAlloc(FILE* fp);

#endif // GITEN_FILE_DATAFILE_H
