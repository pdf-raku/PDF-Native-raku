# Data structures for PDF parsing and serialization
unit module PDF::Native::Cos;

=begin pod

=head2 Description

This under development is a set of objects for the native construction and serialization of COS (PDF) objects.

In particular, CosIndObj is designed as drop in replacement for L<PDF::IO::IndObj>.

=head2 Todo

- `ast()` method on indirect objects
- a native object parser

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
constant lock = Lock.new;

role CosType[$class, UInt:D $type] is export {
    @ClassMap[$type] = $class;

    # Only needed on tree/fragment root nodes.
    submethod DESTROY {
        self.done();
    }

    method delegate {
        fail "expected node of type $type, got {self.type}"
            unless self.type == $type;
        
        self;
    }
}

#| Generic Cos objects
class CosNode is repr('CStruct') is export {
    has uint8 $.type;
    has uint8 $!private;
    has uint16 $.ref-count;

    method !cos_node_reference() is native(libpdf) {*}
    method !cos_node_done() is native(libpdf) {*}
    method !cos_node_cmp(CosNode --> int32) is native(libpdf) {*}

    method reference {
        self!cos_node_reference();
        self;
    }

    method done {
        self!cos_node_done();
    }

    method delegate {
        my $class := @ClassMap[$!type];
        nativecast($class, self).reference;
    }
    method cast(Pointer:D $p) {
        my $type := nativecast(CosNode, $p).type;
        my $class := @ClassMap[$type];
        nativecast($class, $p).reference;
    }

    method cmp(CosNode $obj) {
        self!cos_node_cmp($obj);
    }
    multi method COERCE(CosNode:D $_) is default { $_ }
    multi method COERCE(Pair:D $_) {
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
class CosRef is repr('CStruct') is CosNode is export {
    also does CosType[$?CLASS, COS_NODE_REF];
    has uint64 $.obj-num;
    has uint32 $.gen-num;

    method !cos_ref_new(uint64, uint32 --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_ref_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(UInt:D :$obj-num!, UInt:D :$gen-num = 0) {
        self!cos_ref_new($obj-num, $gen-num);
    }
    method Str(buf8 :$buf = buf8.allocate(20)) {
        my $n = self!cos_ref_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
    multi method COERCE( @ [ UInt:D $obj-num, UInt:D $gen-num ] ) {
        self.new: :$obj-num, :$gen-num;
    }
}

#| An encryption context
class CosCryptCtx is repr('CStruct') is export {

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

    method !cos_crypt_ctx_new(&crypt-func (CosCryptCtx, CArray[uint8], size_t), int32 $mode, Blob:D() $key, int32 $key-len --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_crypt_ctx_done() is native(libpdf) {*}

    method new(Blob:D() :$key!, :&crypt-func!, UInt:D :$mode = COS_CRYPT_ALL) {
        self!cos_crypt_ctx_new(&crypt-func, $mode, $key, $key.bytes, );
    }

    submethod DESTROY { self!cos_crypt_ctx_done() }
}

#| Indirect object
class CosIndObj is repr('CStruct') is CosNode is export {
    also does CosType[$?CLASS, COS_NODE_IND_OBJ];
    has uint64 $.obj-num;
    has uint32 $.gen-num;
    has CosNode $.value;
    method value { $!value.delegate }
    #| Indirect objects the top of the tree and always fragments

    method !cos_ind_obj_new(uint64, uint32, CosNode --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_ind_obj_write(Blob, size_t --> size_t) is native(libpdf) {*}
    method !cos_ind_obj_crypt(CosCryptCtx:D) is native(libpdf) {*}
    method crypt(CosCryptCtx:D :$crypt-ctx!) {
        self!cos_ind_obj_crypt($crypt-ctx);
    }
    method new(UInt:D :$obj-num!, UInt:D :$gen-num = 0, CosNode:D :$value!) {
        self!cos_ind_obj_new($obj-num, $gen-num, $value);
    }
    method Str(buf8 :$buf is copy = buf8.allocate(512)) {
        my $n;
        my $tries;
        repeat {
            $n = self!cos_ind_obj_write($buf, $buf.bytes);
            $buf = buf8.allocate(3 * $buf.bytes + 1)
                unless $n;
        } until $n || ++$tries > 5;
        fail "Unable to write indirect object" unless $n;
        $buf.subbuf(0,$n).decode: "latin-1";
    }
    multi method COERCE(@a where .elems >= 3) {
        my UInt:D $obj-num = @a[0];
        my UInt:D $gen-num = @a[1];
        my CosNode:D() $value = @a[2];
        self.new: :$obj-num, :$gen-num, :$value;
    }
}

#| Array object
class CosArray is CosNode is repr('CStruct') is export {
    also does CosType[$?CLASS, COS_NODE_ARRAY];
    has size_t $.elems;
    has CArray[CosNode] $.values;
    method AT-POS(UInt:D() $idx) {
        $idx < $!elems
            ?? $!values[$idx].delegate
            !! CosNode;
    }
    method !cos_array_new(CArray[CosNode], size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_array_write(Blob, size_t, int32 --> size_t) is native(libpdf) {*}

    method new(CArray[CosNode] :$values!, UInt:D :$elems = $values.elems) {
        self!cos_array_new($values, $elems);
    }
    method Str(buf8 :$buf is copy = buf8.allocate(200), Bool :$compact, Int:D :$indent = $compact ?? -1 !! 0) {
        my $n;
        my $tries;
        repeat {
            $n = self!cos_array_write($buf, $buf.bytes, $indent);
            $buf = buf8.allocate(3 * $buf.bytes + 1)
                unless $n;
        } until $n || ++$tries > 5;
        fail "Unable to write array" unless $n;
        $buf.subbuf(0,$n).decode: "latin-1";
    }
    multi method COERCE(@array) {
        my CArray[CosNode] $values .= new: @array.map: { CosNode.COERCE: $_ };
        self.new: :$values;
    }

}

#| Name object
class CosName is repr('CStruct') is CosNode is export {
    also does CosType[$?CLASS, COS_NODE_NAME];

    has CArray[uint32] $.value; # code-points
    has uint16 $.value-len;

    method !cos_name_new(CArray[CosName], uint16 --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_name_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(CArray[uint32] :$value!, UInt:D :$value-len = $value.elems) {
        self!cos_name_new($value, $value-len);
    }
    method Str(buf8 :$buf = buf8.allocate(32)) {
        my $n = self!cos_name_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }
    multi method COERCE(Str:D $str) {
        my CArray[uint32] $value .= new: $str.ords;
        self.new: :$value;
    }
}

#| Dictionary (hash) object
class CosDict is CosNode is repr('CStruct') is export {
    also does CosType[$?CLASS, COS_NODE_DICT];

    has size_t $.elems;
    has CArray[CosNode] $.values;
    has CArray[CosName] $!keys;
    has CArray[size_t] $.index;
    has size_t $.index-len;

    method !cos_dict_new(CArray[CosName], CArray[CosNode], size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_dict_write(Blob, size_t, int32 --> size_t) is native(libpdf) {*}
    method !cos_dict_build_index(--> Pointer[size_t]) is native(libpdf) {*}
    method !cos_dict_lookup(CosName --> CosNode) is native(libpdf) {*}

    method new(
        CArray[CosName] :$keys,
        CArray[CosNode] :$values!,
        UInt:D :$elems = $values.elems,
    ) {
        self!cos_dict_new($keys, $values, $elems);
    }

    method AT-KEY(CosName:D() $key) {
        my CosNode $value = self!cos_dict_lookup($key);
        $value.defined ?? $value.delegate !! $value;
    }

    method AT-POS(UInt:D() $idx) {
        $idx < $!elems
            ?? $!values[$idx].delegate
            !! CosNode;
    }

    method build-index {
        self!cos_dict_build_index() unless $!index;
    }

    method Str(buf8 :$buf is copy = buf8.allocate(200), Bool :$compact, Int:D :$indent = $compact ?? -1 !! 0) {
        my $n;
        my $tries;
        repeat {
            $n = self!cos_dict_write($buf, $buf.bytes, $indent);
            $buf = buf8.allocate(3 * $buf.bytes + 1)
                unless $n;
        } until $n || ++$tries > 5;
        fail "Unable to write dictionary" unless $n;
        $buf.subbuf(0,$n).decode: "latin-1";
    }
    multi method COERCE(%dict) {
        my @keys = %dict.keys.sort: {
            when 'Type'              {"0"}
            when 'Subtype'|'S'       {"1"}
            when .ends-with('Type')  {"1" ~ $_}
            when 'Length'            {"z"}
            default                  {$_}
        };
  
        my CArray[CosName] $keys   .= new: @keys.map: { CosName.COERCE: $_ };
        my CArray[CosNode] $values .= new: @keys.map: { CosNode.COERCE: %dict{$_} };

        self.new: :$keys, :$values;
    }
}

#| Stream object
class CosStream is repr('CStruct') is CosNode is export {
    also does CosType[$?CLASS, COS_NODE_STREAM];
    has CosDict          $.dict;
    has CArray[uint8]    $.value;
    has size_t           $.value-len;

    method !cos_stream_new(CosDict:D, Blob, size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_stream_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(CosDict:D :$dict!, Blob:D :$value!, UInt:D :$value-len = $value.bytes) {
        self!cos_stream_new($dict, $value, $value-len);
    }

    method Str(buf8 :$buf = buf8.allocate(500)) {
        my $n = self!cos_stream_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
    multi sub coerce-stream(Blob $_) { $_ }
    multi sub coerce-stream(LatinStr:D $_) { .encode: "latin-1" }
    multi method COERCE(%s) {
        my CosDict $dict .= COERCE(%s<dict> // {});
        my $value := coerce-stream(%s<encoded> // '');
        self.new: :$dict, :$value;
    }
}

#| Boolean object
class CosBool is repr('CStruct') is CosNode is export {
    also does CosType[$?CLASS, COS_NODE_BOOL];
    has PDF_TYPE_BOOL $.value;

    method !cos_bool_new(PDF_TYPE_BOOL --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_bool_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(Bool:D :$value!) {
        self!cos_bool_new($value);
    }
    method Str(buf8 :$buf = buf8.allocate(20)) {
        my $n = self!cos_bool_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }
    multi method COERCE(Bool() $value) {
        self.new: :$value;
    }
}

#| Integer object
class CosInt is repr('CStruct') is CosNode is export {
    also does CosType[$?CLASS, COS_NODE_INT];
    has PDF_TYPE_INT $.value handles<Int>;

    method !cos_int_new(PDF_TYPE_INT --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_int_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(Int:D :$value!) {
        self!cos_int_new($value);
    }
    method Str(buf8 :$buf = buf8.allocate(20)) {
        my $n = self!cos_int_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }
    multi method COERCE(Int:D() $value) {
        self.new: :$value;
    }
}

#| Real object
class CosReal is repr('CStruct') is CosNode is export {
    also does CosType[$?CLASS, COS_NODE_REAL];
    has PDF_TYPE_REAL $.value handles<Num>;
    method !cos_real_new(PDF_TYPE_REAL --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_real_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(Num:D() :$value!) {
        self!cos_real_new($value);
    }
    method Str(buf8 :$buf = buf8.allocate(20)) {
        my $n = self!cos_real_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }

    multi method COERCE(Num:D() $value) {
        self.new: :$value;
    }
}

class _CosStringy is CosNode {
    has CArray[uint8] $.value;
    has size_t $.value-len;
    method value { (^$!value-len).map({$!value[$_].chr}).join }

    multi method COERCE(LatinStr:D $str) {
        my blob8:D $value = $str.encode: "latin-1";
        self.new: :$value;
    }
    method Str {
        fail "Please use CosLiteralString or CosHexString subclass";
    }
}

class CosLiteralString is repr('CStruct') is _CosStringy is export {
    also does CosType[$?CLASS, COS_NODE_LIT_STR];
    method !cos_literal_new(blob8, size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_literal_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(blob8:D :$value!, UInt:D :$value-len = $value.elems) {
        self!cos_literal_new($value, $value-len);
    }
    method Str(buf8 :$buf = buf8.allocate(50)) {
        my $n = self!cos_literal_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
}

#| Hex string object
class CosHexString is repr('CStruct') is _CosStringy is export {
    also does CosType[$?CLASS, COS_NODE_HEX_STR];
    method !cos_hex_string_new(blob8, size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_hex_string_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(blob8:D :$value!, UInt:D :$value-len = $value.elems) {
        self!cos_hex_string_new($value, $value-len);
    }
    method Str(buf8 :$buf = buf8.allocate(50)) {
        my $n = self!cos_hex_string_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
}

#| Null object
class CosNull is repr('CStruct') is CosNode is export {
    also does CosType[$?CLASS, COS_NODE_NULL];
    method value { Any }

    method !cos_null_new(--> ::?CLASS:D) is native(libpdf) {*}
    method !cos_null_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new {
        self!cos_null_new();
    }
    method Str(buf8 :$buf = buf8.allocate(10)) {
        my $n = self!cos_null_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }
}

