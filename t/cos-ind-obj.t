use PDF::Native::COS;
use Test;

plan 15;

my COSInt() $value = 69;

for (COSIndObj.new(:obj-num(42), :gen-num(3), :$value), COSIndObj.COERCE($[42, 3, :int(69)])) -> $ind-obj {
    is $ind-obj.type, +COS_NODE_IND_OBJ;
    is $ind-obj.obj-num, 42;
    is $ind-obj.gen-num, 3;
    isa-ok $ind-obj.value, COSInt;
    is $ind-obj.value.value, 69;
    is-deeply $ind-obj.Str.lines , ('42 3 obj', '69', 'endobj');
    is-deeply COSNode.COERCE($ind-obj.ast).Str.lines , ('42 3 obj', '69', 'endobj');
}

my $stream = q:to<END>;
45 0 obj
<< /Length 4 >> stream
abcd
endstream
endobj
END

my COSIndObj $ind-obj .= parse: $stream, :scan;

is-deeply COSNode.COERCE($ind-obj.ast).Str.lines, $stream.lines;

done-testing;
