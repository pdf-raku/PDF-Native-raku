[[Raku PDF Project]](https://pdf-raku.github.io)
 / [[PDF-Class Module]](https://pdf-raku.github.io/PDF-Class-raku)

class PDF::Native::Writer
-------------------------

Serialization functions for PDF components and datatypes

Serialization functions have been implemented for a few PDF data-types:

- boolean, real, integers, literal-strings, hex-strings, names and cross reference tables.

```raku
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

Methods
-------

### method write-bool

```raku
method write-bool(
    Bool(Any) $val,
    $buf = Code.new
) returns Str
```

write 'true' or 'false'

### method write-int

```raku
method write-int(
    Int(Any) $val,
    $buf = Code.new
) returns Str
```

write simple integer, e.g. '42'

### method write-real

```raku
method write-real(
    Num(Any) $val,
    $buf = Code.new
) returns Str
```

write number, e.g. '4.2'

### method write-literal

```raku
method write-literal(
    Str(Any) $val,
    Blob $buf? is copy
) returns Str
```

write string literal, e.g. '(Hello, World!)'

### method write-hex-string

```raku
method write-hex-string(
    Str(Any) $val,
    Blob $buf? is copy
) returns Str
```

write binary hex string, e.g. '<deadbeef>'

### multi method write-entries

```raku
multi method write-entries(
    array $xref,
    Blob $buf? is copy
) returns Str
```

write cross reference entries

### method write-name

```raku
method write-name(
    Str(Any) $val,
    Blob $buf? is copy
) returns Str
```

write name, e.g. '/Raku'

