use PDF::Native::Cos;
use Test;

plan 4;

my CosName() $name = 'Hello!';
is $name.ref-count, 1;

is $name.type, +COS_NODE_NAME;
is-deeply $name.value[^$name.value-len], 'Hello!'.ords;
is-deeply $name.Str , ('/Hello!');

done-testing;