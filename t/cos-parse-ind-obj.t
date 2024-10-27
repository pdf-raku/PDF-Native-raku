use PDF::Native::COS;
use Test;

plan 8;

my COSIndObj $ind-obj .= parse: "10 0 obj 42 endobj";
ok $ind-obj.defined;

is-deeply $ind-obj.Str.lines, ('10 0 obj', '42', 'endobj');

subtest 'indirect object parsing', {
    for  ("10 0 obj\n<</foo[42]>>\nendobj\n", ("10 0 obj", "<</foo[42]>>", "endobj", "").join("\n\%XXX\n") ) {
        subtest "parse: " ~ .raku, {
            $ind-obj .= parse: $_;
            if ok($ind-obj.defined, 'parse') {
                isa-ok $ind-obj.value, COSDict, 'value type';
                is $ind-obj.value<foo>[0].value, 42, 'dereferenced content';
                is $ind-obj.Str.lines, ('10 0 obj', '<< /foo [ 42 ] >>', 'endobj'), '.Str';
            }
            else {
                skip 'parse failed', 3;
            }
        }
    }
}

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

subtest 'scan', {
    is-deeply $ind-obj.parse($stream-obj, :scan).Str.lines, $stream-obj.lines;
}

subtest 'attach', {
    my Blob $in-buf = $stream-obj.encode: "latin-1";
    $ind-obj .= parse: $in-buf;
    my COSStream:D $stream = $ind-obj.value;
    nok $stream.value.defined;
    is $stream.value-pos, 34;
    nok $stream.value-len.defined;
    $stream.attach-data($in-buf, $stream.dict<Length>.Int);
    nok $stream.value-pos.defined;
    is $stream.value-len, $stream.dict<Length>.Int;
    my buf8 $buf .= allocate(2);
    is-deeply $ind-obj.write(:$buf).lines, $stream-obj.lines;
    ok $buf.bytes >= $in-buf.bytes, 'write buffer resized';
}

subtest 'invalid indirect object syntax', {
    for ('', ' ', 'x', '10 x', 'x 10', '10x', '10 0 obj 42', '10 0 obj 42 endobjx', '10 0obj 42 endobj',
         '10 0 objx 42 endobj', '10 0 objx 42endobj', '10 0. obj 42 endobj', '-10 0 obj 42 endobj') {
        is-deeply $ind-obj.parse($_), COSIndObj, "parse failure: " ~ .raku;
    }
}

my Str:D $str = "t/pdf/samples/ind-obj-dict.bin".IO.slurp(:bin).decode: "latin-1";
$ind-obj .= parse: $str;
ok $ind-obj.defined, 'binary dict parse';

for "t/pdf/samples/ind-obj-stream.bin" {
    subtest $_, {
        $str = .IO.slurp(:bin).decode: "latin-1";
        $ind-obj .= parse: $str, :scan;
        ok $ind-obj.defined, 'binary stream parse';
        isa-ok $ind-obj.value, COSStream;
        ok $ind-obj.value.value.defined;
        is $ind-obj.value.value-len, $ind-obj.value.dict<Length>, '/Length';
    }
}

done-testing;
