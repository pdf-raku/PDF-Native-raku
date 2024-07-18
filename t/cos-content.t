use PDF::Native::COS;
use NativeCall;
use Test;

plan 12;

my COSInt()  $value1  = 69;
my COSName() $value2  = 'Hi There';
my COSBool() $value3  = True;
my COSNull   $value4;
my COSReal() $value5  = 12345e-1;
is $value1.ref-count, 1;
my CArray[COSNode] $values .= new: $value1, $value2, $value3, $value4, $value5;
is $values[0].ref-count, 1;
my COSOp $op .= new: :opn<Foo>, :$values;
is $op.elems, 5;
is $value1.ref-count, 2;
is $op.type, +COS_NODE_OP;
isa-ok $op[0], COSInt;
isa-ok $op[1], COSName;
is $op[0].value, 69;
is-deeply $op.write(:indent(4)), '    69 /Hi#20There true null 1234.5 Foo';

my COSOp $op1 .= new: :opn<BT>;
my COSOp $op2 .= new: :opn<ET>;

my CArray[COSOp] $content-values .= new: $op1, $op, $op2;
my COSContent $content .= new: :values($content-values);

is-deeply $content.Str.lines, ('BT', '69 /Hi#20There true null 1234.5 Foo', 'ET');

$content .= parse: "BT /F1 24 Tf  100 250 Td (Hello, world!) Tj ET";
ok $content.defined, "content parse";

todo "indentation";
is-deeply $content.write.lines, ('BT', ' /F1 24 Tf', '  100 250 Td', '  Td (Hello, world!)', 'ET');

done-testing;