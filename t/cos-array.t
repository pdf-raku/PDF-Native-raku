use PDF::Native::COS;
use NativeCall;
use Test;

plan 10;

my CosInt $value1 .= new: :value(69);
my CosRef $value2 .= new: :obj-num(123);
is $value1.ref-count, 1;
my CArray[CosNode] $values .= new: $value1, $value2;
is $values[0].ref-count, 1;
my CosArray $array .= new: :$values;
is $array.elems, 2;
is $value1.ref-count, 2;

is $array.type, +COS_NODE_ARRAY;
is $array.elems, 2;
isa-ok $array[0], CosInt;
isa-ok $array[1], CosRef;
is $array[0].value, 69;
is-deeply $array.Str, '[ 69 123 0 R ]';

done-testing;