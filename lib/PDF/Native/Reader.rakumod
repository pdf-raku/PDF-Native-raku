use v6;

#| Reading of PDF components and datatypes
class PDF::Native::Reader {

=begin pod

The only method so far implemented is `read-xref` for the fast reading of cross reference indices.
=begin code :lang<raku>
use PDF::Native::Reader;

given PDF::Native::Reader.new {

     enum <free inuse>;

     my Str $xref = (
         'xref',
         '10 4',
         '0000000000 65535 f ',
         '0000000042 00000 n ',
         '0000000069 00000 n ',
         '9000000100 00002 n ',
         ''
     ).join(10.chr);
     my Blob $buf = $xref.encode('latin-1');

     my array $entries = .read-xref($buf);
}
=end code
=end pod

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
        $xref = Nil
            if $rows && $!xref-bytes == 0;
        $xref;
    }
}


