use PDF::Native::Cos;
use NativeCall;
use Test;

plan 10;

my CosInt()  $value1  = 69;
my CosName() $value2  = 'Hi There';
my CosBool() $value3  = True;
my CosNull   $value4;
my CosReal() $value5  = 12345e-1;
is $value1.ref-count, 1;
my CArray[CosNode] $values .= new: $value1, $value2, $value3, $value4, $value5;
is $values[0].ref-count, 1;
my CosOp $op .= new: :opn<Foo>, :$values;
is $op.elems, 5;
is $value1.ref-count, 2;
is $op.type, +COS_NODE_OP;
isa-ok $op[0], CosInt;
isa-ok $op[1], CosName;
is $op[0].value, 69;
is-deeply $op.write(:indent(4)), '    69 /Hi#20There true null 1234.5 Foo';

my CosOp $op1 .= new: :opn<BT>;
my CosOp $op2 .= new: :opn<ET>;

my CArray[CosOp] $content-values .= new: $op1, $op, $op2;
my CosContent $content .= new: :values($content-values);

is-deeply $content.Str.lines, ('BT', '69 /Hi#20There true null 1234.5 Foo', 'ET');

done-testing;