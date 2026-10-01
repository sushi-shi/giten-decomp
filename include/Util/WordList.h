#ifndef GITEN_UTIL_WORDLIST_H
#define GITEN_UTIL_WORDLIST_H

#include <rva.h>

#include <Ints.h>

#include <stddef.h>

// A counted word list (count, then an allocated array): InitWordList allocates
// it, ResetWordList frees and reallocates it.
typedef struct WordList {
    i16 count;
    i16* words;
} WordList;

#define InitEmptyWordList(list)                                                                    \
    do {                                                                                           \
        (list)->count = 0;                                                                         \
        (list)->words = NULL;                                                                      \
    } while (0)

static __inline i16 GetWordCount(const WordList* list) {
    return list->count;
}

static __inline i16* GetWordArray(const WordList* list) {
    return list->words;
}

static __inline i16 GetWord(const WordList* list, i16 index) {
    return list->words[index];
}

#define SetWord(list, index, value) ((list)->words[index] = (value))

RVA_DECL(0x0002dbf0)
void InitWordList(WordList* list, i16 count);

void FreeWordList(WordList* list);

RVA_DECL(0x0002dc40)
void ResetWordList(WordList* list, i16 count);

// FindWord and RemoveWord report an absent word as WORD_NONE; MoveWord takes
// WORD_LAST for the last slot.
#define WORD_NONE (-1)
#define WORD_LAST (-1)

// Index of a matching word, or WORD_NONE.
RVA_DECL(0x0002dca0)
i16 FindWord(WordList* list, i16 word);

#define ContainsWord(list, word) (FindWord((list), (word)) >= 0)

// Move the word at `from` to `to` (or WORD_LAST).
RVA_DECL(0x0002dd80)
void MoveWord(WordList* list, i16 from, i16 to);

// Remove a matching word from the list; WORD_NONE if absent.
RVA_DECL(0x0002de10)
i16 RemoveWord(WordList* list, i16 word);

i16* CopyWordArray(i16* words, i16 count);

void StripZeroWords(WordList* list);
i16 KeepFirstSixWords(WordList* list);

#endif // GITEN_UTIL_WORDLIST_H
