unit module PDF::Native::COS;

use NativeCall;
use PDF::Native :libpdf, :types;

our class cos_indobj is repr('CStruct') is export {

    has int32 $.obj-num;
    has int16 $.gen-num;

    method Str {
        my buf8 $buf .= allocate(32);
        my $n := self.write: $buf, $buf.bytes;
        $buf.subbuf(0, $n).decode: "latin-1";
    }
}

our class cos_indref is cos_indobj is repr('CStruct') is export {
    method write(Blob[uint8] $buf, int32 $buf-len)
        returns int32
        is symbol('pdf_cos_indref_write')
        is native(libpdf) {*};

}
