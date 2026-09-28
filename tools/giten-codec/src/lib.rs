#![no_std]
#![forbid(unsafe_code)]
//! Borrowed resource readers and allocation-free Giten codec implementations.
//!
//! Strings remain bytes (the game uses Shift-JIS). Malformed inputs return an
//! error; reproducing retail's out-of-bounds reads is deliberately not an API.

pub mod area;
pub mod bitmap;
pub mod crypt;
pub mod item;
pub mod mids;
pub mod riff;
pub mod text;
pub mod wave;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Error {
    Truncated,
    Invalid,
    Unsupported,
    OutputTooSmall,
    Overflow,
}
pub type Result<T> = core::result::Result<T, Error>;

pub(crate) fn bytes(data: &[u8], offset: usize, size: usize) -> Result<&[u8]> {
    data.get(offset..offset.checked_add(size).ok_or(Error::Overflow)?)
        .ok_or(Error::Truncated)
}
pub(crate) fn u16_at(data: &[u8], offset: usize) -> Result<u16> {
    Ok(u16::from_le_bytes(
        bytes(data, offset, 2)?.try_into().unwrap(),
    ))
}
pub(crate) fn u32_at(data: &[u8], offset: usize) -> Result<u32> {
    Ok(u32::from_le_bytes(
        bytes(data, offset, 4)?.try_into().unwrap(),
    ))
}
pub(crate) fn c_string(data: &[u8]) -> Result<(&[u8], &[u8])> {
    let end = data.iter().position(|&b| b == 0).ok_or(Error::Truncated)?;
    Ok((&data[..end], &data[end + 1..]))
}

#[cfg(test)]
mod tests;
