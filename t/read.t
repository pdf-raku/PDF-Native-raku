use v6;
use Test;
plan 5;

use Lib::PDF::Reader;

given Lib::PDF::Reader {

    enum <free inuse>;
    constant Lf = 10.chr;

     my uint64 @xref = (
        0, 0, 65535, free,
        1, 42, 0, inuse,
        2, 69, 0, inuse,
        3, 9000000100, 2, inuse,
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
         10, 0, 65535, free,
         11, 42, 0, inuse,
         0, 0, 0, 0,
         0, 0, 0, 0,
     );
     is-deeply .read-entries($buf, :obj-first-num(10)), @xref, '.read-entries';

     my $xref = (('20 4' ~ Lf ~ $xref-seg) x 2).subst('20', '10').encode('latin-1');
     is .count-entries($xref), 8, '.count-entries';
     @xref = (
         10, 0, 65535, free,
         11, 42, 0, inuse,
         12, 69, 0, inuse,
         13, 9000000100, 2, inuse,
         20, 0, 65535, free,
         21, 42, 0, inuse,
         22, 69, 0, inuse,
         23, 9000000100, 2, inuse,
     );
     is-deeply .read-xref($xref), @xref, '.read-xref';
}
