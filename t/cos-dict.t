use PDF::Native::Cos;
use PDF::Native::Defs :types;
use PDF::Grammar::COS;
use PDF::Native::Cos::Actions;
use NativeCall;
use Test;

plan 10;

my PDF::Native::Cos::Actions:D $actions .= new: :lite;

sub parse(Str:D $str, :$rule = 'object') {
    .ast given PDF::Grammar::COS.parse($str, :$rule, :$actions);
}

my $str = '<< /foo 69 /bar 123 0 R /baz (42) /nullish null >>';

my CosDict:D $dict = parse($str);

is $dict.type, +COS_NODE_DICT;
is $dict.elems, 4;
isa-ok $dict[0], CosInt;
isa-ok $dict[1], CosRef;
is $dict[0].value, 69;
is $dict<foo>.value, 69;
is $dict<bar>.obj-num, 123;
is-deeply $dict<not-present>, CosNode;
is-deeply $dict<nullish>, CosNode;
is-deeply $dict.Str, $str;

done-testing;