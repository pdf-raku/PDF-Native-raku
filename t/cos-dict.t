use PDF::Native::COS;
use PDF::Native::Defs :types;
use NativeCall;
use Test;

plan 13;

my CosInt $value1 .= new: :value(69);
my CosRef $value2 .= new: :obj-num(123);
is $value1.ref-count, 1;
my PDF_TYPE_CODE_POINTS $key1 .= new: "foo".ords;
my PDF_TYPE_CODE_POINTS $key2 .= new: "bar".ords;
my CArray[Pointer] $keys .= new: nativecast(Pointer, $key1), nativecast(Pointer, $key2);
my CArray[CosNode] $values .= new: $value1, $value2;
my CArray[uint16] $key-lens .= new: 3,3;
is $values[0].ref-count, 1;
my CosDict $dict .= new: :$keys, :$key-lens, :$values;
is $dict.elems, 2;
is $value1.ref-count, 2;

is $dict.type, +COS_NODE_DICT;
is $dict.elems, 2;
isa-ok $dict[0], CosInt;
isa-ok $dict[1], CosRef;
is $dict[0].value, 69;
is $dict<foo>.value, 69;
is $dict<bar>.obj-num, 123;
is-deeply $dict<baz>, CosNode;
is-deeply $dict.Str, '<< /foo 69 /bar 123 0 R >>';

done-testing;