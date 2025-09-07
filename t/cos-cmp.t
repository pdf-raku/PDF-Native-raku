use PDF::Native::COS;
use Test;

multi sub parse(Str:D $str, :$rule! where 'ind-obj') {
    COSIndObj.parse($str, :scan);
}

multi sub parse(Str:D $str) {
    COSNode.parse($str);
}

my COSInt:D $one = parse("1");

subtest 'basic', {
    is COSNode.cmp(COSNode), +COS_CMP_EQUAL;

    is COSNode.cmp($one), +COS_CMP_DIFFERENT_TYPE;

    is $one.cmp(COSNode), +COS_CMP_DIFFERENT_TYPE;
    is $one.cmp($one), +COS_CMP_EQUAL;
    is $one.cmp(parse("1")), +COS_CMP_EQUAL;
}

subtest 'numeric', {
    my COSInt:D $two = parse("2");

    is $two.cmp(parse("2")), +COS_CMP_EQUAL;
    is $two.cmp(2), +COS_CMP_EQUAL;
    is $two.cmp(3), +COS_CMP_DIFFERENT;
    is $two.cmp(parse("2.0")), +COS_CMP_SIMILAR;
    is $two.cmp(parse("2.1")), +COS_CMP_DIFFERENT;
    is $two.cmp($one), +COS_CMP_DIFFERENT;
    is $one.cmp($two), +COS_CMP_DIFFERENT;
}

subtest 'string', {
    my COSLiteralString:D $lx = parse("(x)");
    my COSLiteralString:D $ly = parse("(y)");
    my COSHexString:D $hx .= new: :value('x'.encode: "latin-1");
    my COSHexString:D $hy .= new: :value('y'.encode: "latin-1");
    is $lx.cmp( parse("(x)") ), +COS_CMP_EQUAL;
    is $lx.cmp($one), +COS_CMP_DIFFERENT_TYPE;
    is $lx.cmp($ly), +COS_CMP_DIFFERENT;
    is $ly.cmp($lx), +COS_CMP_DIFFERENT;
    is $lx.cmp($hx), +COS_CMP_SIMILAR;
    is $lx.cmp($hy), +COS_CMP_DIFFERENT;
}

subtest 'array', {
    is parse("[1]").cmp(parse("[1]")), +COS_CMP_EQUAL;
    is parse("[1]").cmp(parse("[2]")), +COS_CMP_DIFFERENT;
    is parse("[1]").cmp(parse("[(1)]")), +COS_CMP_DIFFERENT;
    is parse("[1]").cmp(parse("[1 2]")), +COS_CMP_DIFFERENT;
    is parse("[1]").cmp(parse("<< >>")), +COS_CMP_DIFFERENT_TYPE;
    is parse("[1 2]").cmp(parse("[ 1 3 ]")), +COS_CMP_DIFFERENT;
    is parse("[1 2]").cmp(parse("[ 1 << >> ]")), +COS_CMP_DIFFERENT;
}

subtest 'name', {
    is parse("/a").cmp(parse("/a")), +COS_CMP_EQUAL;
    is parse("/a").cmp(parse("/b")), +COS_CMP_DIFFERENT;
    is parse("/a").cmp(parse("/ab")), +COS_CMP_DIFFERENT;
    is parse("/a").cmp(parse("/")), +COS_CMP_DIFFERENT;
}

subtest 'dict', {
    is parse("<</a 1>>").cmp(parse("<< /a 1 >>")), +COS_CMP_EQUAL;
    is parse("<</a 1>>").cmp(parse("<< /a 2 >>")), +COS_CMP_DIFFERENT;
    is parse("<</a 1>>").cmp(parse("<< /b 1 >>")), +COS_CMP_DIFFERENT;
    is parse("<</a 1>>").cmp(parse("<< /a 1 /b 2>>")), +COS_CMP_DIFFERENT;
    is parse("<</a 1>>").cmp(parse("<< /a 1.0>>")), +COS_CMP_SIMILAR;
    is parse("<</a [1 2 3]>>").cmp(parse("<< /a [1 2 3]>>")), +COS_CMP_EQUAL;
    is parse("<</a [1 2 3]>>").cmp(parse("<< /a [1 2]>>")), +COS_CMP_DIFFERENT;
    is parse("<</a [1 2 3]>>").cmp(parse("<< /a [1 2.0 3]>>")), +COS_CMP_SIMILAR;
    is parse("<</a 1>>").cmp(parse("<< /a 1 /b null>>")), +COS_CMP_EQUAL;
    is parse("<</a 1 /b 2>>").cmp(parse("<< /b 2 /a 1>>")), +COS_CMP_SIMILAR;
}

subtest 'indirect references', {
    is parse("42 0 R").cmp(parse("42 0 R")), +COS_CMP_EQUAL;
    is parse("42 0 R").cmp(parse("43 0 R")), +COS_CMP_DIFFERENT;
    is parse("42 0 R").cmp(parse("42 1 R")), +COS_CMP_DIFFERENT;
}

sub test-stream($entry, $data) {
    my COSIndObj $ind-obj = parse qq:to<END>, :rule<ind-obj>;
    1 0 obj
    << /a $entry /Length {$data.chars} >>
    stream
    {$data}
    endstream
    endobj
    END
    $ind-obj.value;
}

subtest 'streams', {
    is test-stream(1, 'xxx').cmp( test-stream(1, 'xxx')), +COS_CMP_EQUAL;
    is test-stream(1, 'xxx').cmp( test-stream(2, 'xxx')), +COS_CMP_DIFFERENT;
    is test-stream(1, 'xxx').cmp( test-stream('1.0', 'xxx')), +COS_CMP_SIMILAR;
    is test-stream(1, 'xxx').cmp( test-stream(1, 'xxy')), +COS_CMP_DIFFERENT;
    is test-stream(1, 'xxx').cmp( test-stream(1, 'xxxx')), +COS_CMP_DIFFERENT;
}

subtest 'indirect objects', {
    is parse("42 0 obj\n1\nendobj", :rule<ind-obj>).cmp(parse("42 0 obj\n1\nendobj", :rule<ind-obj>)), +COS_CMP_EQUAL;
    is parse("42 0 obj\n1\nendobj", :rule<ind-obj>).cmp(parse("42 0 obj(x)\nendobj", :rule<ind-obj>)), +COS_CMP_DIFFERENT;
    is parse("42 0 obj\n1\nendobj", :rule<ind-obj>).cmp(parse("42 0 obj\n1.0\nendobj", :rule<ind-obj>)), +COS_CMP_SIMILAR;
}

done-testing;
