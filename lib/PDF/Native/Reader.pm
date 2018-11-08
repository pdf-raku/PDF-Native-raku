use v6;

class PDF::Native::Reader {
    has UInt $.xref-bytes;
    use NativeCall;
    use PDF::Native :libpdf, :types;

    sub pdf_read_xref_entry_count(Blob[uint8] $buf, size_t $buflen)
        returns size_t
        is native(libpdf) {*};

    sub pdf_read_xref_seg(array $val, size_t $rows, Blob[uint8] $buf, size_t $buflen, size_t $obj-first-num)
        returns size_t
        is native(libpdf) {*};

    sub pdf_read_xref(array $val, Blob[uint8] $buf, size_t $buflen)
        returns size_t
        is native(libpdf) {*};

    multi method read-entries(array $xref is copy, Blob $buf, UInt $rows = +$buf div 20, UInt :$obj-first-num = 0) {
        $xref //= array[uint64].new;
        $xref[($rows||1) * 4  -  1] ||= 0;
        pdf_read_xref_seg($xref, $rows, $buf, $buf.bytes, $obj-first-num);
        $xref;
    }
    multi method read-entries(Blob $buf, UInt $rows = +$buf div 20, :$obj-first-num = 0) {
        $.read-entries(array, $buf, $rows, :$obj-first-num);
    }
    method count-entries(Blob $buf) {
        pdf_read_xref_entry_count($buf, $buf.bytes);
    }
    multi method read-xref(Blob $buf) {
        my $rows = $.count-entries($buf);
        my $xref //= array[uint64].new;
        $xref[($rows||1) * 4  -  1] ||= 0;
        $!xref-bytes = pdf_read_xref($xref, $buf, $buf.bytes);
        $xref;
    }
}
