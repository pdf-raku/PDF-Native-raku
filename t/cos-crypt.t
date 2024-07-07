use PDF::Native::Cos;
use PDF::Grammar::COS;
use PDF::Native::Cos::Actions;
use NativeCall;
use Test;

plan 12;

my List $lines = (
'42 3 obj',
'[ 69 123 0 R /Hi#20There true null 1234.5 (xyz) ]',
'endobj'
);

my PDF::Native::Cos::Actions:D $actions .= new: :lite;

sub ind-obj-parse(Str:D $str, :$rule) {
    .ast given PDF::Grammar::COS.parse($str, :rule<ind-obj>, :$actions);
}

my CosIndObj $ind-obj = ind-obj-parse($lines.join: "\n");
is $ind-obj.Str.lines.join("\n"), $lines.join("\n");
my CosReal $value6 = $ind-obj.value[5];
my CosLiteralString $value7 = $ind-obj.value[6];

my Buf[uint8] $key .= new(193,67,83,175,223);

sub crypt-func(CosCryptCtx $ctx, CArray[uint8] $buf, size_t $buf-len ) {
    is $ctx.obj-num, 42;
    is $ctx.gen-num, 3;
    is-deeply $ctx.key[^$ctx.key-len], $key.list;
    $buf[$_] = 256 - $buf[$_] for ^$buf-len;
}

my $mode = COS_CRYPT_ONLY_STRINGS;
my CosCryptCtx $crypt-ctx .= new: :$key, :&crypt-func, :$mode;

$ind-obj.crypt(:$crypt-ctx);
is $value6.Str, '1234.5';
is-deeply $value7.Str, "\x[88]\x[87]\x[86]";
isnt $ind-obj.Str.lines.join("\n"), $lines.join("\n");

$ind-obj.crypt(:$crypt-ctx);
is $value7.Str, 'xyz';

is $ind-obj.Str.lines.join("\n"), $lines.join("\n");

done-testing;