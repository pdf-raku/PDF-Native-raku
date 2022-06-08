use v6;

#| General packing and unpacking of binary words
module PDF::Native::Buf {

=begin pod

Handles the packing and unpacking of multi-byte quantities as network words. Such as `/BitsPerComponent` in `/Filter` `/DecodeParms`.

Also handles variable byte packing and unpacking. As seen in the `/W` parameter to XRef streams, and a few other places.

```
    # pack two 4-byte words into an 8 byte buffer
    use PDF::Native::Buf :pack;
    my blob32 $words .= new(660510, 2634300);
    my blob8 $bytes = pack($words, 24);

    # pack triples as 1 byte, 2 bytes, 1 byte
    my uint32 @in[4;3] = [1, 16, 0], [1, 741, 0], [1, 1030, 0], [1, 1446, 0];
    my @W = packing-widths(@in, 3);    # [1, 2, 0];
    $bytes = pack(@in, @W);
```

=head2 Subroutines
=end pod
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
    sub pdf_buf_unpack_W_64(Blob, Blob, size_t, Blob, size_t) is native(libpdf) { * }

    sub pdf_buf_pack_1(Blob, Blob, size_t)  is native(libpdf) { * }
    sub pdf_buf_pack_2(Blob, Blob, size_t)  is native(libpdf) { * }
    sub pdf_buf_pack_4(Blob, Blob, size_t)  is native(libpdf) { * }
    sub pdf_buf_pack_16(Blob, Blob, size_t) is native(libpdf) { * }
    sub pdf_buf_pack_24(Blob, Blob, size_t) is native(libpdf) { * }
    sub pdf_buf_pack_32(Blob, Blob, size_t) is native(libpdf) { * }
    sub pdf_buf_pack_compute_W_64(Blob, size_t, Blob, size_t) is native(libpdf) { * }
    sub pdf_buf_pack_W_64(Blob, Blob, size_t, Blob, size_t) is native(libpdf) { * }

    sub pdf_buf_unpack_xref_stream(Blob, Blob, size_t, Blob, size_t --> size_t) is native(libpdf) { * }
    sub pdf_buf_pack_xref_stream(Blob, Blob, size_t, Blob, size_t is rw --> uint32) is native(libpdf) { * }

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
    #| unpack bytes to an array of words, each of a given size
    multi sub unpack($nums!, PackingSize $n! --> Blob)  {
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

    #| upack bytes to an array words, each of a given size
    multi sub pack($nums!, PackingSize $n! --> Blob)  {
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

    #| variable unpacking of records, e.g. to decode/encode:
    #|   obj 123 0 << /Type /XRef /W [1, 3, 1]
    multi sub unpack( $in!, Array $W! --> array)  {
        my size_t $in-len = +$in;
        my blob8 $in-buf = $in ~~ blob8 ?? $in !! blob8.new($in);
        my blob8 $W-buf .= new($W);
        my size_t $out-len = ($in-len * +$W) div $W.sum;
        my $out-buf := buf64.allocate($out-len);
        pdf_buf_unpack_W_64($in-buf, $out-buf, $in-len, $W-buf, +$W);
	my uint64 @shaped[$out-len div +$W;+$W] Z= $out-buf;
        @shaped;
    }

    #| variable packing of records
    multi sub pack( $in, Array $W! --> Blob)  {
        my $rows = $in.elems;
        my $cols-in = +$W;
        my $cols-out = $W.sum;
	my $out = buf8.allocate($rows * $cols-out);
        my blob64 $in-buf = $in ~~ blob64 ?? $in !! blob64.new($in);
        my blob8 $W-buf .= new($W);
        my size_t $out-len = $rows * $cols-in;
        pdf_buf_pack_W_64($in-buf, $out, $out-len, $W-buf, +$W);
        $out;
    }

    our sub packing-widths($in, UInt:D $n) is export(:pack) {
        my buf8 $W-buf .= allocate($n);
        my blob64 $in-buf = $in ~~ blob64 ?? $in !! blob64.new($in);
        pdf_buf_pack_compute_W_64($in-buf, +$in-buf, $W-buf, $n);
        $W-buf.List;
    }

    multi sub pack(Blob $buf, 8) { $buf }
    multi sub pack($nums, 8) { blob8.new: $nums }
    multi sub unpack(Blob $b, 8) { $b }
    multi sub unpack($nums, 8) { blob8.new: $nums }

    our sub unpack-xref-stream($in, $index) is export(:pack-xref-stream) {
        my blob64 $in-buf = $in ~~ blob64 ?? $in !! blob64.new: $in;
        my blob32 $index-buf = $index ~~ blob32 ?? $index !! blob32.new($index);
        warn "input XRef input size not a multiple of 3"
            unless $in-buf.elems %% 3;
        my $rows = $in-buf.elems div 3;
        my buf64 $out-buf := buf64.allocate(4 * $rows);
        my $n = pdf_buf_unpack_xref_stream($in-buf, $out-buf, $rows, $index-buf, +$index-buf);
        given $n <=> $rows {
            when Less { die $n; die "insufficent data in XRef stream" }
            when More { die "XRef stream content overflow" }
        }
        my uint64 @out[$rows;4] Z= $out-buf;
        @out;
    }

    our sub pack-xref-stream($in, @out, @index) is export(:pack-xref-stream) {
        my blob64 $in-buf = $in ~~ blob64 ?? $in !! blob64.new: $in;
        warn "input XRef input size not a multiple of 4"
            unless $in-buf.elems %% 4;
        my $rows = $in-buf.elems div 4;
        my buf64 $out-buf := buf64.allocate(3 * $rows);
        my buf32 $index-buf := buf32.allocate(2 * $rows);
        my size_t $index-len;
        my $size = pdf_buf_pack_xref_stream($in-buf, $out-buf, $rows, $index-buf, $index-len);
        @out Z= $out-buf;
        @index = $index-buf.subbuf(0, $index-len).list;
        $size;
    }

}
