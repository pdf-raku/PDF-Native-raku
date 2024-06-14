use PDF::Native::COS;
use Test;

plan 3;

my CosRef $ref .= new: :obj-num(42);

is $ref.obj-num, 42;
is $ref.gen-num, 0;
is $ref.Str, '42 0 R';

done-testing;