use PDF::Native::Cos;
use Test;

my buf8 $buf .= allocate(128);

for (
    (CosNode, 42, CosInt.new(:value(42))),
    (CosNode, CosInt.new(:value(42)), CosInt.new(:value(42))),
    (CosNode, 4.2, CosReal.new(:value(42e-1))),
    (CosNode, Any, CosNull.new()),
    (CosNode, True, CosBool.new(:value)),
    (CosNode, False, CosBool.new(:!value)),
    (CosNode, [42, 4.2, True, Any], CosArray.parse("[42 4.2 true null]")),
    (CosNode, CosNode.parse('[]'), CosNode.parse('[]')),
    (CosNode, CosNode.parse('<<>>'), CosNode.parse('<<>>')),
    (CosNode, %( X => [42] ), CosNode.parse('<</X [ 42 ] >>')),
    (CosNode, %( :dict{ :Length(42) }, :encoded("Hi There") ), "<< /Length 42 >> stream\nHi There\nendstream"),
    (CosRef, [42, 10], '42 10 R'),
    (CosName, 'fred', '/fred'),
    (CosLiteralString, 'fred', '(fred)'),
    (CosHexString, 'fred', '<66726564>'),
    (CosIndObj, [42, 10, { :foo(42) }], "42 10 obj\n<< /foo 42 >>\nendobj\n"),
    ) -> @ ( $class, $value, $result is copy ) {
    $result .= write if $result.isa(CosNode);
    is $class.COERCE($value).write(:$buf), $result, "{$class.raku} coercement of {$value.isa(CosNode) ?? "node " ~ $value.write !! $value.raku}";
}

done-testing;
