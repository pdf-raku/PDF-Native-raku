use PDF::Native::COS;
use Test;

plan 6;

my COSLiteralString() $literal = 'Hello!';
is $literal.ref-count, 1;

is $literal.type, +COS_NODE_LIT_STR;
is-deeply $literal.value, 'Hello!';
is-deeply $literal.Str, 'Hello!';
is-deeply $literal.ast, (:literal<Hello!>);
is-deeply $literal.write , '(Hello!)';

done-testing;