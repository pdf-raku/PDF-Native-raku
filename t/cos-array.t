use PDF::Native::COS;
use NativeCall;
use Test;

plan 10;

my COSInt()  $value1  = 69;
my COSRef    $value2 .= new: :obj-num(123);
my COSName() $value3  = 'Hi There';
my COSBool() $value4  = True;
my COSNull   $value5;
my COSComment() $value6 = "Hello\nagain";
my COSReal() $value7  = 12345e-1;
is $value1.ref-count, 1;
my CArray[COSNode] $values .= new: $value1, $value2, $value3, $value4, $value5, $value6, $value7;
is $values[0].ref-count, 1;
my COSArray $array .= new: :$values;
is $array.elems, 7;
is $value1.ref-count, 2;
is $array.type, +COS_NODE_ARRAY;
isa-ok $array[0], COSInt;
isa-ok $array[1], COSRef;
isa-ok $array[2], COSName;
is $array[0].value, 69;
is-deeply $array.Str.lines, ('[ 69 123 0 R /Hi#20There true null % Hello', '% again', ' 1234.5 ]');

done-testing;
