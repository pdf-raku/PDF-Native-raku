use PDF::Native::Cos;
use Test;

plan 4;

my CosHexString() $hex-string = 'Hello!';
is $hex-string.ref-count, 1;

is $hex-string.type, +COS_NODE_HEX_STR;
is-deeply $hex-string.value, 'Hello!';
is-deeply $hex-string.Str , '<48656c6c6f21>';

done-testing;