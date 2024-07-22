use PDF::Native::COS;
use NativeCall;
use Test;

plan 9;

my COSContent $content .= parse: "BT /F1 24 Tf  100 250 Td (Hello, world!) Tj ET";
ok $content.defined, "content parse";

is-deeply $content.write.lines, ('BT', '  /F1 24 Tf', '  100 250 Td', '  (Hello, world!) Tj', 'ET');

is-deeply $content.ast, 'content' => [ :BT[], :Tf[:name("F1"), 24], :Td[100, 250], :Tj[:literal("Hello, world!")], :ET[] ], 'ast';

$content .= parse: "BI ID abc EI";
is-deeply $content.ast, 'content' => [:BI[], :ID([:dict{}, :encoded<abc>]), :EI[]];

todo "inline image write";
is-deeply $content.write.lines, ('BI', 'ID', 'abc', 'EI');

$content .= parse: "BI /Foo (bar) ID abc EI";
is-deeply $content.ast, 'content' => [:BI[], :ID([:dict{:Foo(:literal<bar>)}, :encoded<abc>]), :EI[]];

is-deeply $content.write.lines, ('BI', '/Foo (bar) ID', 'abc', 'EI');

$content .= parse: "BI /L 6 ID abc EI EI";
is-deeply $content.ast, 'content' => [:BI[], :ID([:dict{:L(6)}, :encoded("abc EI")]), :EI[]];

is-deeply $content.write.lines, ('BI', '/L 6 ID', 'abc EI', 'EI');

done-testing;
