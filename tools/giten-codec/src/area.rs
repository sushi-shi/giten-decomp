//! Area records contain 16-bit offsets; the Windows runtime expands them to
//! pointers. This view retains offsets and borrows the original record.
use crate::{Error, Result, bytes, c_string, u16_at};

pub struct Area<'a> {
    data: &'a [u8],
    pub level_count: usize,
    pub name: &'a [u8],
}
pub struct Level<'a> {
    pub offset: usize,
    pub header: &'a [u8],
}
impl<'a> Area<'a> {
    pub fn parse(data: &'a [u8]) -> Result<Self> {
        let level_count = u16_at(data, 4)? as usize;
        if level_count > 32 {
            return Err(Error::Invalid);
        }
        bytes(data, 6, level_count * 2)?;
        let name_offset = u16_at(data, 2)? as usize;
        let (name, _) = c_string(data.get(name_offset..).ok_or(Error::Truncated)?)?;
        Ok(Self {
            data,
            level_count,
            name,
        })
    }
    pub fn level(&self, index: usize) -> Result<Level<'a>> {
        if index >= self.level_count {
            return Err(Error::Invalid);
        }
        let offset = u16_at(self.data, 6 + index * 2)? as usize;
        Ok(Level {
            offset,
            header: bytes(self.data, offset, 56)?,
        })
    }
    /// Wire form of the Windows expanded layout with pointers represented as
    /// 32-bit offsets. `output` retains untouched bytes, including the area id.
    /// The caller supplies the zero-filled 0x2800-byte backing record used by
    /// the game's loader (plus 56 readable bytes for its final copy).
    pub fn expand_offsets(&self, output: &mut [u8]) -> Result<()> {
        let shift = self.level_count * 2 + 2;
        write(output, 2, u16_at(self.data, 2)? as u32)?;
        copy(output, 6, &(self.level_count as u16).to_le_bytes())?;
        for index in 0..self.level_count {
            let level = self.level(index)?;
            let dst = index * 28 + shift + level.offset;
            let base = (index + 1) * 28 + shift;
            write(output, 8 + index * 4, dst as u32)?;
            for field in 0..13 {
                write(
                    output,
                    dst + field * 4,
                    (base + u16_at(level.header, field * 2)? as usize) as u32,
                )?;
            }
            copy(output, dst + 52, &level.header[26..54])?;
            write(
                output,
                dst + 80,
                (base + u16_at(level.header, 54)? as usize) as u32,
            )?;
            let end = if index + 1 < self.level_count {
                self.level(index + 1)?.offset
            } else {
                0x2800
            };
            let length = end.checked_sub(level.offset).ok_or(Error::Invalid)?;
            copy(
                output,
                dst + 84,
                bytes(self.data, level.offset + 56, length)?,
            )?;
        }
        Ok(())
    }
}
fn copy(output: &mut [u8], offset: usize, data: &[u8]) -> Result<()> {
    output
        .get_mut(offset..offset + data.len())
        .ok_or(Error::OutputTooSmall)?
        .copy_from_slice(data);
    Ok(())
}
fn write(output: &mut [u8], offset: usize, value: u32) -> Result<()> {
    copy(output, offset, &value.to_le_bytes())
}
