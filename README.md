# PDF-Native-p6


 <a href="https://ci.appveyor.com/project/p6-pdf/PDF-Native-p6/branch/master"><img src="https://ci.appveyor.com/api/projects/status/github/p6-pdf/PDF-Native-p6?branch=master&passingText=Windows%20-%20OK&failingText=Windows%20-%20FAIL&pendingText=Windows%20-%20pending&svg=true"></a>

Low level native library of PDF support functions.

The immediate aim is to optionally boost performance in the PDF tool-chain.

Potential areas include encryption, reading, writing, stream and filter functions and PDF::Content image processing and encoding functions.

So far just a few areas are covered, including:

- the PDF::IO::Filter::Predictor `decode` and `encode` functions.
- the widely used PDF::IO::Util `pack` and `unpack` functions.
- writing of cross reference tables
- writing of strings, numerics and xrefs

Just installing this module along with PDF v0.5.7+ should provide some performance
boost for larger PDF files.

### `PDF::Native::Filter::Predictors`

These functions implements the predictor stage of TIFF [1] and PNG [2] decoding and encoding.
```
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

[1] [TIFF Predictors](http://www.fileformat.info/format/tiff/corion-lzw.htm)
[2] [PNG Predictors](https://www.w3.org/TR/PNG-Filters.html)

### `PDF::Native::Buf`

Handles the packing and unpacking of multi-byte quantities as network words. Such as `/BitsPerComponent` in `/Filter` `/DecodeParms`.

Also handles variable byte packing and unpacking. As seen in the `/W` parameter to XRef streams, and a few other places.

```
    # pack two 4-byte words into an 8 byte buffer
    use PDF::Native::Buf :pack;
    my blob32 $words .= new(660510, 2634300);
    my blob8 $bytes = pack($words, 24);

    # pack triples as 1 byte, 2 bytes, 1 byte
    my uint32 @in[4;3] = ([1, 16, 0], [1, 741, 0], [1, 1030, 0], [1, 1446, 0]);
    my $W = [1, 2, 1];
    $bytes = pack(@in, $W);
```

### `PDF::Native::Reader`

Reading of PDF content. Only method so far implemented is `read-xref` for the fast reading of cross reference indices.
```
use PDF::Native::Reader;

given PDF::Native::Reader.new {

     enum <free inuse>;

     my Str $xref = (
         'xref',
         '10 4',
         '0000000000 65535 f ',
         '0000000042 00000 n ',
         '0000000069 00000 n ',
         '9000000100 00002 n ',
         ''
     ).join(10.chr);
     my Blob $buf = $xref.encode('latin-1');

     my array $entries = .read-xref($buf);
}
```

### `PDF::Native::Writer`

Serialization functions have been implemented for a few PDF data-types:

- boolean, real, integers, literal-strings, hex-strings, names and cross reference tables.

```
use PDF::Native::Writer;

given PDF::Native::Writer {
     say .write-bool(0);    # false
     say .write-bool(1);    # true
     say .write-real(pi);   # 3.14159
     say .write-int(42e3),  # 42000
     say .write-literal("Hi\nthere"); # (Hi\nthere)
     say .write-hex-string("snoopy"); # <736e6f6f7079>
     say .write-name('Hi#there');     # /Hi##there

     # xref entries
     enum <free inuse>;
     my uint64 @xref[4;3] = (
        [0, 65535, free],
        [42, 0, inuse],
        [69, 0, inuse],
        [100, 2, inuse],
     );
     say .write-entries(@xref).lines;
         # 0000000000 65535 f 
         # 0000000042 00000 n 
         # 0000000069 00000 n 
         # 0000000100 00002 n
}
```

## Todo

Some other areas under consideration:

- C equivalents for other PDF::IO::Filter encoding functions, including predictors, ASCII-Hex, ASCII-85 and run-length encoding

- Support for PDF::Content::Image, including color-channel separation and (de)multiplexing. GIF decompression de-interlacing.

There's sure to be others.

** Experimental Only **, at this stage.

Currently giving some benefits on work in development on larger PDFs.

This module will not be deployed until there as been significant advancement on Rakudo/MoarVM JIT, and I've had a chance to benchmark this.


