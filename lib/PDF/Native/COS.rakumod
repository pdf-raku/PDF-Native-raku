# Data structures for native PDF parsing and serialization
unit module PDF::Native::COS;

=begin pod

=head2 Synopsis

=begin code :lang<raku>
use PDF::Native::COS;

my $stream = q:to<--END-->;
    << /Length 45 >> stream
    BT
    /F1 24 Tf
    100 250 Td (Hello, world!) Tj
    ET
    endstream
    --END--

my COSIndObj $ind-obj .= parse: "123 4 obj\n{$stream}endobj";
say $ind-obj.write;    # serialize to PDF
say $ind-obj.ast.raku; # mimic PDF::Grammar::COS.parse( $_ , :rule<ind-obj>);
my COSInt $val .= parse: "42"; # simple object parse
say $ind-obj.value.dict<Length>.cmp($val); # derefencing and comparision

=end code

=head2 Description

This is under development as a set of objects for the native construction and serialization of COS (PDF) objects.

It utilized by L<PDF::IO::Reader> and L<PDF::IO::Writer> to provide faster reading and writing of larger PDF files.

=end pod

use PDF::Native::Defs :types, :libpdf;
use NativeCall;

enum COS_NODE_TYPE is export «
    COS_NODE_ANY
    COS_NODE_ARRAY
    COS_NODE_BOOL
    COS_NODE_DICT
    COS_NODE_HEX_STR
    COS_NODE_IND_OBJ
    COS_NODE_INT
    COS_NODE_LIT_STR
    COS_NODE_NAME
    COS_NODE_NULL
    COS_NODE_REAL
    COS_NODE_REF
    COS_NODE_STREAM
    COS_NODE_CONTENT
    COS_NODE_OP
    COS_NODE_OP_IMAGE_DATA
»;

constant %TypeMap = %(
    :array(COS_NODE_ARRAY),
    :bool(COS_NODE_BOOL),
    :dict(COS_NODE_DICT),
    :int(COS_NODE_INT),
    :hex-string(COS_NODE_HEX_STR),
    :ind-obj(COS_NODE_IND_OBJ),
    :literal(COS_NODE_LIT_STR),
    :name(COS_NODE_NAME),
    :null(COS_NODE_NULL),
    :real(COS_NODE_REAL),
    :ind-ref(COS_NODE_REF),
    :stream(COS_NODE_STREAM),
    :content(COS_NODE_CONTENT),
    :op(COS_NODE_OP),
    %);

enum COS_CMP is export «
   COS_CMP_EQUAL
   COS_CMP_SIMILAR
   COS_CMP_DIFFERENT
   COS_CMP_DIFFERENT_TYPE
   »;

enum COS_CRYPT_MODE is export «
    COS_CRYPT_ALL
    COS_CRYPT_ONLY_STRINGS
    COS_CRYPT_ONLY_STREAMS
   »;

my subset LatinStr of Str:D where !.contains(/<-[\x0..\xff \n]>/);
our @ClassMap;

role COSType[$class, UInt:D $type] is export {
    @ClassMap[$type] = $class;

    # Only needed on tree/fragment root nodes.
    submethod DESTROY {
        self.done();
    }

    method delegate { ... }

}

#| Generic COS objects
class COSNode is repr('CStruct') is export {
    has uint8 $.type;
    has uint8 $!private;
    has uint16 $.ref-count;

    method !cos_node_reference() is native(libpdf) {*}
    method !cos_node_done() is native(libpdf) {*}
    method !cos_node_cmp(COSNode --> int32) is native(libpdf) {*}
    our sub cos_parse_obj(Blob, size_t --> ::?CLASS:D) is native(libpdf) {*}

    #| Parse a COS object
    multi method parse(LatinStr:D $str --> COSNode) {
        my blob8 $buf = $str.encode: "latin-1";
        cos_parse_obj($buf, $buf.bytes).delegate;
    }

    method reference {
        self!cos_node_reference();
        self;
    }

    method done {
        self!cos_node_done();
    }

    method delegate {
        with self {
            my $class := @ClassMap[$!type];
            nativecast($class, $_).reference;
        }
        else {
            self;
        }
    }

    method cast(Pointer:D $p) {
        my $type := nativecast(COSNode, $p).type;
        my $class := @ClassMap[$type];
        nativecast($class, $p).reference;
    }

    method cmp(COSNode $obj) {
        self!cos_node_cmp($obj);
    }
    multi method COERCE(COSNode:D $_) is default { $_ }
    multi method COERCE(Pair:D $_) is default {
        my $type := %TypeMap{.key} // COS_NODE_NULL;
        @ClassMap[$type].COERCE: .value;
    }
    multi method COERCE(Any:U) { @ClassMap[COS_NODE_NULL].new }
    multi method COERCE(Str:D $_) {
         @ClassMap[COS_NODE_LIT_STR].COERCE: $_
    }
    multi method COERCE(Bool:D $value) {
         @ClassMap[COS_NODE_BOOL].new: :$value
    }
    multi method COERCE(Int:D $value) {
         @ClassMap[COS_NODE_INT].new: :$value
    }
    multi method COERCE(Numeric:D $_) {
         @ClassMap[COS_NODE_REAL].COERCE: $_
    }
    multi method COERCE(@a) {
         @ClassMap[COS_NODE_ARRAY].COERCE: @a
    }
    multi method COERCE(Int:D :int($value)!) { @ClassMap[COS_NODE_INT].new: :$value }
    multi method COERCE(%h) {
        if %h.keys == 1 && %TypeMap{%h.keys[0]} {
            self.COERCE: %h.pairs[0];
        }
        elsif (%h<dict>:exists) || (%h<encoded>:exists) {
            @ClassMap[COS_NODE_STREAM].COERCE: %h
        }
        else {
            @ClassMap[COS_NODE_DICT].COERCE: %h
        }
    }
    method new(|) { fail }
}

#| Indirect object reference
class COSRef is repr('CStruct') is COSNode is export {
    also does COSType[$?CLASS, COS_NODE_REF];
    has uint64 $.obj-num;
    has uint32 $.gen-num;

    our sub cos_ref_new(uint64, uint32 --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_ref_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(UInt:D :$obj-num!, UInt:D :$gen-num = 0) {
        cos_ref_new($obj-num, $gen-num);
    }
    method write(::?CLASS:D: buf8 :$buf = buf8.allocate(20)) handles<Str> {
        my $n = self!cos_ref_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
    method ast { :ind-ref[ $!obj-num, $!gen-num] }
    multi method COERCE( @ [ UInt:D $obj-num, UInt:D $gen-num, *@ ] ) {
        self.new: :$obj-num, :$gen-num;
    }
}

#| An encryption context
class COSCryptCtx is repr('CStruct') is export {

    # encryption key
    has CArray[uint8] $.key;
    has int32 $.key-len;

    has uint32  $!crypt-mode;
    has Pointer $.crypt-func;

    has CArray[uint8] $.buf;
    has size_t $buf-len;

    # parent indirect object
    has uint64 $.obj-num;
    has uint32 $.gen-num;

    our sub cos_crypt_ctx_new(&crypt-func (COSCryptCtx, CArray[uint8], size_t), int32 $mode, Blob:D() $key, int32 $key-len --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_crypt_ctx_done() is native(libpdf) {*}

    method new(Blob:D() :$key!, :&crypt-func!, UInt:D :$mode = COS_CRYPT_ALL) {
        cos_crypt_ctx_new(&crypt-func, $mode, $key, $key.bytes, );
    }

    submethod DESTROY { self!cos_crypt_ctx_done() }
}

#| Array object
class COSArray is COSNode is repr('CStruct') is export {
    also does COSType[$?CLASS, COS_NODE_ARRAY];
    has size_t $.elems;
    has CArray[COSNode] $.values;
    method AT-POS(UInt:D() $idx --> COSNode) {
        $idx < $!elems
            ?? $!values[$idx].delegate
            !! COSNode;
    }
    our sub cos_array_new(CArray[COSNode], size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_array_write(Blob, size_t, int32 --> size_t) is native(libpdf) {*}

    method new(CArray[COSNode] :$values!, UInt:D :$elems = $values.elems) {
        cos_array_new($values, $elems);
    }
    method write(::?CLASS:D: buf8 :$buf is copy = buf8.allocate(200), Bool :$compact, Int:D :$indent = $compact ?? -1 !! 0) handles<Str> {
        my $n;
        my $tries;
        repeat {
            $n = self!cos_array_write($buf, $buf.bytes, $indent);
            last if ++$tries > 5;
            $buf = buf8.allocate(3 * $buf.bytes + 1)
                unless $n;
        } until $n;
        fail "Unable to write array" unless $n;
        $buf.subbuf(0,$n).decode: "latin-1";
    }
    method ast { :array[ (^$!elems).map: { $!values[$_].delegate.ast } ] }
    multi method COERCE(@array) {
        my CArray[COSNode] $values .= new: @array.map: { COSNode.COERCE: $_ };
        self.new: :$values;
    }

}

#| Name object
class COSName is repr('CStruct') is COSNode is export {
    also does COSType[$?CLASS, COS_NODE_NAME];

    has CArray[uint32] $.value; # code-points
    has uint16 $.value-len;

    our sub cos_name_new(CArray[uint32], uint16 --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_name_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(CArray[uint32] :$value!, UInt:D :$value-len = $value.elems) {
        cos_name_new($value, $value-len);
    }
    method write(::?CLASS:D: buf8 :$buf = buf8.allocate(32)) {
        my $n = self!cos_name_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }
    method Str { (^$!value-len).map({$!value[$_].chr}).join }
    method ast { :name(my $ = self.Str) }
    multi method COERCE(Str:D $str) {
        my CArray[uint32] $value .= new: $str.ords;
        self.new: :$value;
    }
}

#| Dictionary (hash) object
class COSDict is COSNode is repr('CStruct') is export {
    also does COSType[$?CLASS, COS_NODE_DICT];

    has size_t $.elems;
    has CArray[COSNode] $.values;
    has CArray[COSName] $!keys;
    has CArray[size_t] $.index;
    has size_t $.index-len;

    our sub cos_dict_new(CArray[COSName], CArray[COSNode], size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_dict_write(Blob, size_t, int32 --> size_t) is native(libpdf) {*}
    method !cos_dict_build_index(--> Pointer[size_t]) is native(libpdf) {*}
    method !cos_dict_lookup(COSName --> COSNode) is native(libpdf) {*}

    method new(
        CArray[COSName] :$keys,
        CArray[COSNode] :$values!,
        UInt:D :$elems = $values.elems,
    ) {
        cos_dict_new($keys, $values, $elems);
    }

    method AT-KEY(COSName:D() $key --> COSNode) {
        my COSNode $value = self!cos_dict_lookup($key);
        $value.defined ?? $value.delegate !! $value;
    }

    method AT-POS(UInt:D() $idx --> COSNode) {
        $idx < $!elems
            ?? $!values[$idx].delegate
            !! COSNode;
    }

    method build-index {
        self!cos_dict_build_index() unless $!index;
    }

    method ast { dict => %( (^$!elems).map: { $!keys[$_].ast.value => $!values[$_].delegate.ast }) }

    method write(::?CLASS:D: buf8
                 :$buf is copy = buf8.allocate(200),
                                 Bool :$compact,
                                 Int:D :$indent = $compact ?? -1 !! 0,
                ) handles <Str> {
        my $n;
        my $tries;
        repeat {
            $n = self!cos_dict_write($buf, $buf.bytes, $indent);
            last if ++$tries > 5;
            $buf = buf8.allocate(3 * $buf.bytes + 1)
                unless $n;
        } until $n;
        fail "Unable to write dictionary" unless $n;
        $buf.subbuf(0,$n).decode: "latin-1";
    }
    multi method COERCE(Hash $dict) {
        my @keys = $dict.keys.sort: {
            when 'Type'              {"0"}
            when 'Subtype'|'S'       {"1"}
            when .ends-with('Type')  {"1" ~ $_}
            when 'Length'            {"z"}
            default                  {$_}
        };

        my CArray[COSName] $keys   .= new: @keys.map: { COSName.COERCE: $_ };
        my CArray[COSNode] $values .= new: @keys.map: { COSNode.COERCE: $dict{$_} };

        self.new: :$keys, :$values;
    }
}

constant $CLIB is export(:CLIB) = BEGIN Rakudo::Internals.IS-WIN ?? 'msvcrt' !! Str;
sub memcpy(Blob $dest, CArray $chars, size_t $n) is native($CLIB) {*};
sub to-blob( CArray[uint8] $value, UInt:D $len ) {
    my blob8 $buf .= allocate($len);
    memcpy($buf, $value, $len);
    $buf;
}

#| Stream object
class COSStream is repr('CStruct') is COSNode is export {
    also does COSType[$?CLASS, COS_NODE_STREAM];

    class ValueUnion is repr('CUnion') {
        has size_t  $.value-len;  # value length, if data attached
        has size_t  $.value-pos;  # position in source buffer, otherwise
    }

    has COSDict          $.dict;
    has CArray[uint8]    $.value;
    HAS ValueUnion       $!u;
    method value-len { $!value.defined ?? $!u.value-len !! Int }
    method value-pos { $!value.defined ?? Int !! $!u.value-pos }

    our sub cos_stream_new(COSDict:D, Blob, size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_stream_attach_data(Blob, size_t, size_t --> int32) is native(libpdf) {*}
    method !cos_stream_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(COSDict:D :$dict!, Blob :$value, UInt:D :$value-len = $value ?? $value.bytes !! 0) {
        cos_stream_new($dict, $value, $value-len);
    }

    method presize-write-buf(buf8:D $buf is rw) {
        # resize a writer buffer.
        if $!value.defined && $buf.bytes < $!u.value-len {
            # Buffer is definitely not big enough for stream data. Resize.
            $buf = buf8.allocate($!u.value-len + 500);
        }
    }

    method write(::?CLASS:D: buf8 :$buf is copy = buf8.allocate(500)) handles<Str> {
        self.presize-write-buf($buf);
        my $n = self!cos_stream_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
    method attach-data(Blob:D $buf, UInt:D $len) {
        self!cos_stream_attach_data($buf, $buf.bytes, $len);
    }
    multi sub coerce-stream(Blob:D $_) { $_ }
    multi sub coerce-stream(LatinStr:D $_) { .encode: "latin-1" }
    multi sub coerce-stream(Any:U $_) { blob8 }
    method ast {
        my Pair $body = do with $!value {
            # stream attached
            encoded => .&to-blob($!u.value-len).decode: "latin-1";
        }
        else {
            # stream not attached
            start => $!u.value-pos;
        };
        stream => %( $!dict.ast, $body );
    }
    multi method COERCE(%s) {
        my COSDict $dict .= COERCE(%s<dict> // {});
        my $value := coerce-stream(%s<encoded>);
        self.new: :$dict, :$value;
    }
}

#| Indirect object
class COSIndObj is repr('CStruct') is COSNode is export {
    also does COSType[$?CLASS, COS_NODE_IND_OBJ];
    has uint64  $.obj-num;
    has uint32  $.gen-num;
    has COSNode $.value;
    method value { $!value.delegate }

    our sub cos_ind_obj_new(uint64, uint32, COSNode --> ::?CLASS:D) is native(libpdf) {*}
    our sub cos_parse_ind_obj(Blob, size_t, int32 --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_ind_obj_write(Blob, size_t --> size_t) is native(libpdf) {*}
    method !cos_ind_obj_crypt(COSCryptCtx:D) is native(libpdf) {*}
    method crypt(COSCryptCtx:D :$crypt-ctx!) {
        self!cos_ind_obj_crypt($crypt-ctx);
    }
    method new(UInt:D :$obj-num!, UInt:D :$gen-num = 0, COSNode:D :$value!) {
        cos_ind_obj_new($obj-num, $gen-num, $value);
    }
    multi method parse(LatinStr:D $str, |c) {
        my blob8 $buf = $str.encode: "latin-1";
        self.parse: $buf, |c;
    }
    multi method parse(Blob:D $buf, Bool :$scan = False) {
        cos_parse_ind_obj($buf, $buf.bytes, +$scan);
    }
    method write(::?CLASS:D: buf8 :$buf! is rw) {
        my $tries;
        my $n;
        my $obj = self.value;
        $obj.presize-write-buf($buf) if $obj.isa(COSStream);
        repeat {
            $n = self!cos_ind_obj_write($buf, $buf.bytes);
            last if ++$tries > 3;
            $buf = buf8.allocate(3 * $buf.bytes + 1)
                unless $n;
        } until $n;
        fail "Unable to write indirect object $!obj-num $!gen-num R in {$buf.bytes} bytes" unless $n;
        $buf.subbuf(0,$n).decode: "latin-1";
    }
    method Str(Blob :$buf is copy = buf8.allocate(512)) {
        self.write: :$buf;
    }
    method ast { :ind-obj[ $!obj-num, $!gen-num, $.value.ast ] }
    multi method COERCE(@a where .elems >= 3) {
        my UInt:D $obj-num = @a[0];
        my UInt:D $gen-num = @a[1];
        my COSNode:D() $value = @a[2];
        self.new: :$obj-num, :$gen-num, :$value;
    }
}

#| Boolean object
class COSBool is repr('CStruct') is COSNode is export {
    also does COSType[$?CLASS, COS_NODE_BOOL];
    has PDF_TYPE_BOOL $.value handles <so>;

    our sub cos_bool_new(PDF_TYPE_BOOL --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_bool_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(Bool:D :$value!) {
        cos_bool_new($value);
    }
    method ast { $!value.so }
    method write(::?CLASS:D: buf8 :$buf = buf8.allocate(20)) handles<Str> {
        my $n = self!cos_bool_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }
    multi method COERCE(Bool() $value) {
        self.new: :$value;
    }
}

#| Integer object
class COSInt is repr('CStruct') is COSNode is export {
    also does COSType[$?CLASS, COS_NODE_INT];
    has PDF_TYPE_INT $.value handles<Int>;

    our sub cos_int_new(PDF_TYPE_INT --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_int_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(Int:D :$value!) {
        cos_int_new($value);
    }
    method write(::?CLASS:D: buf8 :$buf = buf8.allocate(20)) handles<Str> {
        my $n = self!cos_int_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }
    method ast { $!value }
    multi method COERCE(Int:D() $value) {
        self.new: :$value;
    }
}

#| Real object
class COSReal is repr('CStruct') is COSNode is export {
    also does COSType[$?CLASS, COS_NODE_REAL];
    has PDF_TYPE_REAL $.value handles<Num>;
    our sub cos_real_new(PDF_TYPE_REAL --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_real_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(Num:D() :$value!) {
        cos_real_new($value);
    }
    method ast { $!value }
    method write(::?CLASS:D: buf8 :$buf = buf8.allocate(20)) handles<Str> {
        my $n = self!cos_real_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }

    multi method COERCE(Num:D() $value) {
        self.new: :$value;
    }
}

class _COSStringy is COSNode {
    has CArray[uint8] $.value;
    has size_t $.value-len;

    multi method COERCE(LatinStr:D $str) {
        my blob8:D $value = $str.encode: "latin-1";
        self.new: :$value;
    }
    method value { self.Str }
    method Str {
        $!value.&to-blob($!value-len).decode: "latin-1";
    }
}

class COSLiteralString is repr('CStruct') is _COSStringy is export {
    also does COSType[$?CLASS, COS_NODE_LIT_STR];
    our sub cos_literal_new(blob8, size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_literal_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(blob8:D :$value!, UInt:D :$value-len = $value.elems) {
        cos_literal_new($value, $value-len);
    }
    method ast { :literal(my $ = self.Str) }
    method write(::?CLASS:D: buf8 :$buf = buf8.allocate(50)) {
        my $n = self!cos_literal_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
}

#| Hex string object
class COSHexString is repr('CStruct') is _COSStringy is export {
    also does COSType[$?CLASS, COS_NODE_HEX_STR];
    our sub cos_hex_string_new(blob8, size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_hex_string_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(blob8:D :$value!, UInt:D :$value-len = $value.elems) {
        cos_hex_string_new($value, $value-len);
    }
    method ast { :hex-string(my $ = self.Str) }
    method write(::?CLASS:D: buf8 :$buf = buf8.allocate(50)) {
        my $n = self!cos_hex_string_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
}

#| Null object
class COSNull is repr('CStruct') is COSNode is export {
    also does COSType[$?CLASS, COS_NODE_NULL];
    method value handles <defined> { Any }

    our sub cos_null_new(--> ::?CLASS:D) is native(libpdf) {*}
    method !cos_null_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new {
        cos_null_new();
    }
    method ast { :null(Any) }
    method write(buf8 :$buf = buf8.allocate(10)) handles<Str> {
        my $n = self!cos_null_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }
}

class COSOp is repr('CStruct') is COSNode is export {
    also does COSType[$?CLASS, COS_NODE_OP];
    has size_t $.elems;
    has CArray[COSNode] $.values;
    has Str $.opn;
    has int32 $.sub-type;

    our sub cos_op_new(Str, int32, CArray[COSNode], size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_op_write(Blob, size_t, int32 --> size_t) is native(libpdf) {*}

    method new(Str:D :$opn!, CArray[COSNode] :$values, UInt:D :$elems = $values ?? $values.elems !! 0) {
        cos_op_new($opn, $opn.codes, $values, $elems);
    }
    method AT-POS(UInt:D() $idx --> COSNode) {
        $idx < $!elems
            ?? $!values[$idx].delegate
            !! COSNode;
    }
    method write(::?CLASS:D: buf8 :$buf is copy = buf8.allocate(512), Int:D :$indent = 0) handles<Str> {
        my $n = self!cos_op_write($buf, $buf.bytes, $indent);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
    method ast {
        my $ast := $!opn => [ (^$!elems).map: { $!values[$_].delegate.ast } ];
        $!sub-type ?? $ast !! ('??' => $ast);
    }
}

class COSContent is repr('CStruct') is COSNode is export {
    also does COSType[$?CLASS, COS_NODE_CONTENT];
    has size_t $.elems;
    has CArray[COSOp] $.values;

    our sub cos_content_new(CArray[COSOp], size_t --> ::?CLASS:D) is native(libpdf) {*}
    our sub cos_parse_content(Blob, size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_content_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(CArray[COSOp] :$values!, UInt:D :$elems = $values.elems) {
        cos_content_new($values, $elems);
    }

    multi method parse(LatinStr:D $str --> COSNode) {
        my blob8 $buf = $str.encode: "latin-1";
        cos_parse_content($buf, $buf.bytes);
    }
    method AT-POS(UInt:D() $idx --> COSNode) {
        $idx < $!elems
            ?? $!values[$idx].delegate
            !! COSNode;
    }

    method write(::?CLASS:D: buf8 :$buf is copy = buf8.allocate(512)) handles<Str> {
        my $n = self!cos_content_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
    method ast {
        :content[ (^$!elems).map: { $!values[$_].delegate.ast } ]
    }
}
