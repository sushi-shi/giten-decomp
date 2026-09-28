//! Item tables share a price/kind prefix and two NUL-terminated Shift-JIS
//! strings. The parameter order depends on the item kind, not a fixed struct.
use crate::{Error, Result, bytes, c_string, u16_at, u32_at};

pub struct Item<'a> {
    pub id: i16,
    pub price: i32,
    pub kind: u8,
    pub params: [u8; 51],
    pub before: [u8; 2],
    pub after: [u8; 2],
    pub name: &'a [u8],
    pub description: &'a [u8],
}
struct Fields<'a> {
    rest: &'a [u8],
    params: [u8; 51],
}
impl Fields<'_> {
    fn read(&mut self, indices: &[usize]) -> Result<()> {
        for &index in indices {
            let (&byte, tail) = self.rest.split_first().ok_or(Error::Truncated)?;
            self.params[index] = byte;
            self.rest = tail;
        }
        Ok(())
    }
    fn pair(&mut self) -> Result<[u8; 2]> {
        let pair = bytes(self.rest, 0, 2)?.try_into().unwrap();
        self.rest = &self.rest[2..];
        Ok(pair)
    }
}
impl<'a> Item<'a> {
    pub fn from_table(table: &'a [u8], id: i16) -> Result<Self> {
        let count = u16_at(table, 0)? as i16;
        let index = if id < 0 || id >= count {
            1
        } else {
            id as usize
        };
        let offset = u16_at(table, 2 + index * 2)? as usize;
        Self::parse(table.get(offset..).ok_or(Error::Truncated)?, id)
    }
    pub fn parse(data: &'a [u8], id: i16) -> Result<Self> {
        let price = u32_at(data, 0)? as i32;
        let kind = *data.get(4).ok_or(Error::Truncated)?;
        let mut f = Fields {
            rest: &data[5..],
            params: [0; 51],
        };
        f.params[0x15] = 255;
        let (mut before, mut after) = ([0; 2], [0; 2]);
        const VALUE: &[usize] = &[0, 1, 2, 3];
        const TARGET: &[usize] = &[4, 5, 6];
        const EQUIP: &[usize] = &[0x15, 0x16, 0x17, 0x18];
        let mut messages = 0;
        match kind {
            1 | 9 => {
                f.read(VALUE)?;
                f.read(TARGET)?;
                if kind == 9 {
                    f.read(&[0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x31, 0x31])?;
                    f.params[0xa] = f.params[0x10];
                }
                f.read(&[7, 8, 9, 0xa, 0x32])?;
                if kind == 1 {
                    f.params[0x10] = f.params[0xa];
                }
                messages = 2;
            }
            2 => {
                f.read(VALUE)?;
                f.read(TARGET)?;
                f.read(&[0xb])?;
            }
            3 => {
                f.read(VALUE)?;
                f.read(TARGET)?;
                messages = 2;
            }
            4 => {
                f.read(VALUE)?;
                f.read(TARGET)?;
                f.read(&[0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x32])?;
                f.params[0xa] = f.params[0x10];
                messages = 2;
            }
            5 => {
                f.read(VALUE)?;
                f.read(TARGET)?;
                f.read(&[0x11])?;
                messages = 2;
            }
            6 => {
                f.read(VALUE)?;
                f.read(&[4, 5, 0x12, 0x13])?;
            }
            7 => {
                f.read(VALUE)?;
                f.read(TARGET)?;
                f.read(&[0x14])?;
            }
            8 | 10 => {
                f.read(VALUE)?;
                f.read(TARGET)?;
            }
            11 => {
                f.read(EQUIP)?;
                f.read(&[
                    0x19, 0x1a, 0x1b, 0x1c, 0x21, 6, 0x1d, 0x1e, 0x1f, 0x20, 0x22, 0x23, 0x24,
                    0x25, 0x26, 0, 1, 0x13, 3, 0x2d, 0x2e,
                ])?;
                messages = 1;
            }
            12 => {
                f.read(EQUIP)?;
                f.read(&[
                    0x19, 0x1b, 0x1c, 0x1f, 0x20, 0x22, 0x23, 0x27, 0x28, 0x29, 0x2a, 0x2b, 6, 0x32,
                ])?;
            }
            13 => {
                f.read(EQUIP)?;
                f.read(&[0x19, 0x21, 0x24, 0x27, 0x32])?;
            }
            14..=18 => {
                f.read(EQUIP)?;
                f.read(&[0x1a, 0x2c, 0x2f, 0x30, 0x21, 0x1f, 0x22, 0x23, 0x25])?;
            }
            19 => {
                f.read(EQUIP)?;
                f.read(&[0x25, 0, 1, 0x13, 3])?;
                messages = 1;
                f.params[0xb] = 11;
            }
            _ => {}
        }
        if messages >= 1 {
            before = f.pair()?;
        }
        if messages >= 2 {
            after = f.pair()?;
        }
        let (name, tail) = c_string(f.rest)?;
        let (description, _) = c_string(tail)?;
        Ok(Self {
            id,
            price,
            kind,
            params: f.params,
            before,
            after,
            name,
            description,
        })
    }
}
