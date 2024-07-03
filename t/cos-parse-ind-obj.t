use PDF::Native::Cos;
use Test;

plan 4;

my CosIndObj $ind-obj .= parse: "10 0 obj 42 endobj";
ok $ind-obj.defined;

is-deeply $ind-obj.Str.lines, ('10 0 obj', '42', 'endobj');

subtest 'valid indirect object syntax', {
    for  ("10 0 obj\n42\nendobj\n", ("10 0 obj", "42", "endobj", "").join("\n\%XXX\n") ) {
        subtest "parse: " ~ .raku, {
            $ind-obj .= parse: $_;
            if ok($ind-obj.defined, 'parse') {
                isa-ok $ind-obj.value, CosInt, 'value type';
                is $ind-obj.value.value, 42, 'value content';
                is $ind-obj.Str.lines, ('10 0 obj', '42', 'endobj'), '.Str';
            }
            else {
                skip 'parse failed', 3;
            }
        }
    }
}

subtest 'invalid indirect object syntax', {
    for ('', ' ', 'x', '10 x', 'x 10', '10x', '10 0 obj 42 endobjx',
         '10 0 objx 42 endobj', '10 0 objx 42endobj', '10 0. obj 42 endobj') {
        is-deeply $ind-obj.parse($_), CosIndObj, "parse: " ~ .raku;
    }
}

done-testing;
