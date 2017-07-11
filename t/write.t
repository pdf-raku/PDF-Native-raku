use v6;
use Test;
plan 12;

use Lib::PDF::Writer;

given Lib::PDF::Writer {
     is .write-bool(0), "false";
     is .write-bool(1), "true";
     is .write-bool(0, Blob[uint8].allocate(4)), "fal";
     is .write-real(42), "42";
     is .write-real(0), "0";
     is .write-real(-42), "-42";
     is .write-real(.42), "0.42";
     is .write-real(42e5), "4200000";
     is .write-int(42e5), "4200000";
     is .write-int(42), "42";
     is .write-int(-42), "-42";
     is .write-int(-0), "0";
}
