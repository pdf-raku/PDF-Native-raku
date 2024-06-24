use PDF::Native::Cos;
use NativeCall;
use Test;

plan 10;

my CosInt()  $value1  = 69;
my CosRef    $value2 .= new: :obj-num(123);
my CosName() $value3  = 'Hi There';
my CosBool() $value4  = True;
my CosNull   $value5;
my CosReal() $value6  = 12345e-1;
is $value1.ref-count, 1;
my CArray[CosNode] $values .= new: $value1, $value2, $value3, $value4, $value5, $value6;
is $values[0].ref-count, 1;
my CosArray $array .= new: :$values;
is $array.elems, 6;
is $value1.ref-count, 2;
is $array.type, +COS_NODE_ARRAY;
isa-ok $array[0], CosInt;
isa-ok $array[1], CosRef;
isa-ok $array[2], CosName;
is $array[0].value, 69;
is-deeply $array.Str, '[ 69 123 0 R /Hi#20There true null 1234.5 ]';

done-testing;