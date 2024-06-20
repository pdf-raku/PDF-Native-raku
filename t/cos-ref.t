use PDF::Native::Cos;
use Test;

plan 5;

my CosRef $ref .= new: :obj-num(42);

is $ref.type, +COS_NODE_REF;
is $ref.ref-count, 1;
is $ref.obj-num, 42;
is $ref.gen-num, 0;
is $ref.Str, '42 0 R';

done-testing;