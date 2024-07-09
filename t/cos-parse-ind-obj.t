use PDF::Native::Cos;
use Test;

plan 10;

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

subtest 'scan', {
    my $stream-obj = q:to<--END-->;
    42 10 obj
    << /Length 45 >> stream
    BT
    /F1 24 Tf
    100 250 Td (Hello, world!) Tj
    ET
    endstream
    endobj
    --END--
    is-deeply $ind-obj.parse($stream-obj, :scan).Str.lines, $stream-obj.lines;
}

subtest 'invalid indirect object syntax', {
    for ('', ' ', 'x', '10 x', 'x 10', '10x', '10 0 obj 42', '10 0 obj 42 endobjx',
         '10 0 objx 42 endobj', '10 0 objx 42endobj', '10 0. obj 42 endobj', '-10 0 obj 42 endobj') {
        is-deeply $ind-obj.parse($_), CosIndObj, "parse failure: " ~ .raku;
    }
}

my Str:D $str = "t/pdf/samples/ind-obj-dict.bin".IO.slurp(:bin).decode: "latin-1";
$ind-obj .= parse: $str;
ok $ind-obj.defined, 'binary dict parse';

$*ERR.say: "**********************";

$str = "t/pdf/samples/ind-obj-stream.bin".IO.slurp(:bin).decode: "latin-1";
$ind-obj .= parse: $str, :scan;
ok $ind-obj.defined, 'binary stream parse';
isa-ok $ind-obj.value, CosStream;
ok $ind-obj.value.value.defined;
is $ind-obj.value.dict<Length>, $ind-obj.value.u.value-len;

done-testing;
