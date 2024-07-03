use PDF::Native::Cos;
use Test;

plan 3;

my CosIndObj $ind-obj .= parse: "10 0 obj 42 endobj";
ok $ind-obj.defined;

is-deeply $ind-obj.Str.lines, ('10 0 obj', '42', 'endobj');

subtest 'invalid indirect object syntax', {
    for ('', ' ', 'x', '10 x', 'x 10', '10x', '10 0 obj 42 endobjx',
         '10 0 objx 42 endobj', '10 0 objx 42endobj', '10 0. obj 42 endobj') {
        is-deeply $ind-obj.parse($_), CosIndObj, "parse: " ~ .raku;
    }
}

done-testing;
