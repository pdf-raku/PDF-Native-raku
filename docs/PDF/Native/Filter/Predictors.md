[[Raku PDF Project]](https://pdf-raku.github.io)
 / [[PDF-Class Module]](https://pdf-raku.github.io/PDF-Class-raku)

class PDF::Native::Filter::Predictors
-------------------------------------

Predictor stage transcoding

These functions implements the predictor stage of TIFF [TIFF Predictors](http://www.fileformat.info/format/tiff/corion-lzw.htm) and PNG [PNG Predictors](https://www.w3.org/TR/PNG-Filters.html) decoding and encoding.

```raku
use PDF::Native::Filter::Predictors;
# PNG samples. First bit on each row, is an indicator in the range 0 .. 4
my $Predictor = PDF::Native::Filter::Predictors::PNG;
my $Columns = 4;
my $encoded = blob8.new: [
    2,  0x1, 0x0, 0x10, 0x0,
    2,  0x0, 0x2, 0xcd, 0x0,
    2,  0x0, 0x1, 0x51, 0x0,
    1,  0x0, 0x1, 0x70, 0x0,
    3,  0x0, 0x5, 0x7a, 0x0,
    0,  0x1, 0x2, 0x3,  0x4,
];

my blob8 $decoded = PDF::Native::Filter::Predictors.decode(
                                    $encoded,
                                    :$Columns,
                                    :$Predictor, );
```

Methods
-------

### multi method encode

```raku
multi method encode(
    $buf where { ... },
    Int :$Predictor! where { ... },
    Int :$Columns where { ... } = 1,
    Int :$Colors where { ... } = 1,
    Int :$BitsPerComponent where { ... } = 8
) returns Blob
```

Encode predictors

### multi method decode

```raku
multi method decode(
    $buf where { ... },
    Int :$Predictor! where { ... },
    Int :$Columns where { ... } = 1,
    Int :$Colors where { ... } = 1,
    Int :$BitsPerComponent where { ... } = 8
) returns Mu
```

Decode predictors

