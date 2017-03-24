use v6;

class Lib::PDF::Buf {

    use NativeCall;
    use Lib::PDF :libpdf;

    sub pdf_buf_unpack_1(Blob, Blob, size_t)  is native(&libpdf) { * }
    sub pdf_buf_unpack_2(Blob, Blob, size_t)  is native(&libpdf) { * }
    sub pdf_buf_unpack_4(Blob, Blob, size_t)  is native(&libpdf) { * }
    sub pdf_buf_unpack_16(Blob, Blob, size_t) is native(&libpdf) { * }
    sub pdf_buf_unpack_24(Blob, Blob, size_t) is native(&libpdf) { * }
    sub pdf_buf_unpack_32(Blob, Blob, size_t) is native(&libpdf) { * }
    sub pdf_buf_unpack_32_W(Blob, Blob, size_t, Blob, size_t) is native(&libpdf) { * }

    sub pdf_buf_pack_1(Blob, Blob, size_t)  is native(&libpdf) { * }
    sub pdf_buf_pack_2(Blob, Blob, size_t)  is native(&libpdf) { * }
    sub pdf_buf_pack_4(Blob, Blob, size_t)  is native(&libpdf) { * }
    sub pdf_buf_pack_16(Blob, Blob, size_t) is native(&libpdf) { * }
    sub pdf_buf_pack_24(Blob, Blob, size_t) is native(&libpdf) { * }
    sub pdf_buf_pack_32(Blob, Blob, size_t) is native(&libpdf) { * }
    sub pdf_buf_pack_32_W(Blob, Blob, size_t, Blob, size_t) is native(&libpdf) { * }

    my subset PackingSize where 1|2|4|8|16|24|32;
    sub alloc($type, $len) {
        my $buf = Buf[$type].new;
        $buf[$len-1] = 0 if $len;
        $buf;
    }
    sub container(PackingSize $n) {
        %( 1 => uint8, 2 => uint8, 4 => uint8, 8 => uint8, 16 => uint16, 24 => uint32, 32 => uint32){$n};
    }
    
    sub do-packing($n, $m, $in is copy, &pack) {
        my uint32 $in-len = $in.elems;
        die "incomplete scan-line: $in-len * $n not divisable by $m"
            unless ($in-len * $n) %% $m;
        $in = Buf[container($n)].new($in)
            unless $in.isa(Blob);
        my $out-size = max($m, 8);
        my $out-type = container($m);
        my $out := alloc($out-type, ($in-len * $n) div $m);
        &pack($in, $out, $in-len);
        $out.list;
    }
    multi method resample($nums!, PackingSize $n!, PackingSize $m!)  {
        when $n == $m { $nums }
        when $n == 8 {
            my &packer = %(
                1 => &pdf_buf_unpack_1,
                2 => &pdf_buf_unpack_2,
                4 => &pdf_buf_unpack_4,
                16 => &pdf_buf_unpack_16,
                24 => &pdf_buf_unpack_24,
                32 => &pdf_buf_unpack_32,
            ){$m};
            do-packing($n, $m, $nums, &packer);
        }
        when $m == 8 {
            my &packer = %(
                1 => &pdf_buf_pack_1,
                2 => &pdf_buf_pack_2,
                4 => &pdf_buf_pack_4,
                16 => &pdf_buf_pack_16,
                24 => &pdf_buf_pack_24,
                32 => &pdf_buf_pack_32,
            ){$n};
            do-packing($n, $m, $nums, &packer);
        }
    }

    #| variable resampling, e.g. to decode/encode:
    #|   obj 123 0 << /Type /XRef /W [1, 3, 1]
    multi method resample( $in!, 8, Array $W!)  {
        my uint32 $in-len = +$in;
        my buf8 $in-buf .= new($in);
        my buf8 $W-buf .= new($W);
        my $out-buf := buf32.new;
        my $out-len = ($in-len * +$W) div $W.sum;
        $out-buf[$out-len - 1] = 0
           if $out-len;
        pdf_buf_unpack_32_W($in-buf, $out-buf, $in-len, $W-buf, +$W);
	my uint32 @shaped[$out-len div +$W;+$W] Z= $out-buf;
        @shaped;
    }

    multi method resample( $in, Array $W!, 8)  {
        my $width = $W.sum;
        my $in-len = $in.elems;
	my $out = alloc(uint8, $in-len * $width);
        my buf32 $in-buf .= new($in);
        my buf8 $W-buf .= new($W);
        pdf_buf_pack_32_W($in-buf, $out, $in-len, $W-buf, +$W);
        $out.list;
    }
}
