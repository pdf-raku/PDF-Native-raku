use v6;

module PDF::Native::Buf {

    use NativeCall;
    use PDF::Native :libpdf;

    our proto sub unpack( $, $ --> Blob) is export(:pack) {*};
    our proto sub pack( $, $ --> Blob) is export(:pack) {*};

    sub pdf_buf_unpack_1(Blob, Blob, size_t)  is native(libpdf) { * }
    sub pdf_buf_unpack_2(Blob, Blob, size_t)  is native(libpdf) { * }
    sub pdf_buf_unpack_4(Blob, Blob, size_t)  is native(libpdf) { * }
    sub pdf_buf_unpack_16(Blob, Blob, size_t) is native(libpdf) { * }
    sub pdf_buf_unpack_24(Blob, Blob, size_t) is native(libpdf) { * }
    sub pdf_buf_unpack_32(Blob, Blob, size_t) is native(libpdf) { * }
    sub pdf_buf_unpack_64_W(Blob, Blob, size_t, Blob, size_t) is native(libpdf) { * }

    sub pdf_buf_pack_1(Blob, Blob, size_t)  is native(libpdf) { * }
    sub pdf_buf_pack_2(Blob, Blob, size_t)  is native(libpdf) { * }
    sub pdf_buf_pack_4(Blob, Blob, size_t)  is native(libpdf) { * }
    sub pdf_buf_pack_16(Blob, Blob, size_t) is native(libpdf) { * }
    sub pdf_buf_pack_24(Blob, Blob, size_t) is native(libpdf) { * }
    sub pdf_buf_pack_32(Blob, Blob, size_t) is native(libpdf) { * }
    sub pdf_buf_pack_64_W(Blob, Blob, size_t, Blob, size_t) is native(libpdf) { * }

    my subset PackingSize where 1|2|4|8|16|24|32;
    sub container(PackingSize $bits) {
        $bits <= 8 ?? uint8 !! ($bits > 16 ?? uint32 !! uint16)
    }

    sub do-packing($n, $m, $in is copy, &pack) {
        my uint32 $in-len = $in.elems;
        $in = Blob[container($n)].new($in)
            unless $in.isa(Blob);
        my $out := Blob[container($m)].allocate(($in-len * $n + $m-1) div $m);
        &pack($in, $out, $in-len);
        $out;
    }
    multi sub unpack($nums!, PackingSize $n!)  {
        my $type = container($n);
        my &packer = %(
            1 => &pdf_buf_unpack_1,
            2 => &pdf_buf_unpack_2,
            4 => &pdf_buf_unpack_4,
            16 => &pdf_buf_unpack_16,
            24 => &pdf_buf_unpack_24,
            32 => &pdf_buf_unpack_32,
        ){$n};
            do-packing(8, $n, $nums, &packer);
    }

    multi sub pack($nums!, PackingSize $n!)  {
        my $type = container($n);
        my &packer = %(
            1 => &pdf_buf_pack_1,
            2 => &pdf_buf_pack_2,
            4 => &pdf_buf_pack_4,
            16 => &pdf_buf_pack_16,
            24 => &pdf_buf_pack_24,
            32 => &pdf_buf_pack_32,
        ){$n};
        do-packing($n, 8, $nums, &packer);
    }

    #| variable resampling, e.g. to decode/encode:
    #|   obj 123 0 << /Type /XRef /W [1, 3, 1]
    multi sub unpack( $in!, Array $W!)  {
        my size_t $in-len = +$in;
        my blob8 $in-buf = $in ~~ blob8 ?? $in !! blob8.new($in);
        my blob8 $W-buf .= new($W);
        my size_t $out-len = ($in-len * +$W) div $W.sum;
        my $out-buf := buf64.allocate($out-len);
        pdf_buf_unpack_64_W($in-buf, $out-buf, $in-len, $W-buf, +$W);
	my uint64 @shaped[$out-len div +$W;+$W] Z= $out-buf;
        @shaped;
    }

    multi sub pack( $in, Array $W!)  {
        my $rows = $in.elems;
        my $cols-in = +$W;
        my $cols-out = $W.sum;
	my $out = buf8.allocate($rows * $cols-out);
        my blob64 $in-buf = $in ~~ blob64 ?? $in !! blob64.new($in);
        my blob8 $W-buf .= new($W);
        my size_t $out-len = $rows * $cols-in;
        pdf_buf_pack_64_W($in-buf, $out, $out-len, $W-buf, +$W);
        $out;
    }

    multi sub pack(Blob $buf, 8) { $buf }
    multi sub pack($nums, 8) { blob8.new: $nums }
    multi sub unpack(Blob $b, 8) { $b }
    multi sub unpack($nums, 8) { blob8.new: $nums }
}
