use PDF::Native::Cos;
use Test;

plan 4;

my CosIndObj $ind-obj .= parse: "10 0 obj 42 endobj";
ok $ind-obj.defined;

is-deeply $ind-obj.Str.lines, ('10 0 obj', '42', 'endobj');

subtest 'indirect object parsing', {
    for  ("10 0 obj\n<</foo[42]>>\nendobj\n", ("10 0 obj", "<</foo[42]>>", "endobj", "").join("\n\%XXX\n") ) {
        subtest "parse: " ~ .raku, {
            $ind-obj .= parse: $_;
            if ok($ind-obj.defined, 'parse') {
                isa-ok $ind-obj.value, CosDict, 'value type';
                is $ind-obj.value<foo>[0].value, 42, 'dereferenced content';
                is $ind-obj.Str.lines, ('10 0 obj', '<< /foo [ 42 ] >>', 'endobj'), '.Str';
            }
            else {
                skip 'parse failed', 3;
            }
        }
    }
}

subtest 'invalid indirect object syntax', {
    for ('', ' ', 'x', '10 x', 'x 10', '10x', '10 0 obj 42', '10 0 obj 42 endobjx',
         '10 0 objx 42 endobj', '10 0 objx 42endobj', '10 0. obj 42 endobj', '-10 0 obj 42 endobj') {
        is-deeply $ind-obj.parse($_), CosIndObj, "parse failure: " ~ .raku;
    }
}

done-testing;
