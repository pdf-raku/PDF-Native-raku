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

    sub pdf_write_literal(Blob $val, size_t $val_len, Blob $out, size_t $outlen)
        returns Str is encoded('ascii')
        is native(&libpdf) {*};

    sub pdf_write_hex_string(Blob $val, size_t $val_len, Blob $out, size_t $outlen)
        returns Str is encoded('ascii')
        is native(&libpdf) {*};

    sub pdf_write_xref(Blob $val, size_t $rows, Blob $out, size_t $outlen)
        returns Str is encoded('ascii')
        is native(&libpdf) {*};

    sub pdf_write_name(Blob[32] $val, size_t $val_len, Blob $out, size_t $outlen)
        returns Str is encoded('ascii')
        is native(&libpdf) {*};

    method write-bool(Bool(Cool) $val, $buf = Blob[uint8].allocate(10)) {
        pdf_write_bool($val, $buf, $buf.bytes);
    }

    method write-int(Int(Cool) $val, $buf = Blob[uint8].allocate(8)) {
        pdf_write_int($val, $buf, $buf.bytes);
    }

    method write-real(Num(Cool) $val, $buf = Blob[uint8].allocate(32)) {
        pdf_write_real($val, $buf, $buf.bytes);
    }

    method write-literal(Str(Cool) $val, Blob $buf? is copy) {
       my Blob[uint8] $enc = $val.encode: "latin-1";
       my \bytes = $enc.bytes;
       $buf //= Blob[uint8].allocate(4 * bytes  +  3);
       pdf_write_literal($enc, bytes, $buf, $buf.bytes);
    }

    method write-hex-string(Str(Cool) $val, Blob $buf? is copy) {
        my Blob[uint8] $enc = $val.encode: "latin-1";
        my \bytes = $enc.bytes;
        $buf //= Blob[uint8].allocate(2 * bytes  +  3);
        pdf_write_hex_string($enc, bytes, $buf, $buf.bytes);
    }

    method write-xref-array(PDF_UINT @xref, Blob $buf? is copy) {
        my \rows = +@xref div 4;
        # check array is sorted. work out number of segments
        $buf //= Blob[uint8].allocate((rows * 2) * 22 +  5);
        pdf_write_xref(@xref, rows, $buf, $buf.bytes);
    }

    method write-name(Str(Cool) $val, Blob $buf? is copy) {
        my Blob[uint32] $in .= new: $val.ords;
        my \quads = $in.elems;
        $buf //= Blob[uint8].allocate(12 * quads  +  2);
        pdf_write_name($in, quads, $buf, $buf.bytes);
    }
}
