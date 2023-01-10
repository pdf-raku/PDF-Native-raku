use PDF::COS :LatinStr;
use PDF::Native::COS;
use NativeCall;
use PDF::Native :libpdf, :types;

class cos_struct_elem is repr('CStruct') {
    also is cos_indobj;

    has Str        $.Type;
    has Str        $!S;       method S  { $!S }
    has Str        $!ID;      method ID { $!ID }
    has cos_indref $!Pg;      method Pg { $!Pg }
    has cos_struct_elem $!P;
    has CArray[cos_struct_elem] $!K;

    submethod TWEAK(
        LatinStr :$S,
        LatinStr :$ID,
        cos_indref :$Pg,
    ) {
        $!S  := $_ with $S;
        $!ID := $_ with $ID; 
        $!Pg := $_ with $Pg;
    }

    method write(Blob[uint8] $buf, int32 $buf-len)
        returns int32
        is symbol('pdf_cos_struct_elem_write')
        is native(libpdf) {*};
}

class cos_mcr is repr('CStruct') {
    also is cos_indobj;

    has Str        $.Type;
    has cos_indref $!Pg;      method Pg { $!Pg }
    has cos_indref $!Stm;
    has int32      $.MCID;

    submethod TWEAK(
        cos_indref :$Pg,
        cos_indref :$Stm,
    ) {
        $!Pg := $_ with $Pg;
        $!Stm := $_ with $Stm;
    }

    method write(Blob[uint8] $buf, int32 $buf-len)
        returns int32
        is symbol('pdf_cos_mcr_write')
        is native(libpdf) {*};
}

class cos_objr is repr('CStruct') {
    also is cos_indobj;

    has Str        $.Type;
    has cos_indref $!Pg;      method Pg { $!Pg }
    has cos_indref $!Obj;     method Obj { $!Obj }

    submethod TWEAK(
        cos_indref :$Pg,
        cos_indref :$Obj,
    ) {
        $!Pg := $_ with $Pg;
        $!Obj := $_ with $Obj;
    }

    method write(Blob[uint8] $buf, int32 $buf-len)
        returns int32
        is symbol('pdf_cos_objr_write')
        is native(libpdf) {*};
}
