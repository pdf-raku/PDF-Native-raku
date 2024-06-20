#| Native Cos object construction actions for PDF::Grammar::COS
unit class PDF::Native::COS::Actions;

use PDF::Grammar::COS::Actions;
also is PDF::Grammar::COS::Actions;

use PDF::Native::COS;

method int($/) {
    my $value =  $/.Int;
    make CosInt.new :$value;
}

method number($/) {
    my $value = $<numeric>.ast;
    make ($value.isa(CosNode) ?? $value !! CosReal.new: :$value);
}

method string($/) {
    my Pair $ast = $<string>.ast;
    my Blob:D $value = $ast.value.encode: "latin-1";
    make ($ast.key eq 'literal' ?? CosLiteral !! CosHexString).new: :$value;
}

