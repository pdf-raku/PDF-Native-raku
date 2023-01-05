use v6;
use Test;
plan 1;

use PDF::Native::COS;

my cos_indref $ind-ref .= new: :obj-num(1234), :gen-num(5);

subtest 'ind-ref', {
    is cos_indref.Str, '';
    is $ind-ref.Str, "1234 5 R";
}
