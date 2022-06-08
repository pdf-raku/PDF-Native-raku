[[Raku PDF Project]](https://pdf-raku.github.io)
 / [[PDF-Class Module]](https://pdf-raku.github.io/PDF-Class-raku)

class PDF::Native::Filter::Predictors
-------------------------------------

Predictor stage transcoding

These functions implements the predictor stage of TIFF [TIFF Predictors](http://www.fileformat.info/format/tiff/corion-lzw.htm) and PNG [PNG Predictors](https://www.w3.org/TR/PNG-Filters.html) decoding and encoding.

``` use PDF::Native::Filter::Predictors; # PNG samples. First bit on each row, is an indicator in the range 0 .. 4 my $Predictor = PDF::Native::Filter::Predictors::PNG; my $Columns = 4; my $encoded = blob8.new: [ 2, 0x1, 0x0, 0x10, 0x0, 2, 0x0, 0x2, 0xcd, 0x0, 2, 0x0, 0x1, 0x51, 0x0, 1, 0x0, 0x1, 0x70, 0x0, 3, 0x0, 0x5, 0x7a, 0x0, 0, 0x1, 0x2, 0x3, 0x4, ];

    my blob8 $decoded = PDF::Native::Filter::Predictors.decode(
                                        $encoded,
                                        :$Columns,
                                        :$Predictor, );

```

class Int :$Columns where { ... } = 1
-------------------------------------

predictor function

class Int :$Colors where { ... } = 1
------------------------------------

number of samples per row

class Int :$BitsPerComponent where { ... } = 8
----------------------------------------------

number of colors per sample

### multi method encode

```raku
multi method encode(
    $buf is copy where { ... },
    Int :$Predictor! where { ... },
    Int :$Columns where { ... } = 1,
    Int :$Colors where { ... } = 1,
    Int :$BitsPerComponent where { ... } = 8
) returns Mu
```

number of bits per color

class Int :$Columns where { ... } = 1
-------------------------------------

predictor function

class Int :$Colors where { ... } = 1
------------------------------------

number of samples per row

class Int :$BitsPerComponent where { ... } = 8
----------------------------------------------

number of colors per sample

class Int :$Columns where { ... } = 1
-------------------------------------

predictor function

class Int :$Colors where { ... } = 1
------------------------------------

number of samples per row

class Int :$BitsPerComponent where { ... } = 8
----------------------------------------------

number of colors per sample

### multi method decode

```raku
multi method decode(
    Blob $buf,
    Int :$Predictor! where { ... },
    Int :$Columns where { ... } = 1,
    Int :$Colors where { ... } = 1,
    Int :$BitsPerComponent where { ... } = 8
) returns Mu
```

number of bits per color

class Int :$Predictor! where { ... }
------------------------------------

input stream

class Int :$Columns where { ... } = 1
-------------------------------------

predictor function

class Int :$Colors where { ... } = 1
------------------------------------

number of samples per row

class Int :$BitsPerComponent where { ... } = 8
----------------------------------------------

number of colors per sample

