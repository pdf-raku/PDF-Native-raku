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

