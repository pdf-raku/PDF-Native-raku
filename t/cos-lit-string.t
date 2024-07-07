use PDF::Native::Cos;
use Test;

plan 5;

my CosLiteralString() $literal = 'Hello!';
is $literal.ref-count, 1;

is $literal.type, +COS_NODE_LIT_STR;
is-deeply $literal.value, 'Hello!';
is-deeply $literal.Str, 'Hello!';
is-deeply $literal.write , '(Hello!)';

done-testing;