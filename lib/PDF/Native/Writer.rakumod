use v6;

#| Serialization functions for PDF components and datatypes
class PDF::Native::Writer {
=begin pod

Serialization functions have been implemented for a few PDF data-types:

- boolean, real, integers, literal-strings, hex-strings, names and cross reference tables.

=begin code :lang<raku>
use PDF::Native::Writer;

given PDF::Native::Writer {
     say .write-bool(0);    # false
     say .write-bool(1);    # true
     say .write-real(pi);   # 3.14159
     say .write-int(42e3),  # 42000
     say .write-literal("Hi\nthere"); # (Hi\nthere)
     say .write-hex-string("snoopy"); # <736e6f6f7079>
     say .write-name('Hi#there');     # /Hi##there

     # xref entries
     enum <free inuse>;
     my uint64 @xref[4;3] = (
        [0, 65535, free],
        [42, 0, inuse],
        [69, 0, inuse],
        [100, 2, inuse],
     );
     say .write-entries(@xref).lines;
         # 0000000000 65535 f 
         # 0000000042 00000 n 
         # 0000000069 00000 n 
         # 0000000100 00002 n
}
=end code

=head2 Methods
=end pod

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

    sub pdf_write_name(PDF_NAME $val, size_t $val-len, Blob[uint8] $buf, size_t $buf-len)
        returns size_t
        is native(libpdf) {*};

    method !decode(Blob[uint8] $buf, $len --> Str) {
        $buf.subbuf(0, $len).decode: "latin-1";
    }

    #| write 'true' or 'false'
    method write-bool(Bool:D $val, $buf = Blob[uint8].allocate(10) --> Str) {
        self!decode: $buf, pdf_write_bool($val, $buf, $buf.bytes);
    }

    #| write simple integer, e.g. '42'
    method write-int(Int:D $val, $buf = Blob[uint8].allocate(8) --> Str) {
        self!decode: $buf, pdf_write_int($val, $buf, $buf.bytes);
    }

    #| write number, e.g. '4.2'
    method write-real(Num() $val, $buf = Blob[uint8].allocate(32) --> Str) {
        self!decode: $buf, pdf_write_real($val, $buf, $buf.bytes);
    }

    #| write string literal, e.g. '(Hello, World!)'
    method write-literal(Str:D $val, Blob $buf? is copy --> Str) {
        my Blob[uint8] $enc = $val.encode: "latin-1";
        my \bytes-in = $enc.bytes;
        $buf //= Blob[uint8].allocate(2 * bytes-in  +  3);
        self!decode: $buf, pdf_write_literal($enc, bytes-in, $buf, $buf.bytes);
    }

    #| write binary hex string, e.g. '<deadbeef>'
    method write-hex-string(Str:D $val, Blob $buf? is copy --> Str) {
        my Blob[uint8] $enc = $val.encode: "latin-1";
        my \bytes-in = $enc.bytes;
        $buf //= Blob[uint8].allocate(2 * bytes-in  +  3);
        self!decode: $buf, pdf_write_hex_string($enc, bytes-in, $buf, $buf.bytes);
    }

    #| write cross reference entries
    multi method write-entries(array $xref, Blob $buf? is copy --> Str) {
        my \rows = +$xref.list div 3;
        # check array is sorted. work buf number of segments
        $buf //= Blob[uint8].allocate(rows * 22 + 1);
        self!decode: $buf, pdf_write_xref_seg(buf64.new($xref), rows, $buf, $buf.bytes);
    }
    multi method write-entries(List $_, |c --> Str) {
        my uint64 @shaped[.elems;3] = .List;
        self.write-entries(@shaped, |c);
    }

    #| write name, e.g. '/Raku'
    method write-name(Str:D $val, Blob $buf? is copy --> Str) {
        my PDF_NAME $in .= new: $val.ords;
        my \quads = $in.elems;
        $buf //= Blob[uint8].allocate(12 * quads  +  2);
        self!decode: $buf, pdf_write_name($in, quads, $buf, $buf.bytes);
    }

    # just in case this get routed to us
    method write-null($?) { 'null' }
}

