//! MIDS packed event streams: insert a zero stream id after each delta time.
use crate::{Error, Result, bytes, u32_at};

/// Matches the retail converter's partial writes when a bounded input/output
/// ends mid-event. On error, bytes already written remain in `output`.
pub fn convert(input: &[u8], output: &mut [u8]) -> Result<usize> {
    if input.len() & 3 != 0 {
        return Err(Error::Invalid);
    }
    let (mut src, mut dst) = (0, 0);
    while src < input.len() {
        if output.len() - dst < 12 {
            return Err(Error::OutputTooSmall);
        }
        output[dst..dst + 4].copy_from_slice(bytes(input, src, 4)?);
        output[dst + 4..dst + 8].fill(0);
        src += 4;
        dst += 8;
        let event = u32_at(input, src)?;
        output[dst..dst + 4].copy_from_slice(&event.to_le_bytes());
        src += 4;
        dst += 4;
        let length = if event & 0x8000_0000 != 0 {
            ((event & 0x00ff_ffff) as usize + 3) & !3
        } else {
            0
        };
        let payload = bytes(input, src, length)?;
        let target = output
            .get_mut(dst..dst + length)
            .ok_or(Error::OutputTooSmall)?;
        target.copy_from_slice(payload);
        src += length;
        dst += length;
    }
    Ok(dst)
}

#[derive(Clone, Copy, Debug)]
pub struct Format {
    pub time_format: u32,
    pub max_buffer: u32,
    pub flags: u32,
}
pub struct Mids<'a> {
    pub format: Format,
    pub buffer_count: u32,
    data: &'a [u8],
}
pub struct Buffer<'a> {
    pub tick_offset: u32,
    pub events: &'a [u8],
}
pub struct Buffers<'a> {
    remaining: u32,
    data: &'a [u8],
}
impl<'a> Mids<'a> {
    pub fn parse(input: &'a [u8]) -> Result<Self> {
        let riff = crate::riff::Riff::parse(input)?;
        if &riff.kind != b"MIDS" {
            return Err(Error::Invalid);
        }
        let (mut format, mut data) = (None, None);
        for chunk in riff.chunks {
            let chunk = chunk?;
            match &chunk.tag {
                b"fmt " => {
                    format = Some(Format {
                        time_format: u32_at(chunk.data, 0)?,
                        max_buffer: u32_at(chunk.data, 4)?,
                        flags: u32_at(chunk.data, 8)?,
                    })
                }
                b"data" => data = Some(chunk.data),
                _ => {}
            }
        }
        let data = data.ok_or(Error::Invalid)?;
        let result = Self {
            format: format.ok_or(Error::Invalid)?,
            buffer_count: u32_at(data, 0)?,
            data: &data[4..],
        };
        // Validate framing before exposing a borrowed iterator.
        let mut check = result.buffers();
        for buffer in check.by_ref() {
            buffer?;
        }
        if !check.data.is_empty() {
            return Err(Error::Invalid);
        }
        Ok(result)
    }
    pub fn buffers(&self) -> Buffers<'a> {
        Buffers {
            remaining: self.buffer_count,
            data: self.data,
        }
    }
    /// Copy stream-id events, or expand the compact grammar according to flags.
    pub fn decode_buffer(&self, buffer: &Buffer<'_>, output: &mut [u8]) -> Result<usize> {
        if self.format.flags & 1 != 0 {
            convert(buffer.events, output)
        } else {
            let target = output
                .get_mut(..buffer.events.len())
                .ok_or(Error::OutputTooSmall)?;
            target.copy_from_slice(buffer.events);
            Ok(target.len())
        }
    }
}
impl<'a> Iterator for Buffers<'a> {
    type Item = Result<Buffer<'a>>;
    fn next(&mut self) -> Option<Self::Item> {
        if self.remaining == 0 {
            return None;
        }
        let result = (|| {
            let tick_offset = u32_at(self.data, 0)?;
            let length = u32_at(self.data, 4)? as usize;
            let events = bytes(self.data, 8, length)?;
            self.data = &self.data[8 + length..];
            self.remaining -= 1;
            Ok(Buffer {
                tick_offset,
                events,
            })
        })();
        if result.is_err() {
            self.remaining = 0;
        }
        Some(result)
    }
}
