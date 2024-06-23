use PDF::Native::Cos;
use NativeCall;
use Test;

plan 11;

my CosInt  $value1 .= new: :value(69);
my CosRef  $value2 .= new: :obj-num(123);
my CosName $value3 .= new: :value(CArray[uint32].new: 'Hi There'.ords);
my CosBool $value4 .= new: :value;
my CosNull $value5 .= new;
my CosReal $value6 .= new: :value(12345e-1);
my CosLiteral $value7 .= new: :value('xyz'.encode: 'latin-1');
my CArray[CosNode] $values .= new: $value1, $value2, $value3, $value4, $value5, $value6, $value7;
my CosArray $array .= new: :$values;
is $array[0].value, 69;
my CosIndObj $ind-obj .= new: :obj-num(42), :gen-num(3), :value($array);
is-deeply $ind-obj.Str.lines, (
'42 3 obj',
'[ 69 123 0 R /Hi#20There true null 1234.5 (xyz) ]',
'endobj'
);

my Buf[uint8] $key .= new(193,67,83,175,223);

sub crypt-func(CosCryptCtx $ctx, CArray[uint8] $buf, size_t $buf-len ) {
    is $ctx.obj-num, 42;
    is $ctx.gen-num, 3;
    is-deeply $ctx.key[^$ctx.key-len], $key.list;
    $buf[$_] = 256 - $buf[$_] for ^$buf-len;
}

my CosCryptCtx $crypt-ctx .= new: :$key, :&crypt-func, :mode(COS_CRYPT_ONLY_STRINGS);

$ind-obj.crypt(:$crypt-ctx);
is-deeply $value7.Str, "(\x[88]\x[87]\x[86])";

$ind-obj.crypt(:$crypt-ctx);
is $value7.Str, '(xyz)';

is-deeply $ind-obj.Str.lines, (
'42 3 obj',
'[ 69 123 0 R /Hi#20There true null 1234.5 (xyz) ]',
'endobj'
);
done-testing;