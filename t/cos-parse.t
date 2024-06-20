use PDF::Grammar::COS;
use PDF::Native::COS;
use PDF::Native::COS::Actions;
use Test;

plan 7;

my PDF::Native::COS::Actions:D $actions .= new: :lite;

given PDF::Grammar::COS.parse('123', :rule<int>, :$actions) {
      my CosInt:D $node = .ast;
      is $node.Str, '123', 'parse int';
}

given PDF::Grammar::COS.parse('123', :rule<number>, :$actions) {
      my CosInt:D $node = .ast;
      is $node.Str, '123', 'parse int number';
}

given PDF::Grammar::COS.parse('123.45', :rule<number>, :$actions) {
      my CosReal:D $node = .ast;
      is $node.Str, '123.45', 'parse real';
}

given PDF::Grammar::COS.parse('.45', :rule<number>, :$actions) {
      my CosReal:D $node = .ast;
      is $node.Str, '0.45', 'parse real fraction';
}

given PDF::Grammar::COS.parse('(Hello,\40World\n)', :rule<string>, :$actions) {
      my CosLiteral:D $node = .ast;
      is $node.Str, '(Hello, World\n)', 'parse literal';
}

my $hex = '<4E6F762073686D6F7A206B6120706f702e>';
given PDF::Grammar::COS.parse($hex, :rule<string>, :$actions) {
      my CosHexString:D $node = .ast;
      is $node.value, 'Nov shmoz ka pop.';
      is $node.Str, $hex.lc, 'parse hex';
}
