# Project helper reuse

The [Gruntz helper follow-up](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/helper-source-followup.md) treats repeated owner operations as leads for recovering source helpers. In Giten, compare each candidate with the original instructions and ordered referents before replacing an exact caller. A helper that computes the same eventual address can still read a different field or read it at a different time.

| Existing helper and candidate sites | Current evidence and next step |
| --- | --- |
| `GetNextBitmap` in `include/Gfx/Bitmap.h`; three packed BMP advances in `DecodeLayerImage` and `DecodeLayerImageAlt` in `src/Gfx/layertexture.cpp` | The decoders capture `bitmapSize = bmp->file.bfSize` before `LoadTexture` or `OpenTextureBitmap`, then advance by that saved value. `GetNextBitmap(bmp)` reloads `bfSize` after the call. Replacing all three advances loses both exact matches (100% to 84.67% and 84.39% in a focused compile); the original expressions are retained. Inspect retail's size load across the intervening calls and recover a helper that accepts the captured size only if another site or source witness supports it. |
| `GetNextMidiStreamBuffer` in `include/Sound/MidiStream.h`; the prepared-buffer advance in `CMidiStream::ReadBuffers` in `src/Sound/midistream.cpp` | `ReadBuffers` advances by `m_formats[part].maxBuffer`, while the helper reads `buffer->header.dwBufferLength`. The loop assigns that header field earlier, but replacing the expression loses its exact match (100% to 92.04% in a focused compile). Keep the direct expression until the retail load source and a same-origin helper are established. |

These are source-origin questions, not invitations to retain a helper call merely because the values agree on valid inputs. The existing helper calls in `sprite.cpp`, `effectdraw.cpp`, and the other MIDI paths remain supported at their own sites.
