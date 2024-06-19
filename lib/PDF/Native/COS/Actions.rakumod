unit class PDF::Native::COS::Actions;

use PDF::Grammar::COS::Actions;
also is PDF::Grammar::COS::Actions;

use PDF::Native::COS;

method number($/) {
    my $value = $<numeric>.ast.value;
    make ($value.isa(Int) ?? CosInt !! CosReal).new: :$value;
}

