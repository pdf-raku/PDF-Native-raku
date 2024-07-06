[[Raku PDF Project]](https://pdf-raku.github.io)
 / [[PDF-Native Module]](https://pdf-raku.github.io/PDF-Native-raku)
 / [PDF::Native](https://pdf-raku.github.io/PDF-Native-raku/PDF/Native)
 :: [Cos](https://pdf-raku.github.io/PDF-Native-raku/PDF/Native/Cos)

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

my CosIndObj $ind-obj .= parse: "123 4 obj\n{$stream}endobj";
say $ind-obj.Str;      # serialize
say $ind-obj.ast.raku; # mimic PDF::Grammar::COS.parse( $_ , :rule<ind-obj>);
my CosInt $val .= parse: "42"; # simple object parse
say $ind-obj.value.dict<Length>.cmp($val); # derefencing and comparision
```

Description
-----------

This under development as a set of objects for the native construction and serialization of COS (PDF) objects.

In particular, CosIndObj is intended as drop in replacement for [PDF::IO::IndObj](https://pdf-raku.github.io/PDF-raku).

class PDF::Native::Cos::CosNode
-------------------------------

Generic Cos objects

class PDF::Native::Cos::CosRef
------------------------------

Indirect object reference

class PDF::Native::Cos::CosCryptCtx
-----------------------------------

An encryption context

class PDF::Native::Cos::CosIndObj
---------------------------------

Indirect object

### method cos_ind_obj_new

```raku
method cos_ind_obj_new(
    uint64 $,
    uint32 $,
    PDF::Native::Cos::CosNode $
) returns PDF::Native::Cos::CosIndObj
```

Indirect objects the top of the tree and always fragments

class PDF::Native::Cos::CosArray
--------------------------------

Array object

class PDF::Native::Cos::CosName
-------------------------------

Name object

class PDF::Native::Cos::CosDict
-------------------------------

Dictionary (hash) object

class PDF::Native::Cos::CosStream
---------------------------------

Stream object

class PDF::Native::Cos::CosBool
-------------------------------

Boolean object

class PDF::Native::Cos::CosInt
------------------------------

Integer object

class PDF::Native::Cos::CosReal
-------------------------------

Real object

class PDF::Native::Cos::CosHexString
------------------------------------

Hex string object

class PDF::Native::Cos::CosNull
-------------------------------

Null object

### method AT-POS

```raku
method AT-POS(
    UInt:D(Any):D $idx
) returns PDF::Native::Cos::CosNode
```

Indirect objects the top of the tree and always fragments

### method AT-POS

```raku
method AT-POS(
    UInt:D(Any):D $idx
) returns PDF::Native::Cos::CosNode
```

Indirect objects the top of the tree and always fragments

