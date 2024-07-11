#| Native Cos object construction actions for PDF::Grammar::COS
unit class PDF::Native::COS::Actions;

use PDF::Grammar::COS::Actions;
also is PDF::Grammar::COS::Actions;

use PDF::Native::COS;
use NativeCall;

method int($/) {
    make $/.Int;
}

method number($/) {
    make $<numeric>.ast;
}

method string($/) {
    my Pair $ast = $<string>.ast;
    my Blob:D $value = $ast.value.encode: "latin-1";
    make ($ast.key eq 'literal' ?? COSLiteralString !! COSHexString).new: :$value;
}

method name($/) {
    # names are utf-8 encoded
    my $value = CArray[uint32].new: Buf.new( $<name-bytes>».ast ).decode('utf8-c8').ords;
    make COSName.new: :$value;
}

method array($/) {
    my CArray[COSNode] $values .= new: @<object>».ast;
    make COSArray.new: :$values;
}

method dict($/) {
    my CArray[COSName] $keys   .= new: @<name>.map: *.ast;
    my CArray[COSNode] $values .= new: @<object>».ast;

    make COSDict.new: :$keys, :$values;
}

method ind-ref($/) {
    my Int $obj-num = $<obj-num>.ast;
    my Int $gen-num = $<gen-num>.ast;
    make COSRef.new: :$obj-num, :$gen-num;
}

method ind-obj($/) {
    my $obj-num = $<obj-num>.ast;
    my $gen-num = $<gen-num>.ast;
    my $value = $<object>.ast;
    make COSIndObj.new: :$obj-num, :$gen-num, :$value;
}

method object:sym<true>($/)   { make COSBool.new :value }
method object:sym<false>($/)  { make COSBool.new :!value }
method object:sym<null>($/)   { make COSNull.new }
method object:sym<number>($/) {
    my $value = $<number>.ast;
    make ($value.isa(Int) ?? COSInt !! COSReal).new: :$value;
}
method object:sym<dict>($/) {
    my COSDict:D $dict = $<dict>.ast;
    make do with $<stream> {
    	my Blob:D $value = .ast.encode: "latin-1";
        COSStream.new: :$dict, :$value;
    }
    else {
        $dict;
    }
}
