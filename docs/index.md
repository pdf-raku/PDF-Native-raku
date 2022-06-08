[[Raku PDF Project]](https://pdf-raku.github.io)
 / [PDF::Native](https://pdf-raku.github.io/PDF-Native-raku)

# PDF::Native

## Description

This module provides a selection of native implementations of
PDF functions.

Just installing this module along with PDF v0.5.7+ provides some
increase in performance.

Currently, this module implements a hand-full of functions, mostly
related to reading and writing larger PDF files.

So far, just a subset of potential areas are covered:

=item the PDF::IO::Filter::Predictor `decode` and `encode` functions.
=item the widely used PDF::IO::Util `pack` and `unpack` functions.
=item reading of cross reference tables and PDF 1.5+ cross reference streams.
=item writing of strings, numerics, cross-reference tables and streams.

## Classes in this Distribution

### `PDF::Native::Filter::Predictors`

### `PDF::Native::Buf`

### `PDF::Native::Reader`

### `PDF::Native::Writer`


## Todo

Some other areas under consideration:

- C equivalents for other PDF::IO::Filter encoding functions, including predictors, ASCII-Hex, ASCII-85 and run-length encoding
- Support for type-1 character transcoding (PDF::Content::Font::Enc::Glyphic)
- Support for PDF::Content::Image, including color-channel separation and (de)multiplexing. GIF decompression de-interlacing.

There's sure to be others.

Currently giving noticeable benefits on larger PDFs.


