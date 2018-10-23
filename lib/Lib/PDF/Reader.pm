use v6;

class Lib::PDF::Reader {
    use NativeCall;
    use Lib::PDF :libpdf, :types;

    sub pdf_read_xref_seg(array $val, size_t $rows, Blob[uint8] $out, size_t $outlen)
        returns size_t
        is native(&libpdf) {*};

    multi method read-entries(array $xref is copy, Blob $buf, UInt $rows = +$buf div 20) {
        # check array is sorted. work out number of segments
        $xref //= array[uint64].new;
        $xref[($rows||1) * 3  -  1] ||= 0;
        pdf_read_xref_seg($xref, $rows, $buf, $buf.bytes);
        $xref;
    }
    multi method read-entries(Blob $buf, UInt $rows = +$buf div 20) {
        $.read-entries(array, $buf, $rows);
    }
}
