[[Raku PDF Project]](https://pdf-raku.github.io)
 / [PDF::Native](https://pdf-raku.github.io/PDF-Native-raku)

# PDF::Native

## Description

This module provides a selection of native implementations of
PDF parsing and functions.

Just installing this module provides an increase in performance,
which is most noticeable when reading or writing larger PDF files.

So far, the areas covered are:

- parsing of native (COS) objects by PDF::IO::Reader
- the PDF::IO::Filter::Predictor `decode` and `encode` functions.
- the widely used PDF::IO::Util `pack` and `unpack` functions.
- reading of cross reference tables and PDF 1.5+ cross reference streams.
- writing of strings, numerics, cross-reference tables and streams.


## Classes in this Distribution

- [PDF::Native::COS](https://pdf-raku.github.io/PDF-Native-raku/PDF/Native/COS)
- [PDF::Native::Filter::Predictors](https://pdf-raku.github.io/PDF-Native-raku/PDF/Native/Filter/Predictors)
- [PDF::Native::Buf](https://pdf-raku.github.io/PDF-Native-raku/PDF/Native/Buf)
- [PDF::Native::Reader](https://pdf-raku.github.io/PDF-Native-raku/PDF/Native/Reader)
- [PDF::Native::Writer](https://pdf-raku.github.io/PDF-Native-raku/PDF/Native/Writer)


## Todo

Some other areas under consideration:

- C equivalents for other PDF::IO::Filter encoding functions, including predictors, ASCII-Hex, ASCII-85 and run-length encoding
- Support for type-1 character transcoding (PDF::Content::Font::Enc::Glyphic)
- Support for PDF::Content::Image, including color-channel separation and (de)multiplexing. GIF decompression de-interlacing.

There's sure to be others.

Currently giving noticeable benefits on larger PDFs.


