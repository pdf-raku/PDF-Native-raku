use PDF::Native::COS;
use Test;

my buf8 $buf .= allocate(128);

for (
    (COSNode, 42, COSInt.new(:value(42))),
    (COSNode, COSInt.new(:value(42)), COSInt.new(:value(42))),
    (COSNode, 4.2, COSReal.new(:value(42e-1))),
    (COSNode, Any, COSNull.new()),
    (COSNode, True, COSBool.new(:value)),
    (COSNode, False, COSBool.new(:!value)),
    (COSNode, [42, 4.2, True, Any], COSArray.parse("[42 4.2 true null]")),
    (COSNode, COSNode.parse('[]'), COSNode.parse('[]')),
    (COSNode, COSNode.parse('<<>>'), COSNode.parse('<<>>')),
    (COSNode, %( X => [42] ), COSNode.parse('<</X [ 42 ] >>')),
    (COSNode, %( :dict{ :Length(42) }, :encoded("Hi There") ), "<< /Length 42 >> stream\nHi There\nendstream"),
    (COSRef, [42, 10], '42 10 R'),
    (COSRef, [42, 10, 'junk'], '42 10 R'),
    (COSName, 'fred', '/fred'),
    (COSLiteralString, 'fred', '(fred)'),
    (COSHexString, 'fred', '<66726564>'),
    (COSIndObj, [42, 10, { :foo(42) }], "42 10 obj\n<< /foo 42 >>\nendobj\n"),
    ) -> @ ( $class, $value, $result is copy ) {
    $result .= write if $result.isa(COSNode);
    is $class.COERCE($value).write(:$buf), $result, "{$class.raku} coercement of {$value.isa(COSNode) ?? "node " ~ $value.write !! $value.raku}";
}

throws-like COSNode.COERCE("x"), X::AdHoc, :message("COERCE(Str:D) requires a specific class:  COSName, COSLiteralString, or COSHexString");

done-testing;
