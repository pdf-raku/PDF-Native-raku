#| Native Cos object construction actions for PDF::Grammar::COS
unit class PDF::Native::Cos::Actions;

use PDF::Grammar::COS::Actions;
also is PDF::Grammar::COS::Actions;

use PDF::Native::Cos;
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
    make ($ast.key eq 'literal' ?? CosLiteral !! CosHexString).new: :$value;
}

method name($/) {
    # names are utf-8 encoded
    my $value = CArray[uint32].new: Buf.new( $<name-bytes>».ast ).decode('utf8-c8').ords;
    make CosName.new: :$value;
}

method array($/) {
    my CArray[CosNode] $values .= new: @<object>».ast;
    make CosArray.new: :$values;
}

method dict($/) {
    my CArray[CosName] $keys   .= new: @<name>.map: *.ast;
    my CArray[CosNode] $values .= new: @<object>».ast;

    make CosDict.new: :$keys, :$values;
}

method ind-ref($/) {
    my Int $obj-num = $<obj-num>.ast;
    my Int $gen-num = $<gen-num>.ast;
    make CosRef.new: :$obj-num, :$gen-num;
}

method ind-obj($/) {
    my $obj-num = $<obj-num>.ast;
    my $gen-num = $<gen-num>.ast;
    my $value = $<object>.ast;
    make CosIndObj.new: :$obj-num, :$gen-num, :$value;
}

method object:sym<true>($/)   { make CosBool.new :value }
method object:sym<false>($/)  { make CosBool.new :!value }
method object:sym<null>($/)   { make CosNull.new }
method object:sym<number>($/) {
    my $value = $<number>.ast;
    make ($value.isa(Int) ?? CosInt !! CosReal).new: :$value;
}
method object:sym<dict>($/) {
    my CosDict:D $dict = $<dict>.ast;
    make do with $<stream> {
    	my Blob:D $value = .ast.encode: "latin-1";
        CosStream.new: :$dict, :$value;
    }
    else {
        $dict;
    }
}
