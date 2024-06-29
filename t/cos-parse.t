use PDF::Grammar::COS;
use PDF::Native::Cos;
use PDF::Native::Cos::Actions;
use Test;

plan 15;

my PDF::Native::Cos::Actions:D $actions .= new: :lite;

given PDF::Grammar::COS.parse('123', :rule<object>, :$actions) {
      my CosInt:D $node = .ast;
      is $node.Str, '123', 'parse int';
}

given PDF::Grammar::COS.parse('123.45', :rule<object>, :$actions) {
      my CosReal:D $node = .ast;
      is $node.Str, '123.45', 'parse real';
}

given PDF::Grammar::COS.parse('.45', :rule<object>, :$actions) {
      my CosReal:D $node = .ast;
      is $node.Str, '0.45', 'parse real fraction';
}

given PDF::Grammar::COS.parse('(Hello,\40World\n)', :rule<object>, :$actions) {
      my CosLiteralString:D $node = .ast;
      is $node.Str, '(Hello, World\n)', 'parse literal';
}

my $hex = '<4E6F762073686D6F7A206B6120706f702e>';
given PDF::Grammar::COS.parse($hex, :rule<object>, :$actions) {
      my CosHexString:D $node = .ast;
      is $node.value, 'Nov shmoz ka pop.';
      is $node.Str, $hex.lc, 'parse hex';
}

given PDF::Grammar::COS.parse('/Hello,#20World#21', :rule<object>, :$actions) {
      my CosName:D $node = .ast;
      is $node.Str, '/Hello,#20World!', 'parse name';
}

given PDF::Grammar::COS.parse('true', :rule<object>, :$actions) {
      my CosBool:D $node = .ast;
      is $node.Str, 'true', 'parse bool';
}

given PDF::Grammar::COS.parse('false', :rule<object>, :$actions) {
      my CosBool:D $node = .ast;
      is $node.Str, 'false', 'parse bool';
}

given PDF::Grammar::COS.parse('null', :rule<object>, :$actions) {
      my CosNull:D $node = .ast;
      is $node.Str, 'null', 'parse null';
}

given PDF::Grammar::COS.parse('12 3 R', :rule<object>, :$actions) {
      my CosRef:D $node = .ast;
      is $node.Str, '12 3 R', 'parse indirect reference';
}

given PDF::Grammar::COS.parse('[1(2) /3  ]', :rule<object>, :$actions) {
    my CosArray:D $node = .ast;
    is $node.Str, '[ 1 (2) /3 ]', 'parse array';
}

given PDF::Grammar::COS.parse('<</a 42/BB(Hi)>>', :rule<object>, :$actions) {
      my CosDict:D $node = .ast;
      is $node.Str, '<< /a 42 /BB (Hi) >>', 'parse dict';
}

my $stream = q:to<--END-->;
    << /Length 45 >> stream
    BT
    /F1 24 Tf
    100 250 Td (Hello, world!) Tj
    ET
    endstream
    --END--

given PDF::Grammar::COS.parse( $stream, :rule<object>, :$actions) {
    my CosStream:D $node = .ast;
    is $node.Str, $stream.chomp, 'parse stream';
}

my $ind-obj = "123 4 obj\n{$stream}endobj";

with PDF::Grammar::COS.parse( $ind-obj, :rule<ind-obj>, :$actions) {
    my CosIndObj:D $node = .ast;
    note $/.from.raku;
    is $node.Str.chomp, $ind-obj, 'parse indirect object';
}
