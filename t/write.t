use v6;
use Test;
plan 35;

use PDF::Native::Writer;

given PDF::Native::Writer {
     is .write-bool(False), "false";
     is .write-bool(True), "true";
     is .write-bool(False, Blob[uint8].allocate(4)), "fals";
     is .write-real(pi), "3.14159";
     is .write-real(0.074877), "0.07488";
     is .write-real(42), "42";
     is .write-real(0), "0";
     is .write-real(-42), "-42";
     is .write-real(.42), "0.42";
     is .write-real(42e5.Int), "4200000";
     is .write-int(42e5.Int), "4200000";
     is .write-int(42), "42";
     is .write-int(-42), "-42";
     is .write-int(-0), "0";
     is .write-literal("Hi"), '(Hi)';
     is .write-literal("A\rB\nC\fD\bE\t"), '(A\rB\nC\fD\bE\t)';
     is .write-literal(""), '()';
     is .write-literal("\\ % # / ( ) < > [ ] \{ \}"), '(\\\\ % # / \( \) < > [ ] { })';
     is .write-literal("\x0E\x0\xA0"),"(\o016\o0\o240)";
     is .write-literal("a(b", Blob[uint8].allocate(4)), "(a)";
     is .write-literal("a(b", Blob[uint8].allocate(5)), "(a\\()";
     is .write-literal("a(b", Blob[uint8].allocate(6)), "(a\\(b)";
     is .write-hex-string("snoopy"),'<736e6f6f7079>';
     is .write-hex-string("snoopy", Blob[uint8].allocate(4)),'<73>';
     is .write-hex-string("snoopy", Blob[uint8].allocate(5)),'<73>';
     is .write-hex-string("snoopy", Blob[uint8].allocate(6)),'<736e>';

     enum <free inuse>;

     my uint64 @xref[4;3] = (
        [0, 65535, free],
        [42, 0, inuse],
        [69, 0, inuse],
        [100, 2, inuse],
     );
     is-deeply .write-entries(@xref).lines, (
         '0000000000 65535 f ',
         '0000000042 00000 n ',
         '0000000069 00000 n ',
         '0000000100 00002 n '
     );
     is .write-name('Hi'), '/Hi';
     is .write-name('Hi#there'), '/Hi##there';
     is .write-name("A\fB\xE C"), '/A#0cB#0e#20C';
     is .write-name("Zsófia"), '/Zs#c3#b3fia';
     is .write-name("\x10aa"), '/#e1#82#aa';
     is .write-name("\x10aaaa"), '/#f4#8a#aa#aa';
     is .write-name("42(approx)"), '/42#28approx#29';
     is .write-null(), 'null';
}
