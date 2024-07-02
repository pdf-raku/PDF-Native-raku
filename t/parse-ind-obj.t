use PDF::Native::Parser;
use PDF::Native::Cos;
use Test;

plan 2;

my CosIndObj $ind-obj .= parse: "10 0 obj 42 endobj";
ok $ind-obj.defined;

is-deeply $ind-obj.Str.lines, ('10 0 obj', '42', 'endobj');

done-testing;