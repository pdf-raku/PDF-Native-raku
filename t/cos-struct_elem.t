use v6;
use Test;
plan 1;

use PDF::Native::COS;
use PDF::Native::COS::StructElem;

my cos_indref $Pg .= new: :obj-num(42), :gen-num(3);

my PDF::Native::COS::StructElem $elem .= new: :obj-num(1234), :gen-num(5), :S<P>, :ID<ABC123>, :$Pg;

subtest 'struct_elem', {
    is cos_indref.Str, '';
    is  $elem.S, 'P';
    is $elem.Str, "<< /Type /StructElem /S /P /ID <414243313233> /Pg 42 3 R >>";
}
