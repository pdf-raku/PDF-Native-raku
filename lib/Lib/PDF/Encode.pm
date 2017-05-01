use v6;

module Lib::PDF::Encode {

    use NativeCall;
    use Lib::PDF :libpdf;

    sub pdf_encode_base64(Blob, size_t, Blob, size_t)  is native(&libpdf) { * }

    sub out-alloc(Blob $in) {
	my \out-blocks = ($in.bytes div 3) + ($in.bytes %% 3 ?? 0 !! 1);
	buf8.allocate: out-blocks * 4;
    }

    our sub base64-encode(Blob $in, Blob $out = out-alloc($in)) is export(:base64) {
	pdf_encode_base64($in, $in.bytes, $out, $out.bytes);
	$out;
    }
}
