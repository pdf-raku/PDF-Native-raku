[[Raku PDF Project]](https://pdf-raku.github.io)
 / [[PDF-Native Module]](https://pdf-raku.github.io/PDF-Native-raku)
 / [PDF::Native](https://pdf-raku.github.io/PDF-Native-raku/PDF/Native)
 :: [Cos](https://pdf-raku.github.io/PDF-Native-raku/PDF/Native/Cos)

Description
-----------

This under development is a set of objects for the native construction and serialization of COS (PDF) objects.

In particular, CosIndObj is designed as drop in replacement for [PDF::IO::IndObj](https://pdf-raku.github.io/PDF-raku).

Todo
----

- `ast()` method on indirect objects - a native object parser

class submethod DESTROY (PDF::Native::Cos::CosNull $:: *%_) { #`(Submethod|4680485064488) ... }
-----------------------------------------------------------------------------------------------

Only needed on tree/fragment root nodes.

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

