unit class PDF::Native::COS;

use PDF::Native::Defs :types, :libpdf;
use NativeCall;

enum COS_NODE_TYPE is export «
    COS_NODE_ANY
    COS_NODE_ARRAY
    COS_NODE_BOOL
    COS_NODE_DICT
    COS_NODE_HEX
    COS_NODE_IND_OBJ
    COS_NODE_INT
    COS_NODE_LITERAL
    COS_NODE_NAME
    COS_NODE_NULL
    COS_NODE_REAL
    COS_NODE_REF
»;

our @ClassMap;

constant lock = Lock.new;

role CosType[$class, UInt:D $type] is export {
    @ClassMap[$type] = $class;

    #| Only needed on tree/fragment root nodes.

    submethod DESTROY {
        self.done();
    }

    method delegate {
        fail "expected node of type $type, got {self.type}"
            unless self.type == $type;
        
        self
    }
}

class CosNode is repr('CStruct') is export {
    has uint16 $.type;
    has uint16 $.ref-count;

    method !cos_node_reference() is native(libpdf) {*}
    method !cos_node_done() is native(libpdf) {*}

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
    method Str {
        my Buf[uint8] $buf .= allocate(20);
        my $n = self!cos_ref_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
}

class CosCryptCtx is repr('CStruct') is export {

    # encryption key
    has CArray[uint8] $.key;
    has int32 $.key-len;

    # parent indirect object
    has uint64 $.obj-num;
    has uint32 $.gen-num;

    # scratch buffer
    has CArray[uint8] $.buf;
    has size_t $buf-len;
}

class CosIndObj is repr('CStruct') is CosNode is export {
    also does CosType[$?CLASS, COS_NODE_IND_OBJ];
    has uint64 $.obj-num;
    has uint32 $.gen-num;
    has CosNode $.value;
    method value { $!value.delegate }
    #| Indirect objects the top of the tree and always fragments

    method !cos_ind_obj_new(uint64, uint32, CosNode --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_ind_obj_write(Blob, size_t --> size_t) is native(libpdf) {*}
    method !cos_ind_obj_crypt(Blob, size_t, &crypt-cb (CosCryptCtx, CArray[uint8], size_t)) is native(libpdf) {*}
    method crypt(Blob() $key, &crypt-func) {
        self!cos_ind_obj_crypt($key, $key.bytes, &crypt-func);
    }
    method new(UInt:D :$obj-num!, UInt:D :$gen-num = 0, CosNode:D :$value!) {
        self!cos_ind_obj_new($obj-num, $gen-num, $value);
    }
    method Str {
        my Buf[uint8] $buf .= allocate(200);
        my $n = self!cos_ind_obj_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
}

class CosArray is CosNode is repr('CStruct') is export {
    also does CosType[$?CLASS, COS_NODE_ARRAY];
    # naive implementation for now
    has size_t $.elems;
    has CArray[CosNode] $.values;
    method !cos_array_new(CArray[CosNode], size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_array_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(CArray[CosNode] :$values!, UInt:D :$elems = $values.elems) {
        self!cos_array_new($values, $elems);
    }
    method AT-POS(UInt:D() $idx) {
        $idx < $!elems
            ?? $!values[$idx].delegate
            !! CosNode;
    }
    method Str {
        my Buf[uint8] $buf .= allocate(200);
        my $n = self!cos_array_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
}

class CosDict is repr('CStruct') is CosArray is export {
    also does CosType[$?CLASS, COS_NODE_DICT];
    has CArray[CArray[uint32]] $.keys;
    has CArray[uint16]   $.key-lens;
    method !cos_dict_new(CArray, CArray[CosNode], CArray[uint16], size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_dict_write(Blob, size_t --> size_t) is native(libpdf) {*}
    method !cos_dict_lookup(PDF_TYPE_CODE_POINTS, uint16 --> CosNode) is native(libpdf) {*}

    method new(
        CArray :$keys!,
        CArray[CosNode] :$values!,
        CArray[uint16] :$key-lens = CArray[uint16].new($keys.map(*.elems)),
        UInt:D :$elems = $values.elems,
) {
        self!cos_dict_new($keys, $values, $key-lens, $elems);
    }

    method AT-KEY(Str:D() $key) {
        my PDF_TYPE_CODE_POINTS $cps .= new: $key.ords;
        my CosNode $value = self!cos_dict_lookup($cps, $cps.elems);
        $value.defined ?? $value.delegate !! $value;
    }

    method Str {
        my Buf[uint8] $buf .= allocate(200);
        my $n = self!cos_dict_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
   
}

class CosBool is repr('CStruct') is CosNode is export {
    also does CosType[$?CLASS, COS_NODE_BOOL];
    has PDF_TYPE_BOOL $.value;

    method !cos_bool_new(PDF_TYPE_BOOL --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_bool_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(Bool:D :$value!) {
        self!cos_bool_new($value);
    }
    method Str {
        my Buf[uint8] $buf .= allocate(20);
        my $n = self!cos_bool_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }
}

class CosInt is repr('CStruct') is CosNode is export {
    also does CosType[$?CLASS, COS_NODE_INT];
    has PDF_TYPE_INT $.value;

    method !cos_int_new(PDF_TYPE_INT --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_int_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(UInt:D :$value!) {
        self!cos_int_new($value);
    }
    method Str {
        my Buf[uint8] $buf .= allocate(20);
        my $n = self!cos_int_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }
}

class CosReal is repr('CStruct') is CosNode is export {
    also does CosType[$?CLASS, COS_NODE_REAL];
    has PDF_TYPE_REAL $.value;
    method !cos_real_new(PDF_TYPE_REAL --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_real_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(Num:D :$value!) {
        self!cos_real_new($value);
    }
    method Str {
        my Buf[uint8] $buf .= allocate(20);
        my $n = self!cos_real_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }
}

class _CosStringy is repr('CStruct') is CosNode {
    has CArray[uint8] $.value;
    has size_t $.value-len;
    method value { (^$!value-len).map({$!value[$_].chr}).join }
}

class CosLiteral is repr('CStruct') is _CosStringy is export {
    also does CosType[$?CLASS, COS_NODE_LITERAL];
    method !cos_literal_new(blob8, size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_literal_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(blob8:D :$value!, UInt:D :$value-len = $value.elems) {
        self!cos_literal_new($value, $value-len);
    }
    method Str {
        my Buf[uint8] $buf .= allocate(20);
        my $n = self!cos_literal_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
}

class CosHexString is repr('CStruct') is _CosStringy is export {
    also does CosType[$?CLASS, COS_NODE_HEX];
    method !cos_hex_string_new(blob8, size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_hex_string_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(blob8:D :$value!, UInt:D :$value-len = $value.elems) {
        self!cos_hex_string_new($value, $value-len);
    }
    method Str {
        my Buf[uint8] $buf .= allocate(20);
        my $n = self!cos_hex_string_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode: "latin-1";
    }
}

class CosName is repr('CStruct') is CosNode is export {
    also does CosType[$?CLASS, COS_NODE_NAME];
    has CArray[uint32] $.value; # code-points
    has uint16 $.value-len;
    method !cos_name_new(CArray[uint32], uint16 --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_name_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(CArray[uint32] :$value!, UInt:D :$value-len = $value.elems) {
        self!cos_name_new($value, $value-len);
    }
    method Str {
        my Buf[uint8] $buf .= allocate(20);
        my $n = self!cos_name_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }
}

class CosNull is repr('CStruct') is CosNode is export {
    also does CosType[$?CLASS, COS_NODE_NULL];
    method value { Any }

    method !cos_null_new(--> ::?CLASS:D) is native(libpdf) {*}
    method !cos_null_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new {
        self!cos_null_new();
    }
    method Str {
        my Buf[uint8] $buf .= allocate(10);
        my $n = self!cos_null_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }
}

