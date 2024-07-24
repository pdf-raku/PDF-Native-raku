[[Raku PDF Project]](https://pdf-raku.github.io)
 / [[PDF-Native Module]](https://pdf-raku.github.io/PDF-Native-raku)
 / [PDF::Native](https://pdf-raku.github.io/PDF-Native-raku/PDF/Native)
 :: [COS](https://pdf-raku.github.io/PDF-Native-raku/PDF/Native/COS)

Synopsis
--------

```raku
use PDF::Native::COS;

my $stream = q:to<--END-->;
    << /Length 45 >> stream
    BT
    /F1 24 Tf
    100 250 Td (Hello, world!) Tj
    ET
    endstream
    --END--

my COSIndObj $ind-obj .= parse: "123 4 obj\n{$stream}endobj";
say $ind-obj.write;    # serialize to PDF
say $ind-obj.ast.raku; # mimic PDF::Grammar::COS.parse( $_ , :rule<ind-obj>);
my COSInt $val .= parse: "42"; # simple object parse
say $ind-obj.value.dict<Length>.cmp($val); # derefencing and comparision
```

Description
-----------

This is under development as a set of objects for the native construction and serialization of COS (PDF) objects.

It utilized by [PDF::IO::Reader](https://pdf-raku.github.io/PDF-raku) and [PDF::IO::Writer](https://pdf-raku.github.io/PDF-raku) to provide faster reading and writing of larger PDF files.

class PDF::Native::COS::COSNode
-------------------------------

Generic COS objects

### multi method parse

```raku
multi method parse(
    Str:D $str where { ... }
) returns PDF::Native::COS::COSNode
```

Parse a COS object

class PDF::Native::COS::COSRef
------------------------------

Indirect object reference

class PDF::Native::COS::COSCryptCtx
-----------------------------------

An encryption context

class PDF::Native::COS::COSArray
--------------------------------

Array object

class PDF::Native::COS::COSName
-------------------------------

Name object

class PDF::Native::COS::COSDict
-------------------------------

Dictionary (hash) object

class PDF::Native::COS::COSStream
---------------------------------

Stream object

class PDF::Native::COS::COSIndObj
---------------------------------

Indirect object

class PDF::Native::COS::COSBool
-------------------------------

Boolean object

class PDF::Native::COS::COSInt
------------------------------

Integer object

class PDF::Native::COS::COSReal
-------------------------------

Real object

class PDF::Native::COS::COSHexString
------------------------------------

Hex string object

class PDF::Native::COS::COSNull
-------------------------------

Null object

class PDF::Native::COS::COSOp
-----------------------------

Graphics Operator

class PDF::Native::COS::COSContent
----------------------------------

Graphics content stream

class PDF::Native::COS::COSInlineImage
--------------------------------------

Inline Image

