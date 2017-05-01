use v6;

module Lib::PDF::Encode {

    use NativeCall;
    use Lib::PDF :libpdf;

    sub pdf_encode_base64(Blob, size_t, Blob, size_t)  is native(&libpdf) { * }
    sub pdf_decode_base64(Blob, size_t, Blob, size_t --> int32)  is native(&libpdf) { * }

    sub enc-alloc(Blob $in) {
	my \out-blocks = ($in.bytes div 3) + ($in.bytes %% 3 ?? 0 !! 1);
	buf8.allocate: out-blocks * 4;
    }

    sub dec-alloc(Blob $in) {
	my \out-blocks = ($in.bytes + 3) div 4;
	buf8.allocate: out-blocks * 3;
    }

    our sub base64-encode(Blob $in, Blob $out = enc-alloc($in)) is export(:base64) {
	pdf_encode_base64($in, $in.bytes, $out, $out.bytes);
	$out;
    }

    our sub base64-decode(Blob $in, Blob $out = dec-alloc($in)) is export(:base64) {
	my int32 $n = pdf_decode_base64($in, $in.bytes, $out, $out.bytes);
	die "unable to decode as base64"
	    if $n < 0;
	$out.reallocate($n)
	    unless $n == $out.bytes;
	$out;
    }
}
