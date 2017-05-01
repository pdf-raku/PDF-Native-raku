use Lib::PDF::Encode :base64;

use Test;

is base64-encode("".encode).decode, "", "0 bytes";
is base64-encode("a".encode).decode, "YQ==", "1 byte";
is base64-encode("ab".encode).decode, "YWI=", "2 bytes";
is base64-encode("abc".encode).decode, "YWJj", "3 bytes";
is base64-encode("abcd".encode).decode, "YWJjZA==", "4 bytes";

my $quote = q:to<-END->.lines.join: ' ';
Man is distinguished, not only by his reason, but by this singular passion from
other animals, which is a lust of the mind, that by a perseverance of delight
in the continued and indefatigable generation of knowledge, exceeds the short
vehemence of any carnal pleasure.
-END-

my $expected = q:to<-END->.lines.join;
TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz
IHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg
dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu
dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo
ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=
-END-

is base64-encode($quote.encode).decode, $expected, "longer encoding";

my $trunc-out = buf8.allocate(10);
is base64-encode($quote.encode, $trunc-out).decode, $expected.substr(0,10), "truncated encoding";

done-testing;
