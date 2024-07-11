use PDF::Native::COS;
use Test;

plan 15;

sub parse(Str:D $str) {
    COSNode.parse: $str;
}

my $str = '<< /foo 69 /bar 123 0 R /baz (42) /nullish null >>';

my COSDict:D $dict = parse($str);

is $dict.type, +COS_NODE_DICT;
is $dict.elems, 4;
isa-ok $dict[0], COSInt;
isa-ok $dict[1], COSRef;
is $dict[0].value, 69;
is $dict<foo>.value, 69;
is $dict<bar>.obj-num, 123;
my COSName() $key = 'bar'; 
is $dict{$key}.obj-num, 123;
is-deeply $dict<not-present>, COSNode;
is-deeply $dict<nullish>, COSNode;
is-deeply $dict.Str, $str;

$str = q:to<END>;
<<
  /Type /Page
  /Contents 6 0 R
  /MediaBox [ 0 0 420 595 ]
  /Parent 4 0 R
  /Resources << /Font << /F1 7 0 R >> /Procset [ /PDF /Text ] >>
>>
END

$dict = parse($str);

is-deeply $dict.Str.lines, $str.lines;
is-deeply $dict.write(:compact), $str.trim.subst(/[\s|\n]+/, ' ', :g);

my buf8 $buf .= allocate(20);
is-deeply $dict.Str(:$buf).lines, $str.lines;

is-deeply COSNode.COERCE($dict.ast).Str.lines, $str.lines;

done-testing;