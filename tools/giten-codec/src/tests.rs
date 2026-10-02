use super::*;

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
