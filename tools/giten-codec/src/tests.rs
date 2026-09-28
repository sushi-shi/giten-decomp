use super::*;

#[test]
fn cipher_feedback_and_split_state() {
    let mut key = crypt::Key::new(3);
    assert_eq!(key.decode(3), 0);
    let mut rest = [1, 2];
    key.decode_in_place(&mut rest);
    assert_eq!(rest, [2, 3]);
    let mut encoded = [0; 5];
    assert_eq!(crypt::encode_record(&[0, 2, 3], &mut encoded), Ok(5));
    assert_eq!(encoded, [3, 0, 3, 1, 2]);
    let mut output = [0xcd; 4];
    assert_eq!(
        crypt::decode_record(&encoded, &mut output),
        Ok((3, &[][..]))
    );
    assert_eq!(output, [0, 2, 3, 0xcd]);
}
#[test]
fn record_bounds_precede_writes() {
    let mut output = [0xcd; 2];
    assert_eq!(
        crypt::decode_record(&[3, 0, 1], &mut output),
        Err(Error::Truncated)
    );
    assert_eq!(
        crypt::decode_record(&[3, 0, 1, 2, 3], &mut output),
        Err(Error::OutputTooSmall)
    );
    assert_eq!(output, [0xcd; 2]);
}
#[test]
fn midi_partial_writes_are_observable() {
    let mut output = [0xcd; 20];
    assert_eq!(
        mids::convert(&[1, 0, 0, 0], &mut output),
        Err(Error::Truncated)
    );
    assert_eq!(&output[..8], &[1, 0, 0, 0, 0, 0, 0, 0]);
    assert_eq!(&output[8..], &[0xcd; 12]);
    assert_eq!(mids::convert(&[1], &mut output), Err(Error::Invalid));
}
#[test]
fn midi_long_event_keeps_padding() {
    let input = [7, 0, 0, 0, 1, 0, 0, 0x80, 0x55, 0xaa, 0xbb, 0xcc];
    let mut output = [0xcd; 20];
    assert_eq!(mids::convert(&input, &mut output), Ok(16));
    assert_eq!(&output[12..16], &[0x55, 0xaa, 0xbb, 0xcc]);
    assert_eq!(&output[16..], &[0xcd; 4]);
}
#[test]
fn palette_channel_order_and_padded_surface() {
    let mut dib = [0u8; 40 + 1024 + 8];
    dib[0] = 40;
    dib[4] = 4;
    dib[8] = 2;
    dib[12] = 1;
    dib[14] = 8;
    dib[44..48].copy_from_slice(&[0xf8, 0xfc, 0xf8, 0]);
    dib[1064..1072].copy_from_slice(&[0, 0, 0, 0, 1, 1, 1, 1]);
    let bitmap = bitmap::Bitmap::from_dib(&dib).unwrap();
    let mut output = [0xcd; 24];
    bitmap
        .blit(&mut output, 12, 1, 0, Some(bitmap::PixelFormat::RGB565))
        .unwrap();
    assert_eq!(&output[2..10], &[0xff; 8]);
    assert_eq!(&output[14..22], &[0; 8]);
    assert_eq!(&output[10..14], &[0xcd; 4]);
}
#[test]
fn riff_odd_chunk_padding_and_truncation() {
    let valid = *b"RIFF\x0e\0\0\0TESTdata\x01\0\0\0x\0";
    let mut chunks = riff::Riff::parse(&valid).unwrap().chunks;
    assert_eq!(chunks.next().unwrap().unwrap().data, b"x");
    assert!(chunks.next().is_none());
    assert!(riff::Riff::parse(&valid[..valid.len() - 1]).is_err());
}
#[test]
fn malformed_views_return_errors() {
    for length in 0..64 {
        let input = [0xff; 64];
        let data = &input[..length];
        assert!(bitmap::Bitmap::from_bmp(data).is_err());
        assert!(bitmap::Bitmap::from_dib(data).is_err());
        assert!(area::Area::parse(data).is_err());
        assert!(item::Item::from_table(data, 0).is_err());
        assert!(mids::Mids::parse(data).is_err());
        assert!(wave::Wave::parse(data).is_err());
    }
}

#[test]
fn text_escapes_and_unpaired_leads() {
    assert_eq!(text::read_char(&[0x81, 0]), Ok((0x81a6, 1)));
    assert_eq!(text::read_char(&[0xfe, 0]), Ok((0xfe00, 2)));
    assert_eq!(text::read_char(&[0xff]), Err(Error::Truncated));
    assert_eq!(text::read_char(&[0]), Ok((0, 0)));
}
