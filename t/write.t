use v6;
use Test;
plan 26;

use Lib::PDF::Writer;

given Lib::PDF::Writer {
     is .write-bool(0), "false";
     is .write-bool(1), "true";
     is .write-bool(0, Blob[uint8].allocate(4)), "fal";
     is .write-real(pi), "3.14159";
     is .write-real(42), "42";
     is .write-real(0), "0";
     is .write-real(-42), "-42";
     is .write-real(.42), "0.42";
     is .write-real(42e5), "4200000";
     is .write-int(42e5), "4200000";
     is .write-int(42), "42";
     is .write-int(-42), "-42";
     is .write-int(-0), "0";
     is .write-literal("Hi"), '(Hi)';
     is .write-literal("A\rB\nC\fD\bE\t"), '(A\rB\nC\fD\bE\t)';
     is .write-literal(""), '()';
     is .write-literal("\\ % # / ( ) < > [ ] \{ \}"), '(\\\\ \% \# / \( \) \< \> \[ \] \{ \})';
     is .write-literal("\x0E\x0\xA0"),'(\\016\000\\240)'; 
     is .write-hex-string("snoopy"),'<736e6f6f7079>';

     enum <free inuse>;

     my uint32 @xref = [
        free, 0, 65535, 0,
        inuse, 1, 0, 42,
        inuse, 2, 0, 69,
        inuse, 4, 0, 100,
     ];
     is-deeply .write-xref-array(@xref).lines, (
         'xref',
         '0 3',
         '0000000000 65535 f ',
         '0000000042 00000 n ',
         '0000000069 00000 n ',
         '4 1',
         '0000000100 00000 n '
     );
     is .write-name('Hi'), '/Hi';
     is .write-name('Hi#there'), '/Hi##there';
     is .write-name("A\fB\xE C"), '/A#0cB#0e#20C';
     is .write-name("Zsófia"), '/Zs#c3#b3fia';
     is .write-name("\x10aa"), '/#e1#82#aa';
     is .write-name("\x10aaaa"), '/#f4#8a#aa#aa';

}
