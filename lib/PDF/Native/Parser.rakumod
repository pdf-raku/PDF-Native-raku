unit class PDF::Native::Parser;

use PDF::Native::Defs :libpdf;
use PDF::Native::Cos;
use Method::Also;
use NativeCall;

sub cos_parse_ind_obj(CosIndObj, Blob, size_t, size_t is rw  --> CosIndObj) is native(libpdf) {*}

multi method parse(Blob:D $buf, UInt:D $size = $buf.bytes, :rule($)! where 'ind-obj'  --> CosIndObj) is also<parse-ind-obj> {
    my CosIndObj $ind-obj = cos_parse_ind_obj(CosIndObj, $buf, $size, my size_t $pos = 0);
    return $_ with $ind-obj;
    fail "nah";
}
