//! Bounded RIFF chunk traversal, including odd-length chunk padding.
use crate::{Error, Result, bytes, u32_at};

pub struct Riff<'a> {
    pub kind: [u8; 4],
    pub chunks: Chunks<'a>,
}
pub struct Chunks<'a>(&'a [u8]);
pub struct Chunk<'a> {
    pub tag: [u8; 4],
    pub data: &'a [u8],
}
impl<'a> Riff<'a> {
    pub fn parse(data: &'a [u8]) -> Result<Self> {
        if bytes(data, 0, 4)? != b"RIFF" {
            return Err(Error::Invalid);
        }
        let body = bytes(data, 8, u32_at(data, 4)? as usize)?;
        Ok(Self {
            kind: bytes(body, 0, 4)?.try_into().unwrap(),
            chunks: Chunks(&body[4..]),
        })
    }
}
impl<'a> Iterator for Chunks<'a> {
    type Item = Result<Chunk<'a>>;
    fn next(&mut self) -> Option<Self::Item> {
        if self.0.is_empty() {
            return None;
        }
        let result = (|| {
            let tag = bytes(self.0, 0, 4)?.try_into().unwrap();
            let size = u32_at(self.0, 4)? as usize;
            let data = bytes(self.0, 8, size)?;
            let consumed = 8usize
                .checked_add(size)
                .and_then(|n| n.checked_add(size & 1))
                .ok_or(Error::Overflow)?;
            self.0 = self.0.get(consumed..).ok_or(Error::Truncated)?;
            Ok(Chunk { tag, data })
        })();
        if result.is_err() {
            self.0 = &[];
        }
        Some(result)
    }
}
