//! Host-only driver for the same binary jobs as the period-compiler harness.
use giten_codec::{
    area::Area,
    bitmap::{Bitmap, PixelFormat},
    crypt,
    item::Item,
    mids,
};
use std::{
    fs::File,
    io::{self, BufReader, BufWriter, Read, Write},
};

fn read_word(r: &mut impl Read) -> io::Result<u32> {
    let mut b = [0; 4];
    r.read_exact(&mut b)?;
    Ok(u32::from_le_bytes(b))
}
fn word(out: &mut Vec<u8>, value: u32) {
    out.extend(value.to_le_bytes());
}
fn evaluate(kind: u32, arg: usize, input: &[u8]) -> Result<Vec<u8>, String> {
    let err = |e| format!("{e:?}");
    let mut out = Vec::new();
    match kind {
        1 | 10 => {
            let size = u16::from_le_bytes(input[..2].try_into().unwrap()) as usize;
            out.resize(size + 16, 0xcd);
            if kind == 1 {
                crypt::decode_record(input, &mut out).map_err(err)?;
            } else {
                out[..size].copy_from_slice(&input[2..2 + size]);
            }
        }
        2 | 3 => {
            let resource = kind == 3;
            let bmp = if resource {
                Bitmap::from_dib(input)
            } else {
                Bitmap::from_bmp(input)
            }
            .map_err(err)?;
            let eight = arg == 8;
            let format = if eight {
                None
            } else {
                Some(if arg == 555 {
                    PixelFormat::RGB555
                } else {
                    PixelFormat::RGB565
                })
            };
            let pitch = (bmp.width + 8) * if eight { 1 } else { 2 };
            let mut pixels = vec![0xcd; pitch * (bmp.height + 4)];
            bmp.blit(
                &mut pixels,
                pitch,
                if resource { 3 } else { 0 },
                if resource { 2 } else { 0 },
                format,
            )
            .map_err(err)?;
            let key = if resource {
                0
            } else if eight {
                1
            } else {
                bmp.color_key()
            };
            word(&mut out, 1);
            word(&mut out, if !resource && !eight { key } else { 0xcdcdcdcd });
            word(&mut out, key);
            word(&mut out, key);
            for value in [1, 1, 8, u32::from(eight), if eight { 0x44 } else { 0 }] {
                word(&mut out, value);
            }
            let mut palette = [0xcd; 1024];
            if eight {
                bmp.palette_entries(&mut palette).map_err(err)?;
            }
            out.extend(palette);
            out.extend(palette);
            out.extend(pixels);
        }
        4 => {
            let mut buffer = vec![0xcd; arg + 16];
            let result = mids::convert(input, &mut buffer[..arg]);
            word(&mut out, u32::from(result.is_ok()));
            word(&mut out, result.map(|n| n as u32).unwrap_or(0xcdcdcdcd));
            out.extend(buffer);
        }
        5 => {
            let mut record = input.to_vec();
            record.resize(0x2800 + 56, 0);
            let area = Area::parse(&record).map_err(err)?;
            out.resize(0x4000, 0xcd);
            area.expand_offsets(&mut out).map_err(err)?;
        }
        6 => {
            let item = Item::from_table(input, arg as i16).map_err(err)?;
            out.extend(item.id.to_le_bytes());
            out.extend(item.price.to_le_bytes());
            out.push(item.kind);
            out.extend(item.params);
            out.extend(item.before);
            out.extend(item.after);
            out.extend(item.name);
            out.push(0);
            out.extend(item.description);
            out.push(0);
        }
        11 => {
            let mut padded = input.to_vec();
            padded.push(0);
            for pos in (0..input.len()).step_by(arg) {
                let (code, consumed) = giten_codec::text::read_char(&padded[pos..]).map_err(err)?;
                out.extend((consumed as u16).to_le_bytes());
                out.extend(code.to_le_bytes());
            }
        }
        9 => {
            let wave = giten_codec::wave::Wave::parse(&input[4..]).map_err(err)?;
            let size = wave.samples.len();
            let long = size > 0x19000;
            let capacity = if long { 44100 * 9 } else { 0x204cc };
            let resource = u32::from_le_bytes(input[..4].try_into().unwrap());
            for value in [
                u32::from(long),
                0,
                size as u32,
                0,
                size as u32,
                0,
                u32::from(arg == 0x52 || arg == 0x5d),
                resource,
            ] {
                word(&mut out, value);
            }
            let mut buffer = vec![0; capacity + 16];
            buffer[capacity..].fill(0xcd);
            buffer[..size].copy_from_slice(wave.samples);
            out.extend(buffer);
        }
        8 => {
            let file = mids::Mids::parse(input).map_err(err)?;
            for value in [
                file.format.time_format,
                file.format.max_buffer,
                file.format.flags,
                file.buffer_count,
            ] {
                word(&mut out, value);
            }
            for buffer in file.buffers() {
                let buffer = buffer.map_err(err)?;
                let mut bytes = vec![0; file.format.max_buffer as usize];
                let length = file.decode_buffer(&buffer, &mut bytes).map_err(err)?;
                word(&mut out, file.format.max_buffer);
                word(&mut out, length as u32);
                out.extend(&bytes[..length]);
            }
        }
        7 => {
            let mut position = 0usize;
            let mut size = 0xcdcdcdcd;
            let mut success = false;
            let mut skip = arg;
            loop {
                if input.len().saturating_sub(position) < 14 {
                    position = input.len();
                    break;
                }
                let header = &input[position..position + 14];
                position += 14;
                if &header[..2] != b"BM" {
                    break;
                }
                let record_size = u32::from_le_bytes(header[2..6].try_into().unwrap());
                if skip == 0 {
                    position -= 14;
                    size = record_size;
                    success = true;
                    break;
                }
                position += record_size as usize - 14;
                if position >= input.len() {
                    break;
                }
                skip -= 1;
            }
            word(&mut out, u32::from(success));
            word(&mut out, position as u32);
            word(&mut out, size);
        }
        _ => return Err(format!("unknown job kind {kind}")),
    }
    Ok(out)
}
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<_> = std::env::args_os().collect();
    if args.len() != 3 {
        return Err("codec-oracle jobs.bin results.bin".into());
    }
    let mut jobs = BufReader::new(File::open(&args[1])?);
    let mut out = BufWriter::new(File::create(&args[2])?);
    if read_word(&mut jobs)? != 0x424f4a47 {
        return Err("bad magic".into());
    }
    let count = read_word(&mut jobs)?;
    out.write_all(&0x53455247u32.to_le_bytes())?;
    out.write_all(&count.to_le_bytes())?;
    for index in 0..count {
        let kind = read_word(&mut jobs)?;
        let arg = read_word(&mut jobs)? as usize;
        let size = read_word(&mut jobs)? as usize;
        if size > 32 * 1024 * 1024 {
            return Err("job too large".into());
        }
        let mut input = vec![0; size];
        jobs.read_exact(&mut input)?;
        let result =
            evaluate(kind, arg, &input).map_err(|e| format!("job {index} kind {kind}: {e}"))?;
        out.write_all(&1u32.to_le_bytes())?;
        out.write_all(&(result.len() as u32).to_le_bytes())?;
        out.write_all(&result)?;
    }
    let mut extra = [0];
    if jobs.read(&mut extra)? != 0 {
        return Err("trailing jobs".into());
    }
    out.flush()?;
    eprintln!("{count} Rust jobs completed");
    Ok(())
}
