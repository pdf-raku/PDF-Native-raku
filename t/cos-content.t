use PDF::Native::COS;
use NativeCall;
use Test;

plan 16;

my COSContent $content .= parse: "BT /F1 24 Tf  100 250 Td (Hello, world!) Tj ET";
ok $content.defined, "content parse";

is-deeply $content.write.lines, ('BT', '  /F1 24 Tf', '  100 250 Td', '  (Hello, world!) Tj', 'ET');

is-deeply $content.ast, 'content' => [ :BT[], :Tf[:name("F1"), 24], :Td[100, 250], :Tj[:literal("Hello, world!")], :ET[] ], 'ast';

is-deeply $content.COERCE($content.ast). ast, $content.ast, "COERCE";

$content .= parse: "BI ID abc EI";
is-deeply $content.ast, 'content' => [:BI[], :ID([:dict{}, :encoded<abc>]), :EI[]];

is-deeply $content.COERCE($content.ast). ast, $content.ast, "COERCE";

is-deeply $content.write.lines, ('BI', 'ID', 'abc', 'EI');

$content .= parse: "BI /Foo (bar) ID  abc EI";
is-deeply $content.ast, 'content' => [:BI[], :ID[:dict{:Foo(:literal<bar>)}, :encoded(' abc')], :EI[]];

is-deeply $content.write.lines, ('BI', '/Foo (bar) ID', ' abc', 'EI');

$content .= parse: "BI ID ab EIcEI EI";
is-deeply $content.ast, 'content' => [:BI[], :ID[:dict{},  :encoded("ab EIcEI")], :EI[]];

$content .= parse: "BI /L 7 ID  abc EI EI";
is-deeply $content.ast, 'content' => [:BI[], :ID[:dict{:L(7)}, :encoded(" abc EI")], :EI[]];

is-deeply $content.write.lines, ('BI', '/L 7 ID', ' abc EI', 'EI');

$content .= parse: "XX /Y 6 ZZ 42 Td";
is-deeply $content.ast, 'content' => ['??' => :XX[], '??' => :ZZ[:name<Y>, 6], '??' => :Td[42]];

is-deeply $content.write.lines, ('XX', '/Y 6 ZZ', '42 Td');

$content .= COERCE: [ :BT[], :comment["test comment"], :ET[] ];

is-deeply $content.ast, 'content' => [ :BT[], :comment["test comment"], :ET[] ];
is-deeply $content.write.lines, ('BT', '  % test comment', 'ET');

done-testing;
