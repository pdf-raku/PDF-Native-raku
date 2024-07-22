use Test;
use PDF::Native::Writer;
use PDF::Native::COS;

INIT my \MAX_THREADS = %*ENV<MAX_THREADS> || 10;
INIT my \MAX_LOOP = %*ENV<MAX_LOOP> || 500;

sub blat(&r, :$n = MAX_THREADS) {
    (^$n).race(:batch(1)).map(&r);
}

subtest 'writer', {
    for 1..MAX_LOOP {
        my Str:D @str[MAX_THREADS*6] = blat {
            (PDF::Native::Writer.write-literal("abcdefg"),
             PDF::Native::Writer.write-hex-string("xyz123456"),
             PDF::Native::Writer.write-int(4200),
             PDF::Native::Writer.write-bool(True),
             PDF::Native::Writer.write-name("Rakudo"),
             PDF::Native::Writer.write-real(pi),
             ).Slip
        }
    }
}

subtest 'parser', {
    for 1..MAX_LOOP {
        my COSNode:D @obj[MAX_THREADS*3] = blat {
            (COSContent.parse("BT /F1 24 Tf  100 250 Td (Hello, world!) Tj ET"),
             COSNode.parse("<< /foo [ $_ ] >>"),
             COSIndObj.parse("$_ 0 obj << /foo [ $_ ] /Length 3 >>stream\nabc\nendstream endobj"),
            ).Slip
        }
    }
}


done-testing;
