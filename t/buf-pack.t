use v6;
use Test;
plan 21;

use PDF::Native::Buf :pack;
use NativeCall;

my $buf;
my @bytes = (10, 20, 30, 40, 50, 60, 70, 80);
my $bytes := blob8.new(@bytes);

is-deeply ($buf = unpack($bytes,  4)), blob8.new(0,10, 1,4, 1,14, 2,8, 3,2, 3,12, 4,6, 5,0), '4 bit unpack';
is-deeply pack($buf, 4), $bytes, 'pack round-trip: 8 => 4 => 8';

is-deeply ($buf = unpack($bytes,  2)), blob8.new(0,0,2,2, 0,1,1,0, 0,1,3,2, 0,2,2,0, 0,3,0,2, 0,3,3,0, 1,0,1,2, 1,1,0,0), '2 bit unpack';
is-deeply pack($buf, 2), $bytes, 'pack round-trip: 8 => 2 => 8';

is-deeply ($buf = unpack($bytes, 1)), blob8.new(0,0,0,0,1,0,1,0, 0,0,0,1,0,1,0,0, 0,0,0,1,1,1,1,0, 0,0,1,0,1,0,0,0,
                                                0,0,1,1,0,0,1,0, 0,0,1,1,1,1,0,0, 0,1,0,0,0,1,1,0, 0,1,0,1,0,0,0,0), '1 bit unpack';
is-deeply pack($buf, 1), $bytes, 'pack round-trip: 8 => 1 => 8';

is-deeply ($buf = unpack($bytes, 16)), blob16.new(2580, 7720, 12860, 18000), '16 bit unpack';
is-deeply pack($buf, 16), $bytes, 'pack round-trip: 16 => 8 => 16';

is-deeply ($buf = unpack($bytes[0..5], 24)), blob32.new(660510, 2634300), '16 bit unpack';
is-deeply pack($buf, 24), blob8.new(@bytes[0..5]), 'pack round-trip: 16 => 8 => 16';

is-deeply pack([1415192289,], 32), blob8.new(84, 90, 30, 225), '32 => 8 pack';
is-deeply pack([2 ** 32 - 1415192289 - 1,], 32), blob8.new(255-84, 255-90, 255-30, 255-225), '32 => 8 pack (twos comp)';

my uint64 @in1[1;3] = ([10, 1318440, 12860],);
my $idx;
my @W = [1, 3, 2];
is-deeply ($idx = unpack($bytes, @W)).values, @in1.values, '8 => [1, 3, 2] unpack';
is-deeply packing-widths($idx, 3), (1, 3, 2), 'packing widths';
is-deeply pack($idx, [1, 3, 2]), buf8.new(@bytes[0..5]), '8 => [1, 3, 2] => 8 round-trip';

my uint64 @in[4;3] = [1, 16, 0], [1, 741, 0], [1, 1030, 0], [1, 1446, 0];
@W = [1, 2, 1];
my $out = buf8.new(1, 0,16, 0,  1, 2,229, 0,  1, 4,6, 0,  1, 5,166, 0);

is-deeply pack(@in, @W), $out, '@W[1, 2, 1] pack';
is-deeply unpack($out, @W).list, @in.list, '@W[1, 2, 1] unpack';

@W = packing-widths(@in, 3);
# zero column is zero-width
is-deeply @W, [1,2,0], 'packing widths - zero column';
my $out2 = buf8.new(1, 0,16,  1, 2,229,  1, 4,6,  1, 5,166,);
is-deeply pack(@in, @W), $out2, '@W[1, 2, 0] pack';
is-deeply unpack($out2, @W).list, @in.list, '@W[1, 2, 0] unpack';

subtest 'pack-xref-stream', {
    use PDF::Native::Buf :pack-xref-stream;
    my @index = [42,1, 69,3];
    my uint64 @xref-expected[4;4] = [42, 1, 16, 0], [69, 1, 741, 0], [70, 1, 1030, 0], [71, 1, 1446, 0];
    my @xref := unpack-xref-stream(@in, @index);
    is-deeply @xref, @xref-expected;
    my $size = pack-xref-stream(@xref, my uint64 @out[4;3], my @index2);
    is-deeply @out, @in, 'unpack/pack round-trip';
    is-deeply @index2, @index;
    is-deeply $size, 72;
}

