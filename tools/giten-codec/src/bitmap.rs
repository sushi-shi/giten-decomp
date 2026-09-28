//! Giten's uncompressed, bottom-up, 8-bit BMP/DIB resources.
use crate::{Error, Result, bytes, u16_at, u32_at};

#[derive(Clone, Copy, Debug)]
pub struct Bitmap<'a> {
    pub width: usize,
    pub height: usize,
    pub palette: &'a [u8],
    pub pixels: &'a [u8],
    pub colors_used: usize,
}
#[derive(Clone, Copy, Debug)]
pub struct PixelFormat {
    pub shifts: [u8; 3],
    pub losses: [u8; 3],
}
impl PixelFormat {
    pub const RGB565: Self = Self {
        shifts: [11, 5, 0],
        losses: [3, 2, 3],
    };
    pub const RGB555: Self = Self {
        shifts: [10, 5, 0],
        losses: [3, 3, 3],
    };
    pub fn pack(self, rgb: [u8; 3]) -> u16 {
        rgb.into_iter().zip(self.shifts).zip(self.losses).fold(
            0u32,
            |v, ((channel, shift), loss)| {
                v | u32::from(channel)
                    .wrapping_shr(u32::from(loss))
                    .wrapping_shl(u32::from(shift))
            },
        ) as u16
    }
}
impl<'a> Bitmap<'a> {
    pub fn from_bmp(data: &'a [u8]) -> Result<Self> {
        if bytes(data, 0, 2)? != b"BM" {
            return Err(Error::Invalid);
        }
        let size = u32_at(data, 2)? as usize;
        let file = bytes(data, 0, size)?;
        Self::parse(
            bytes(file, 14, file.len().checked_sub(14).ok_or(Error::Invalid)?)?,
            file.get(u32_at(file, 10)? as usize..)
                .ok_or(Error::Truncated)?,
        )
    }
    pub fn from_dib(data: &'a [u8]) -> Result<Self> {
        // The game's PE resource reader uses a fixed 256-entry palette.
        Self::parse(data, data.get(40 + 1024..).ok_or(Error::Truncated)?)
    }
    fn parse(info: &'a [u8], pixels: &'a [u8]) -> Result<Self> {
        if u32_at(info, 0)? != 40
            || u16_at(info, 12)? != 1
            || u16_at(info, 14)? != 8
            || u32_at(info, 16)? != 0
        {
            return Err(Error::Unsupported);
        }
        let width = u32_at(info, 4)? as i32;
        let height = u32_at(info, 8)? as i32;
        if width <= 0 || height <= 0 {
            return Err(Error::Unsupported);
        }
        let (width, height) = (width as usize, height as usize);
        // Giten walks width bytes per row even when a standard BMP has row
        // padding. Preserve that observable quirk for the original resources.
        let count = match u32_at(info, 32)? {
            0 => 256,
            n => n as usize,
        };
        if count > 256 {
            return Err(Error::Invalid);
        }
        let palette = bytes(info, 40, count * 4)?;
        let pixels = bytes(pixels, 0, width.checked_mul(height).ok_or(Error::Overflow)?)?;
        Ok(Self {
            width,
            height,
            palette,
            pixels,
            colors_used: count,
        })
    }
    pub fn color(&self, index: usize) -> [u8; 3] {
        match self.palette.get(index * 4..index * 4 + 4) {
            Some(q) => [q[2], q[1], q[0]],
            None => [0; 3],
        }
    }
    pub fn color_key(&self) -> u32 {
        let [r, g, b] = self.color(1);
        (u32::from(r & 31) << 16) | (u32::from(g & 31) << 8) | u32::from(b & 31)
    }
    pub fn palette_entries(&self, output: &mut [u8; 1024]) -> Result<()> {
        if self.palette.len() < 1024 {
            return Err(Error::Unsupported);
        }
        for (q, entry) in self.palette.chunks_exact(4).zip(output.chunks_exact_mut(4)) {
            entry.copy_from_slice(&[q[2], q[1], q[0], 1]);
        }
        Ok(())
    }
    /// Writes active pixels only, preserving pitch padding and the surrounding surface.
    pub fn blit(
        &self,
        output: &mut [u8],
        pitch: usize,
        x: usize,
        y: usize,
        format: Option<PixelFormat>,
    ) -> Result<()> {
        let pixel_size = if format.is_some() { 2 } else { 1 };
        let end_x = x
            .checked_add(self.width)
            .and_then(|v| v.checked_mul(pixel_size))
            .ok_or(Error::Overflow)?;
        let end_y = y.checked_add(self.height).ok_or(Error::Overflow)?;
        if pitch < end_x || output.len() < end_y.checked_mul(pitch).ok_or(Error::Overflow)? {
            return Err(Error::OutputTooSmall);
        }
        let mut colors = [0u16; 256];
        if let Some(f) = format {
            for (i, color) in colors.iter_mut().enumerate().take(self.colors_used) {
                *color = f.pack(self.color(i));
            }
        }
        for row in 0..self.height {
            let src = &self.pixels[(self.height - 1 - row) * self.width..][..self.width];
            let dst = &mut output[(y + row) * pitch + x * pixel_size..][..self.width * pixel_size];
            if format.is_some() {
                for (&index, pixel) in src.iter().zip(dst.chunks_exact_mut(2)) {
                    pixel.copy_from_slice(&colors[index as usize].to_le_bytes());
                }
            } else {
                dst.copy_from_slice(src);
            }
        }
        Ok(())
    }
}

/// Consecutive complete BMP records, advancing by bfSize rather than pixel count.
pub struct Bitmaps<'a>(pub &'a [u8]);
impl<'a> Iterator for Bitmaps<'a> {
    type Item = Result<&'a [u8]>;
    fn next(&mut self) -> Option<Self::Item> {
        if self.0.is_empty() {
            return None;
        }
        let result = (|| {
            if bytes(self.0, 0, 2)? != b"BM" {
                return Err(Error::Invalid);
            }
            let size = u32_at(self.0, 2)? as usize;
            if size < 54 {
                return Err(Error::Invalid);
            }
            bytes(self.0, 0, size)
        })();
        match result {
            Ok(bmp) => self.0 = &self.0[bmp.len()..],
            Err(_) => self.0 = &[],
        }
        Some(result)
    }
}
