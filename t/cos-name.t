use PDF::Native::COS;
use Test;
use NativeCall;

plan 4;

my CArray[uint32] $value .= new: 'Hello!'.ords;
my CosName $name .= new: :$value;
is $name.ref-count, 1;

is $name.type, +COS_NODE_NAME;
is-deeply $name.value, $value;
is-deeply $name.Str , ('/Hello!');

done-testing;