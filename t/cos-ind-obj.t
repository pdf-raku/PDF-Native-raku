use PDF::Native::Cos;
use Test;

plan 14;

my CosInt() $value = 69;

for (CosIndObj.new(:obj-num(42), :gen-num(3), :$value), CosIndObj.COERCE($[42, 3, :int(69)])) -> $ind-obj {
    is $ind-obj.type, +COS_NODE_IND_OBJ;
    is $ind-obj.obj-num, 42;
    is $ind-obj.gen-num, 3;
    isa-ok $ind-obj.value, CosInt;
    is $ind-obj.value.value, 69;
    is-deeply $ind-obj.Str.lines , ('42 3 obj', '69', 'endobj');
    is-deeply CosNode.COERCE($ind-obj.ast).Str.lines , ('42 3 obj', '69', 'endobj');
}

done-testing;
