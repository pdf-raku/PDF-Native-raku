use Lib::PDF::Encode :base64;

use Test;

is base64-encode("".encode).decode, "", "encode 0 bytes";
is base64-encode("a".encode).decode, "YQ==", "encode 1 byte";
is base64-encode("ab".encode).decode, "YWI=", "encode 2 bytes";
is base64-encode("abc".encode).decode, "YWJj", "encode 3 bytes";
is base64-encode("abcd".encode).decode, "YWJjZA==", "encode 4 bytes";

my $text = q:to<-END->.lines.join: ' ';
Man is distinguished, not only by his reason, but by this singular passion from
other animals, which is a lust of the mind, that by a perseverance of delight
in the continued and indefatigable generation of knowledge, exceeds the short
vehemence of any carnal pleasure.
-END-

my $base64 = q:to<-END->.lines.join;
TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz
IHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg
dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu
dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo
ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=
-END-

is base64-encode($text.encode).decode, $base64, "longer encoding";

my $trunc-out = buf8.allocate(10);
is base64-encode($text.encode, $trunc-out).decode, $base64.substr(0,10), "truncated encoding";

is base64-decode("YWJj".encode).decode, "abc", "decode to 3 bytes";
is base64-decode("YWI=".encode).decode, "ab", "decode to 2 bytes";
is base64-decode("YQ==".encode).decode, "a", "decode to 1 bytes";
is base64-decode("".encode).decode, "", "decode to 1 bytes";
is base64-decode("YWJjZA==".encode).decode, "abcd", "decode to 4 bytes";
is base64-decode("YWJjZA".encode).decode, "abcd", "decode no padding";
is base64-decode(" Y\nWJj ZA == ".encode).decode, "abcd", "decode whitespace";
dies-ok {base64-decode("YW(=".encode).decode}, "decode invalid input";

is base64-decode($base64.encode).decode, $text, "longer decoding";
is base64-decode($base64.encode, $trunc-out).decode, $text.substr(0,10), "truncted decoding";

done-testing;
