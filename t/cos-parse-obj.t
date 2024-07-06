use PDF::Grammar::COS;
use PDF::Native::Cos;
use PDF::Native::Cos::Actions;
use Test;

plan 55;

my PDF::Native::Cos::Actions:D $actions .= new: :lite;

given CosNode.parse('123') {
    .&isa-ok: CosInt;
    is .Str, '123', 'parse int';
}

given CosNode.parse('123.45') {
    .&isa-ok: CosReal;
    is .Str, '123.45', 'parse real';
}

given CosNode.parse('.45') {
    .&isa-ok: CosReal;
    is .Str, '0.45', 'parse real fraction';
}

given CosNode.parse('[]') {
    .&isa-ok: CosArray;
    is .Str, '[ ]', 'parse array';
}

given CosNode.parse("\n \%xxx\n[\%yyy\n]\%zzz\n ") {
    .&isa-ok: CosArray;
    is .Str, '[ ]', 'parse array + ws/comments';
}

given CosNode.parse('(Hello,\40World\n)') {
    .&isa-ok: CosLiteralString;
    is .Str, '(Hello, World\n)', 'parse literal';
}

given CosNode.parse('(\((\{}\)))') {
    .&isa-ok: CosLiteralString;
    is .Str, '(\(\({}\)\))', 'parse parens';
}

given CosNode.parse('(\7\07\007\7)') {
    .&isa-ok: CosLiteralString;
    is-deeply .Str, (flat '(', 7.chr xx 4, ')').join, 'parse octal escapes';
}

for '(\n\r\t\f\b\\' ~ 10.chr ~ 'X)', '(\n\r\t\f\b\\' ~ 13.chr ~ 10.chr ~ 'X)' {
    given CosNode.parse($_) {
        .&isa-ok: CosLiteralString;
        is-deeply .Str, "(\\n\\r\\t\\f\\bX)", 'parse: '~.raku;
    }
}

my $hex = '<4E6F762073686D6F7A206B6120706f702e>';
given CosNode.parse($hex) {
    .&isa-ok: CosHexString;
    is .Str.lc, $hex.lc, 'parse hex';
}

for ('<4E60>' ,'< 4 E 6 0 >', '<4E6>', '<4E6 >') {
    is CosNode.parse($_).Str, '<4e60>', "parse: $_";
}

for ('<>' ,'< >', '<  >') {
    is CosNode.parse($_).Str, '<>', "parse: $_";
}

given CosNode.parse('/Hello,#20World#21') {
    .&isa-ok: CosName;
    is .Str, '/Hello,#20World!', 'parse name';
}

given CosNode.parse('/') {
    .&isa-ok: CosName;
    is .Str, '/', 'parse empty name';
}
given CosNode.parse('true') {
    .&isa-ok: CosBool;
    is .Str, 'true', 'parse bool';
}

given CosNode.parse('false') {
    .&isa-ok: CosBool;
    is .Str, 'false', 'parse bool';
}

given CosNode.parse('null') {
    .&isa-ok: CosNull;
    is .Str, 'null', 'parse null';
}

given CosNode.parse('<< /a 42 >>') {
    .&isa-ok: CosDict;
    is .Str, '<< /a 42 >>', 'parse dict';
}

given CosNode.parse('<</<>>>') {
    .&isa-ok: CosDict;
    is .Str, '<< / <> >>', 'parse dict';
}


given CosNode.parse('12 3 R') {
    .&isa-ok: CosRef;
    is .Str, '12 3 R', 'parse indirect reference';
}

given CosNode.parse('[123 .45/foo<aa>true 12 3 R false null]') {
    .&isa-ok: CosArray;
    is .Str, '[ 123 0.45 /foo <aa> true 12 3 R false null ]', 'parse array';
}

subtest 'invalid numbers', {
    for <++0 0+0 0.. ..0 0a 0..0> {
        is-deeply CosNode.parse($_), CosNode, "invalid number: $_";
    }
}

given CosNode.parse('[1(2) /3  ]') {
    .&isa-ok: CosArray;
    is .Str, '[ 1 (2) /3 ]', 'parse array';
}

given CosNode.parse('<</a 42/BB(Hi)>>') {
    .&isa-ok: CosDict;
    is .Str, '<< /a 42 /BB (Hi) >>', 'parse dict';
}

todo 'native parse';
my $stream = q:to<--END-->;
    << /Length 45 >> stream
    BT
    /F1 24 Tf
    100 250 Td (Hello, world!) Tj
    ET
    endstream
    --END--

todo 'native parse';
given PDF::Grammar::COS.parse( $stream, :rule<object>, :$actions) {
    my CosStream:D $node = .ast;
    is $node.Str, $stream.chomp, 'parse stream';
    is-deeply CosNode.COERCE($node.ast).Str.lines, $stream.lines;
}

my $ind-obj = "123 4 obj\n{$stream}endobj";

todo 'native parse';
with PDF::Grammar::COS.parse( $ind-obj, :rule<ind-obj>, :$actions) {
    my CosIndObj:D $node = .ast;
    is $node.Str.chomp, $ind-obj, 'parse indirect object';
}
