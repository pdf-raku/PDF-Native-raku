use v6;

class Lib::PDF::Writer {
    use NativeCall;
    use Lib::PDF :libpdf, :types;

    sub pdf_write_bool(PDF_BOOL $val, Blob $out, size_t $outlen)
        returns Str is encoded('ascii')
        is native(&libpdf) {*};

    sub pdf_write_int(PDF_INT $val, Blob $out, size_t $outlen)
        returns Str is encoded('ascii')
        is native(&libpdf) {*};

    sub pdf_write_real(PDF_REAL $val, Blob $out, size_t $outlen)
        returns Str is encoded('ascii')
        is native(&libpdf) {*};

    method write-bool(Bool(Cool) $val, $buf = Blob[uint8].allocate(8)) {
        pdf_write_bool($val, $buf, $buf.bytes);
    }

    method write-int(Int(Cool) $val, $buf = Blob[uint8].allocate(8)) {
        pdf_write_int($val, $buf, $buf.bytes);
    }

    method write-real(Num(Cool) $val, $buf = Blob[uint8].allocate(32)) {
        pdf_write_real($val, $buf, $buf.bytes);
    }
}
