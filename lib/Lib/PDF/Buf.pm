use v6;

class Lib::PDF::Buf {

    use NativeCall;
    use Lib::PDF :libpdf;

    sub pdf_buf_pack_8_4(Blob, Blob, size_t)  is native(&libpdf) { * }
    sub pdf_buf_pack_8_16(Blob, Blob, size_t) is native(&libpdf) { * }
    sub pdf_buf_pack_8_32(Blob, Blob, size_t) is native(&libpdf) { * }
    sub pdf_buf_pack_4_8(Blob, Blob, size_t)  is native(&libpdf) { * }
    sub pdf_buf_pack_16_8(Blob, Blob, size_t) is native(&libpdf) { * }
    sub pdf_buf_pack_32_8(Blob, Blob, size_t) is native(&libpdf) { * }
##    sub pdf_buf_pack_32_8_W(Blob, Blob, size_t, Blob, size_t) is native(&libpdf) { * }
##    sub pdf_buf_pack_8_32_W(Blob, Blob, size_t, Blob, size_t) is native(&libpdf) { * }

    my subset PackingSize where 4|8|16|24|32;
    sub alloc($type, $len) {
        my $buf = Buf[$type].new;
        $buf[$len-1] = 0 if $len;
        $buf;
    }
    sub container(PackingSize $n) {
        %( 4 => uint8, 8 => uint8, 16 => uint16, 24 => uint32, 32 => uint32){$n};
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
            my &packer = %( 4 => &pdf_buf_pack_8_4, 16 => &pdf_buf_pack_8_16, 24|32 => &pdf_buf_pack_8_32 ){$m};
            do-packing($n, $m, $nums, &packer);
        }
        when $m == 8 {
            my &packer = %( 4 => &pdf_buf_pack_4_8, 16 => &pdf_buf_pack_16_8, 24|32 => &pdf_buf_pack_32_8 ){$n};
            do-packing($n, $m, $nums, &packer);
        }
    }

    #| variable resampling, e.g. to decode/encode:
    #|   obj 123 0 << /Type /XRef /W [1, 3, 1]
    multi method resample( $nums!, 8, Array $W!)  {
        my uint $j = 0;
        my uint $k = 0;
        my uint32 @idx;
        @idx[+$nums div $W.sum] = 0
            if +$nums;
        while $j < +$nums {
            for $W.keys -> $i {
                my uint32 $s = 0;
                for 1 .. $W[$i] {
                    $s *= 256;
                    $s += $nums[$j++];
                }
                @idx[$k++] = $s;
            }
        }
	@idx.rotor(+$W);
    }

    multi method resample( $num-sets, Array $W!, 8)  {
	my uint8 @sample;
        @sample[$W.sum * +$num-sets - 1] = 0
            if +$num-sets;
        my uint32 $i = -1;
        for $num-sets.list -> List $nums {
            my uint $k = 0;
            for $nums.list -> uint $num is copy {
                my uint $n = +$W[$k++];
                $i += $n;
                loop (my $j = 0; $j < $n; $j++) {
                    @sample[$i - $j] = $num;
                    $num div= 256;
                }
            }
         }
	 @sample;
    }
}
