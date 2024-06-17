use PDF::Native::COS;
use Test;
use NativeCall;

plan 4;

my blob8 $value = 'Hello!'.encode: 'latin-1';
my CosHexString $hex-string .= new: :$value;
is $hex-string.ref-count, 1;

is $hex-string.type, +COS_NODE_HEX;
is-deeply $hex-string.value, 'Hello!';
is-deeply $hex-string.Str , '<48656c6c6f21>';

done-testing;