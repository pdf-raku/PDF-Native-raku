use PDF::Native::Cos;
use Test;
use NativeCall;

plan 4;

my blob8 $value = 'Hello!'.encode: 'latin-1';
my CosLiteral $literal .= new: :$value;
is $literal.ref-count, 1;

is $literal.type, +COS_NODE_LITERAL;
is-deeply $literal.value, 'Hello!';
is-deeply $literal.Str , '(Hello!)';

done-testing;