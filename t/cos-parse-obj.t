use PDF::Native::COS;
use Test;

plan 75;

given COSNode.parse('123') {
    .&isa-ok: COSInt;
    is .Str, '123', 'parse int';
}

given COSNode.parse('123.45') {
    .&isa-ok: COSReal;
    is .Str, '123.45', 'parse real';
}

given COSNode.parse('.45') {
    .&isa-ok: COSReal;
    is .Str, '0.45', 'parse real fraction';
}

given COSNode.parse('[]') {
    .&isa-ok: COSArray;
    is .Str, '[ ]', 'parse array';
}

given COSNode.parse("\n \%xxx\n[\%yyy\n]\%zzz\n ") {
    .&isa-ok: COSArray;
    is .Str, '[ ]', 'parse array + ws/comments';
}

given COSNode.parse('(Hello,\40World\n)') {
    .&isa-ok: COSLiteralString;
    is .Str.chomp, 'Hello, World', 'parse literal';
    is .write, '(Hello, World\n)', 'parse literal';
}

given COSNode.parse('(\((\{}\)))') {
    .&isa-ok: COSLiteralString;
    is .Str, '(({}))', 'parse parens';
    is .write, '(\(\({}\)\))', 'parse parens';
}

given COSNode.parse('(\7\07\007\0077)') {
    .&isa-ok: COSLiteralString;
    is-deeply .Str, (7.chr xx 4).join ~ '7', 'parse octal escapes';
    is-deeply .write, (flat '(', 7.chr xx 4, '7)').join, 'parse octal escapes';
}

for '(\n\r\t\f\b\\' ~ 10.chr ~ 'X\Y)', '(\n\r\t\f\b\\' ~ 13.chr ~ 10.chr ~ 'X\Y)' {
    given COSNode.parse($_) {
        .&isa-ok: COSLiteralString;
        is-deeply .Str, "\n\r\t\x[c]\x[8]XY", 'parse: '~.raku;
        is-deeply .write, "(\\n\\r\\t\\f\\bXY)", 'parse: '~.raku;
    }
}

my $hex = '<4E6F762073686D6F7A206B6120706f702e>';
given COSNode.parse($hex) {
    .&isa-ok: COSHexString;
    is .Str, 'Nov shmoz ka pop.', 'parse hex';
    is .write.lc, $hex.lc, 'parse hex';
}

for ('<4E60>' ,'< 4 E 6 0 >', '<4E6>', '<4E6 >', '< 4E6>') {
    is COSNode.parse($_).Str, 'N`', "parse: $_";
    is COSNode.parse($_).write, '<4e60>', "parse: $_";
}

for ('<>' ,'< >', '<  >') {
    is COSNode.parse($_).Str, '', "parse: $_";
    is COSNode.parse($_).write, '<>', "parse: $_";
}

given COSNode.parse('/Hello,#20World#21') {
    .&isa-ok: COSName;
    is .Str, 'Hello, World!', 'parse name';
    is .write, '/Hello,#20World!', 'parse name';
}

for '', 'PTEX.Fullbanner' -> $s {
    given COSNode.parse('/'~$s) {
        .&isa-ok: COSName;
        is .Str, $s, 'literal name parse ' ~ .raku;
        is .write, '/' ~ $s;
    }
}
given COSNode.parse('true') {
    .&isa-ok: COSBool;
    is .Str, 'true', 'parse bool';
    is .write, 'true', 'parse bool';
}

given COSNode.parse('false') {
    .&isa-ok: COSBool;
    is .Str, 'false', 'parse bool';
    is .write, 'false', 'parse bool';
}

given COSNode.parse('null') {
    .&isa-ok: COSNull;
    is .Str, 'null', 'parse null';
    is .write, 'null', 'parse null';
}

given COSNode.parse('<< /a 42 >>') {
    .&isa-ok: COSDict;
    is .Str, '<< /a 42 >>', 'parse dict';
}

given COSNode.parse('<</<>>>') {
    .&isa-ok: COSDict;
    is .Str, '<< / <> >>', 'parse dict';
}


given COSNode.parse('12 3 R') {
    .&isa-ok: COSRef;
    is .Str, '12 3 R', 'parse indirect reference';
}

given COSNode.parse('[123 .45/foo<aa>true 12 3 R false null]') {
    .&isa-ok: COSArray;
    is .Str, '[ 123 0.45 /foo <aa> true 12 3 R false null ]', 'parse array';
}

subtest 'invalid numbers', {
    for <++0 0+0 0.. ..0 0a 0..0 + - . .+1> {
        is-deeply COSNode.parse($_), COSNode, "invalid number: $_";
    }
}

given COSNode.parse('[1(2) /3 -0 -1 -.1 ]') {
    .&isa-ok: COSArray;
    is .Str, '[ 1 (2) /3 0 -1 -0.1 ]', 'parse array';
}

given COSNode.parse('<</a 42/BB(Hi)>>') {
    .&isa-ok: COSDict;
    is .Str, '<< /a 42 /BB (Hi) >>', 'parse dict';
}

