//! Game text tokens under code page 932, including 0xfe/0xff escapes.
//! These are encoded character codes, not Unicode scalar values.
use crate::{Error, Result};

pub const fn is_lead(byte: u8) -> bool {
    matches!(byte, 0x81..=0x9f | 0xe0..=0xfc)
}

/// Returns (character code, bytes consumed). A NUL consumes no bytes. A lead
/// byte immediately followed by NUL becomes the game's 0x81a6 replacement.
pub fn read_char(input: &[u8]) -> Result<(u16, usize)> {
    let first = *input.first().ok_or(Error::Truncated)?;
    if first == 0 {
        return Ok((0, 0));
    }
    if first >= 0xfe || is_lead(first) {
        let second = *input.get(1).ok_or(Error::Truncated)?;
        if first < 0xfe && second == 0 {
            return Ok((0x81a6, 1));
        }
        Ok((u16::from_be_bytes([first, second]), 2))
    } else {
        Ok((u16::from(first), 1))
    }
}
