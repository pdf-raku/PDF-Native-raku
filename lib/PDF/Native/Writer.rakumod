use v6;

class PDF::Native::Writer {
    use NativeCall;
    use PDF::Native :libpdf, :types;

    sub pdf_write_bool(PDF_BOOL $val, Blob[uint8] $buf, size_t $buf-len)
        returns size_t
        is native(libpdf) {*};

    sub pdf_write_int(PDF_INT $val, Blob[uint8] $buf, size_t $buf-len)
        returns size_t
        is native(libpdf) {*};

    sub pdf_write_real(PDF_REAL $val, Blob[uint8] $buf, size_t $buf-len)
        returns size_t
        is native(libpdf) {*};

    sub pdf_write_literal(Blob[uint8] $val, size_t $val-len, Blob[uint8] $buf, size_t $buf-len)
        returns size_t
        is native(libpdf) {*};

    sub pdf_write_hex_string(Blob[uint8] $val, size_t $val-len, Blob[uint8] $buf, size_t $buf-len)
        returns size_t
        is native(libpdf) {*};

    sub pdf_write_xref_seg(PDF_XREF $val, size_t $rows, Blob[uint8] $buf, size_t $buf-len)
        returns size_t
        is native(libpdf) {*};

    sub pdf_write_name(PDF_CODE_POINTS $val, size_t $val-len, Blob[uint8] $buf, size_t $buf-len)
        returns size_t
        is native(libpdf) {*};

    method !decode(Blob[uint8] $buf, $len --> Str) {
        $buf.subbuf(0, $len).decode: "latin-1";
    }

    method write-bool(Bool() $val, $buf = Blob[uint8].allocate(10)) {
        self!decode: $buf, pdf_write_bool($val, $buf, $buf.bytes);
    }

    method write-int(Int() $val, $buf = Blob[uint8].allocate(8)) {
        self!decode: $buf, pdf_write_int($val, $buf, $buf.bytes);
    }

    method write-real(Num() $val, $buf = Blob[uint8].allocate(32)) {
        self!decode: $buf, pdf_write_real($val, $buf, $buf.bytes);
    }

    method write-literal(Str() $val, Blob $buf? is copy) {

        my Blob[uint8] $enc = $val.encode: "latin-1";
        my \bytes-in = $enc.bytes;
        $buf //= Blob[uint8].allocate(2 * bytes-in  +  3);
        self!decode: $buf, pdf_write_literal($enc, bytes-in, $buf, $buf.bytes);
    }

    method write-hex-string(Str() $val, Blob $buf? is copy) {
        my Blob[uint8] $enc = $val.encode: "latin-1";
        my \bytes-in = $enc.bytes;
        $buf //= Blob[uint8].allocate(2 * bytes-in  +  3);
        self!decode: $buf, pdf_write_hex_string($enc, bytes-in, $buf, $buf.bytes);
    }

    multi method write-entries(array $xref, Blob $buf? is copy) {
        my \rows = +$xref.list div 3;
        # check array is sorted. work buf number of segments
        $buf //= Blob[uint8].allocate(rows * 22 + 1);
        self!decode: $buf, pdf_write_xref_seg(buf64.new($xref), rows, $buf, $buf.bytes);
    }
    multi method write-entries(List $_, |c) is default {
        my uint64 @shaped[.elems;3] = .List;
        self.write-entries(@shaped, |c);
    }

    method write-name(Str() $val, Blob $buf? is copy) {
        my PDF_CODE_POINTS $in .= new: $val.ords;
        my \quads = $in.elems;
        $buf //= Blob[uint8].allocate(12 * quads  +  2);
        self!decode: $buf, pdf_write_name($in, quads, $buf, $buf.bytes);
    }

    # just in case this get routed to us
    method write-null($?) { 'null' }
}
