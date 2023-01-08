unit class PDF::Native::COS::StructElem is repr('CStruct');

use PDF::COS :LatinStr;
use PDF::Native::COS;
also is cos_indobj;

use NativeCall;
use PDF::Native :libpdf, :types;

has Str        $.Type;
has Str        $!S;       method S  { $!S }
has Str        $!ID;      method ID { $!ID }
has cos_indref $!Pg;      method Pg { $!Pg }
has PDF::Native::COS::StructElem $!P;
has CArray[PDF::Native::COS::StructElem] $!K;

submethod TWEAK(
    LatinStr :$S,
    LatinStr :$ID,
    cos_indref :$Pg,
) {
    $!S  := $_ with $S;
    $!ID := $_ with $ID; 
    $!Pg := $_ with $Pg;
}

method Str {
    my buf8 $buf .= allocate(2048);
    my $n := self.write: $buf, $buf.bytes;
    $buf.subbuf(0, $n).decode: "latin-1";
}

method write(Blob[uint8] $buf, int32 $buf-len)
    returns int32
    is symbol('pdf_cos_struct_elem_write')
    is native(libpdf) {*};
