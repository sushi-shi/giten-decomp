//! Embedded PCM WAVE resources. Playback itself belongs to DirectSound.
use crate::{Error, Result, bytes, u16_at, u32_at};

pub struct Wave<'a> {
    pub channels: u16,
    pub sample_rate: u32,
    pub bits_per_sample: u16,
    pub samples: &'a [u8],
}
impl<'a> Wave<'a> {
    /// The game consumes a fixed PCM header, ignoring the outer RIFF length.
    /// Some original resources overstate that length; the data chunk length
    /// still bounds their actual sample payload correctly.
    pub fn parse(data: &'a [u8]) -> Result<Self> {
        if bytes(data, 0, 4)? != b"RIFF"
            || bytes(data, 8, 8)? != b"WAVEfmt "
            || bytes(data, 36, 4)? != b"data"
        {
            return Err(Error::Invalid);
        }
        if u32_at(data, 16)? != 16 || u16_at(data, 20)? != 1 {
            return Err(Error::Unsupported);
        }
        let wave = Self {
            channels: u16_at(data, 22)?,
            sample_rate: u32_at(data, 24)?,
            bits_per_sample: u16_at(data, 34)?,
            samples: bytes(data, 44, u32_at(data, 40)? as usize)?,
        };
        let align = usize::from(u16_at(data, 32)?);
        if wave.channels == 0 || align == 0 || !wave.samples.len().is_multiple_of(align) {
            return Err(Error::Invalid);
        }
        Ok(wave)
    }
}
