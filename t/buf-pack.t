use v6;
use Test;
plan 13;

use Lib::PDF::Buf :pack;
use NativeCall;

my $buf;
my @bytes = (10, 20, 30, 40, 50, 60, 70, 80);
my $bytes := buf8.new(@bytes);

is-deeply ($buf =unpack($bytes,  4)), buf8.new(0, 10, 1, 4, 1, 14, 2, 8, 3, 2, 3, 12, 4, 6, 5, 0), '4 bit unpack';
is-deeply pack($buf, 4), $bytes, 'pack round-trip: 8 => 4 => 8';

is-deeply ($buf =unpack($bytes,  2)), buf8.new(0, 0, 2, 2, 0, 1, 1, 0, 0, 1, 3, 2, 0, 2, 2, 0, 0, 3, 0, 2, 0, 3, 3, 0, 1, 0, 1, 2, 1, 1, 0, 0), '2 bit unpack';
is-deeply pack($buf, 2), $bytes, 'pack round-trip: 8 => 2 => 8';

is-deeply ($buf=unpack($bytes, 16)), buf16.new(2580, 7720, 12860, 18000), '16 bit unpack';
is-deeply pack($buf, 16), $bytes, 'pack round-trip: 16 => 8 => 16';

is-deeply ($buf=unpack($bytes[0..5], 24)), buf32.new(660510, 2634300), '16 bit unpack';
is-deeply pack($buf, 24), buf8.new(@bytes[0..5]), 'pack round-trip: 16 => 8 => 16';

is-deeply ($buf=pack([1415192289,], 32)), buf8.new(84, 90, 30, 225), '32 => 8 pack';
is-deeply ($buf=pack([2 ** 32 - 1415192289 - 1,], 32)), buf8.new(255-84, 255-90, 255-30, 255-225), '32 => 8 pack (twos comp)';

my uint32 @in1[1;3] = ([10, 1318440, 12860],);
my $idx;
is-deeply ($idx=unpack($bytes, [1, 3, 2])).values, @in1.values, '8 => [1, 3, 2] unpack';
is-deeply pack($idx, [1, 3, 2]), buf8.new(@bytes[0..5]), '8 => [1, 3, 2] => 8 round-trip';

my uint32 @in[4;3] = ([1, 16, 0], [1, 741, 0], [1, 1030, 0], [1, 1446, 0]);
my $W = [1, 2, 1];
my $out = buf8.new(1, 0, 16, 0,  1, 2, 229, 0,  1, 4, 6, 0,  1, 5, 166, 0);

is-deeply pack(@in, $W), $out, '$W[1, 2, 1] pack';
