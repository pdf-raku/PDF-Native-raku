use PDF::Native::COS;
use Test;

plan 6;

my CosInt $value .= new: :value(69);
my CosIndObj $ind-obj .= new: :obj-num(42), :gen-num(3), :$value;

is $ind-obj.type, +COS_NODE_IND_OBJ;
is $ind-obj.obj-num, 42;
is $ind-obj.gen-num, 3;
isa-ok $ind-obj.value, CosInt;
is $ind-obj.value.value, 69;
is-deeply $ind-obj.Str.lines , ('42 3 obj', '69', 'endobj');

done-testing;