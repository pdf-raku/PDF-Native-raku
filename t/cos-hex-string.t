use PDF::Native::COS;
use Test;

plan 5;

my COSHexString() $hex-string = 'Hello!';
is $hex-string.ref-count, 1;

is $hex-string.type, +COS_NODE_HEX_STR;
is-deeply $hex-string.value, 'Hello!';
is-deeply $hex-string.Str, 'Hello!';
is-deeply $hex-string.write , '<48656c6c6f21>';

done-testing;