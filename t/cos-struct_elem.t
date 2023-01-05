use v6;
use Test;
plan 1;

use PDF::Native::COS;
use PDF::Native::COS::StructElem;

my PDF::Native::COS::StructElem $elem .= new: :obj-num(1234), :gen-num(5), :S<P>;

subtest 'struct_elem', {
    is cos_indref.Str, '';
    is  $elem.S, 'P';
    warn $elem.Str.raku;
    is $elem.Str, "<< /Type /StructElem /S /P >>";
}
