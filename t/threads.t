use Test;
use PDF::Native::Writer;

INIT my \MAX_THREADS = %*ENV<MAX_THREADS> || 10;
INIT my \MAX_LOOP = %*ENV<MAX_LOOP> || 500;

sub blat(&r, :$n = MAX_THREADS) {
    (^$n).race(:batch(1)).map(&r);
}

subtest 'writer', {
    for 1..MAX_LOOP {
        blat {

            PDF::Native::Writer.write-literal: "abcdefg";
            PDF::Native::Writer.write-hex-string: "xyz123456";
        
            PDF::Native::Writer.write-int: $_ for 1..10;
            PDF::Native::Writer.write-bool: True;
            PDF::Native::Writer.write-name: "Rakudo";
            PDF::Native::Writer.write-real: pi;
        }
    }
}


done-testing;
