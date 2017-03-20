use v6;
use Test;
plan 1;

use Lib::PDF::Buf;
use NativeCall;

my @result;
my $bytes = buf8.new(10, 20, 30, 40, 50, 60);

is-deeply (@result = Lib::PDF::Buf.resample($bytes,  8, 4)), [0, 10, 1, 4, 1, 14, 2, 8, 3, 2, 3, 12], '4 bit resample';
