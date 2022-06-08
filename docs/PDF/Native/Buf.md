[[Raku PDF Project]](https://pdf-raku.github.io)
 / [[PDF-Class Module]](https://pdf-raku.github.io/PDF-Class-raku)

module PDF::Native::Buf
-----------------------

General packing and unpacking of binary words

Handles the packing and unpacking of multi-byte quantities as network words. Such as `/BitsPerComponent` in `/Filter` `/DecodeParms`.

Also handles variable byte packing and unpacking. As seen in the `/W` parameter to XRef streams, and a few other places.

```raku
# pack two 4-byte words into an 8 byte buffer
use PDF::Native::Buf :pack;
my blob32 $words .= new(660510, 2634300);
my blob8 $bytes = pack($words, 24);

# pack triples as 1 byte, 2 bytes, 1 byte
my uint32 @in[4;3] = [1, 16, 0], [1, 741, 0], [1, 1030, 0], [1, 1446, 0];
my @W = packing-widths(@in, 3);    # [1, 2, 0];
$bytes = pack(@in, @W);
```

Subroutines
-----------

### multi sub unpack

```raku
multi sub unpack(
    $nums,
    $n where { ... }
) returns Blob
```

unpack bytes to an array of words, each of a given size

### multi sub pack

```raku
multi sub pack(
    $nums,
    $n where { ... }
) returns Blob
```

upack bytes to an array words, each of a given size

### multi sub unpack

```raku
multi sub unpack(
    $in,
    Array $W
) returns array
```

variable unpacking of records, e.g. to decode/encode: obj 123 0 << /Type /XRef /W [1, 3, 1]

### multi sub pack

```raku
multi sub pack(
    $in,
    Array $W
) returns Blob
```

variable packing of records

