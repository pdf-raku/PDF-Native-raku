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

    my subset PackingSize where 4|8|16|32;
    sub alloc($type, $len) {
        my $buf = Buf[$type].new;
        $buf[$len-1] = 0 if $len;
        $buf;
    }
    sub container(PackingSize $n) {
        %( 4 => uint8, 8 => uint8, 16 => uint16, 32 => uint32){$n};
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
            my &packer = %( 4 => &pdf_buf_pack_8_4, 16 => &pdf_buf_pack_8_16, 32 => &pdf_buf_pack_8_32 ){$m};
            do-packing($n, $m, $nums, &packer);
        }
        when $m == 8 {
            my &packer = %( 4 => &pdf_buf_pack_4_8, 16 => &pdf_buf_pack_16_8, 32 => &pdf_buf_pack_32_8 ){$n};
            do-packing($n, $m, $nums, &packer);
        }
    }

    #| variable resampling, e.g. to decode/encode:
    #|   obj 123 0 << /Type /XRef /W [1, 3, 1]
    multi method resample( $nums!, 8, Array $W!)  {
        my uint $j = 0;
        my @samples;
        while $j < +$nums {
            my @sample = $W.keys.map: -> $i {
                my uint $s = 0;
                for 1 .. $W[$i] {
                    $s *= 256;
                    $s += $nums[$j++];
                }
                $s;
            }
            @samples.push: @sample;
        }
	@samples;
    }

    multi method resample( $num-sets, Array $W!, 8)  {
	my uint8 @sample;
         for $num-sets.list -> Array $nums {
            my uint $i = 0;
            for $nums.list -> uint $num is copy {
                my uint8 @bytes;
                for 1 .. $W[$i++] {
                    @bytes.unshift: $num;
                    $num div= 256;
                }
                @sample.append: @bytes;
            }
        }
	[flat @sample];
    }
}
