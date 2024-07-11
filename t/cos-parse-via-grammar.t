use PDF::Grammar::COS;
use PDF::Native::COS;
use PDF::Native::COS::Actions;
use Test;

plan 19;

my PDF::Native::COS::Actions:D $actions .= new: :lite;

given PDF::Grammar::COS.parse('123', :rule<object>, :$actions) {
      my COSInt:D $node = .ast;
      is $node.Str, '123', 'parse int';
}

given PDF::Grammar::COS.parse('123.45', :rule<object>, :$actions) {
      my COSReal:D $node = .ast;
      is $node.Str, '123.45', 'parse real';
}

given PDF::Grammar::COS.parse('.45', :rule<object>, :$actions) {
      my COSReal:D $node = .ast;
      is $node.Str, '0.45', 'parse real fraction';
}

given PDF::Grammar::COS.parse('(Hello,\40World\n)', :rule<object>, :$actions) {
      my COSLiteralString:D $node = .ast;
      is $node.Str.chomp, 'Hello, World', 'parse literal';
      is $node.write, '(Hello, World\n)', 'parse literal';
}

my $hex = '<4E6F762073686D6F7A206B6120706f702e>';
given PDF::Grammar::COS.parse($hex, :rule<object>, :$actions) {
      my COSHexString:D $node = .ast;
      is $node.value, 'Nov shmoz ka pop.';
      is $node.Str, 'Nov shmoz ka pop.', 'parse hex';
      is $node.write, $hex.lc, 'parse hex';
}

given PDF::Grammar::COS.parse('/Hello,#20World#21', :rule<object>, :$actions) {
      my COSName:D $node = .ast;
      is $node.Str, 'Hello, World!', 'parse name';
      is $node.write, '/Hello,#20World!', 'parse name';
}

given PDF::Grammar::COS.parse('true', :rule<object>, :$actions) {
      my COSBool:D $node = .ast;
      is $node.Str, 'true', 'parse bool';
}

given PDF::Grammar::COS.parse('false', :rule<object>, :$actions) {
      my COSBool:D $node = .ast;
      is $node.Str, 'false', 'parse bool';
}

given PDF::Grammar::COS.parse('null', :rule<object>, :$actions) {
      my COSNull:D $node = .ast;
      is $node.Str, 'null', 'parse null';
}

given PDF::Grammar::COS.parse('12 3 R', :rule<object>, :$actions) {
      my COSRef:D $node = .ast;
      is $node.Str, '12 3 R', 'parse indirect reference';
}

given PDF::Grammar::COS.parse('[1(2) /3  ]', :rule<object>, :$actions) {
    my COSArray:D $node = .ast;
    is $node.Str, '[ 1 (2) /3 ]', 'parse array';
}

given PDF::Grammar::COS.parse('<</a 42/BB(Hi)>>', :rule<object>, :$actions) {
      my COSDict:D $node = .ast;
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
    my COSStream:D $node = .ast;
    is $node.Str, $stream.chomp, 'parse stream';
    is-deeply COSNode.COERCE($node.ast).Str.lines, $stream.lines;
}

my $ind-obj = "123 4 obj\n{$stream}endobj";

with PDF::Grammar::COS.parse( $ind-obj, :rule<ind-obj>, :$actions) {
    my COSIndObj:D $node = .ast;
    is $node.Str.chomp, $ind-obj, 'parse indirect object';
}
