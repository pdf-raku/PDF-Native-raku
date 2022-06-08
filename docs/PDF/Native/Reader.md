[[Raku PDF Project]](https://pdf-raku.github.io)
 / [[PDF-Class Module]](https://pdf-raku.github.io/PDF-Class-raku)

class PDF::Native::Reader
-------------------------

Reading of PDF components and datatypes

The only method so far implemented is `read-xref` for the fast reading of cross reference indices.

```raku
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

