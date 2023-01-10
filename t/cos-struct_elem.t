use v6;
use Test;
plan 3;

use PDF::Native::COS;
use PDF::Native::COS::StructElem;

my cos_indref $Pg .= new: :obj-num(42), :gen-num(3);

subtest 'struct_elem', {
    my cos_struct_elem $elem .= new: :obj-num(1234), :gen-num(5), :S<P>, :ID<ABC123>, :$Pg;

    is cos_indref.Str, '';
    is  $elem.S, 'P';
    is  $elem.ID, 'ABC123';
    is $elem.Str, "<< /Type /StructElem /S /P /ID <414243313233> /Pg 42 3 R >>";
}

subtest 'mcr', {
    my cos_mcr $mcr .= new: :obj-num(1234), :gen-num(5), :MCID(42), :$Pg;

    is $mcr.Str, "<< /Type /MCR /Pg 42 3 R /MCID 42 >>";
}

subtest 'objr', {
    my cos_indref $Obj .= new: :obj-num(69), :gen-num(0);
    my cos_objr $objr .= new: :obj-num(1234), :gen-num(5), :$Pg, :$Obj;
    is-deeply $objr.Obj, $Obj;

    is $objr.Str, "<< /Type /OBJR /Pg 42 3 R /Obj 69 0 R >>";
}
