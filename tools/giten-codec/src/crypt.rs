//! Length-prefixed records with a ciphertext-feedback XOR key.
use crate::{Error, Result, bytes, u16_at};

#[derive(Clone, Copy, Debug)]
pub struct Key(u8);
impl Key {
    pub const fn new(length: u16) -> Self {
        Self((length as u8) ^ ((length >> 8) as u8))
    }
    pub fn decode(&mut self, cipher: u8) -> u8 {
        let plain = cipher ^ self.0;
        self.0 = cipher;
        plain
    }
    pub fn encode(&mut self, plain: u8) -> u8 {
        self.0 ^= plain;
        self.0
    }
    pub fn decode_in_place(&mut self, data: &mut [u8]) {
        for byte in data {
            *byte = self.decode(*byte);
        }
    }
}

/// Returns the decoded byte count and the unconsumed input.
pub fn decode_record<'a>(input: &'a [u8], output: &mut [u8]) -> Result<(usize, &'a [u8])> {
    let length = u16_at(input, 0)? as usize;
    let payload = bytes(input, 2, length)?;
    let target = output.get_mut(..length).ok_or(Error::OutputTooSmall)?;
    target.copy_from_slice(payload);
    Key::new(length as u16).decode_in_place(target);
    Ok((length, &input[length + 2..]))
}

/// The inverse format operation. Retail has no corresponding resource writer.
pub fn encode_record(input: &[u8], output: &mut [u8]) -> Result<usize> {
    let length = u16::try_from(input.len()).map_err(|_| Error::Overflow)?;
    let target = output
        .get_mut(..input.len() + 2)
        .ok_or(Error::OutputTooSmall)?;
    target[..2].copy_from_slice(&length.to_le_bytes());
    let mut key = Key::new(length);
    for (plain, cipher) in input.iter().zip(&mut target[2..]) {
        *cipher = key.encode(*plain);
    }
    Ok(target.len())
}
