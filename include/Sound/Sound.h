#ifndef GITEN_SOUND_SOUND_H
#define GITEN_SOUND_SOUND_H

#include <rva.h>

#include <Ints.h>

// Sound effects and music.

RVA_DECL(0x00048b80)
i16 MapEffectSoundId(i16 id);

RVA_DECL(0x000489f0)
void PlaySoundEffect(i16 sound);

i16 MapSoundEffectId(i16 id);

// @identity-TODO: Every caller pushes a second argument (e.g. 1) that the body never reads; its
// role is unrecovered.
RVA_DECL(0x00049a60)
i16 PlayMusic(i16 track, i16 loop);

// The track PlayMusic last started.
RVA_DECL(0x00049b70)
i16 CurrentMusicTrack(void);

#endif // GITEN_SOUND_SOUND_H
