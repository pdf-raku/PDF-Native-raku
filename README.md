# lib-pdftools
Low level native library of PDF specific functions.

The main aim is to optionally boost performance in the PDF tool-chain including
PDF stream and filter functions and PDF::Content image processing and encoding functions.

So far covered are low level bulk word packing and unpacking - a pure Perl
implmentation of the widely used PDF::IO::Util::resample function.

Some other areas being considered:

- C equivalents for PDF::IO::Filter encoding functions, including predictors, ascii-hex, ascii-85 and run-length encoding

- Support for PDF::Content::Image, including base-64 encoding/decoding, color-channel seperation and (de)multiplexing. GIF decompression de-interlacing.

There's sure to be others.

** Experimental Only **, at this stage. Needs further development and benchmarking.
