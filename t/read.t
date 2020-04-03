use v6;
use Test;
plan 5;

use PDF::Native::Reader;

given PDF::Native::Reader.new {

    enum <free inuse>;
    constant Lf = 10.chr;

     my uint64 @xref = (
        0, free, 0, 65535,
        1, inuse, 42, 0,
        2, inuse, 69, 0,
        3, inuse, 9000000100, 2,
     );
     my Str $xref-seg = (
         '0000000000 65535 f ',
         '0000000042 00000 n ',
         '0000000069 00000 n ',
         '9000000100 00002 n ',
         ''
     ).join(Lf);
     my Blob $buf = $xref-seg.encode('latin-1');

     is-deeply .read-entries($buf,4), @xref;
     is-deeply .read-entries($buf), @xref;
     $buf = $xref-seg.subst("69", "6x").encode('latin-1');
     @xref = (
         10, free, 0, 65535,
         11,  inuse, 42, 0,
         0, 0, 0, 0,
         0, 0, 0, 0,
     );
     is-deeply .read-entries($buf, :obj-first-num(10)), @xref, '.read-entries';
     my $xref-str = ('xref', Lf,
                     '10 4', Lf, $xref-seg,
                     '20 4', Lf, $xref-seg).join;
     my $xref = $xref-str.encode('latin-1');
     is .count-entries($xref), 8, '.count-entries';
     @xref = (
         10, free, 0, 65535,
         11, inuse, 42, 0,
         12, inuse, 69, 0,
         13, inuse, 9000000100, 2,
         20, free, 0, 65535,
         21, inuse, 42, 0,
         22, inuse, 69, 0,
         23, inuse, 9000000100, 2,
     );
     is-deeply .read-xref($xref), @xref, '.read-xref';
}
