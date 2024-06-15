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

role cosNode[$class, UInt:D $type] is export {
    @ClassMap[$type] = $class;
    method delegate {
        fail "expected node of type $type, got {self.type}"
            unless self.type == $type;
        self
    }
}

class CosNode is repr('CStruct') is export {
    has uint8 $.type;
    has uint8 $.ref-count;

    method delegate {
        my $class := @ClassMap[$!type];
        nativecast($class, self);
    }
    method cast(Pointer:D $p) {
        my $type := nativecast(CosNode, $p).type;
        my $class := @ClassMap[$type];
        nativecast($class, $p);
    }

    #| Only needed on tree/fragment root nodes.
    method !cos_node_done() is native(libpdf) {*}

    submethod DESTROY {
        lock.protect: { self!cos_node_done(); }
    }
}

#| Indirect object reference
class CosRef is repr('CStruct') is CosNode is export {
    also does cosNode[$?CLASS, COS_NODE_REF];
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
        $buf.subbuf(0,$n).decode;
    }
}

class CosIndObj is repr('CStruct') is CosNode is export {
    also does cosNode[$?CLASS, COS_NODE_IND_OBJ];
    has uint64 $.obj-num;
    has uint32 $.gen-num;
    has CosNode $.value;
    method value { $!value.delegate }
    #| Indirect objects the top of the tree and always fragments

    method !cos_ind_obj_new(uint64, uint32, CosNode --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_ind_obj_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(UInt:D :$obj-num!, UInt:D :$gen-num = 0, CosNode:D :$value!) {
        self!cos_ind_obj_new($obj-num, $gen-num, $value);
    }
    method Str {
        my Buf[uint8] $buf .= allocate(200);
        my $n = self!cos_ind_obj_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }
}

class CosArray is CosNode is repr('CStruct') is export {
    also does cosNode[$?CLASS, COS_NODE_ARRAY];
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
        $buf.subbuf(0,$n).decode;
    }
}

class CosDict is repr('CStruct') is CosArray is export {
    also does cosNode[$?CLASS, COS_NODE_DICT];
    has CArray[CArray[uint32]] $.keys;
    has CArray[uint8]   $.key-lens;
    method !cos_dict_new(CArray, CArray[CosNode], CArray[uint8], size_t --> ::?CLASS:D) is native(libpdf) {*}
    method !cos_dict_write(Blob, size_t --> size_t) is native(libpdf) {*}

    method new(
        CArray :$keys!,
        CArray[CosNode] :$values!,
        CArray[uint8] :$key-lens = CArray[uint8].new($keys.map(*.elems)),
        UInt:D :$elems = $values.elems,
) {
        self!cos_dict_new($keys, $values, $key-lens, $elems);
    }

    method AT-KEY(Str:D() $key) {
        # todo: implement in C. higher order key search
        for (^$.elems) {
            my $len := $!key-lens[$_];
            my $k = $!keys[$_][^$len].map(*.chr).join;
            return $.values[$_].delegate
                if $key eq $k;
        }
        CosNode;
     }
    method Str {
        my Buf[uint8] $buf .= allocate(200);
        my $n = self!cos_dict_write($buf, $buf.bytes);
        $buf.subbuf(0,$n).decode;
    }
   
}

class CosBool is repr('CStruct') is CosNode is export {
    also does cosNode[$?CLASS, COS_NODE_BOOL];
    has PDF_TYPE_BOOL $.value;
}

class CosInt is repr('CStruct') is CosNode is export {
    also does cosNode[$?CLASS, COS_NODE_INT];
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
    also does cosNode[$?CLASS, COS_NODE_REAL];
    has PDF_TYPE_REAL $.value;
}

class _CosStringy is repr('CStruct') is CosNode {
    has Str $.value;
}

class CosLiteral is repr('CStruct') is _CosStringy is export {
    also does cosNode[$?CLASS, COS_NODE_LITERAL];
}

class CosHexString is repr('CStruct') is _CosStringy is export {
    also does cosNode[$?CLASS, COS_NODE_HEX];
}

class CosName is repr('CStruct') is _CosStringy is export {
    also does cosNode[$?CLASS, COS_NODE_NAME];
}

class CosNull is repr('CStruct') is CosNode is export {
    also does cosNode[$?CLASS, COS_NODE_NULL];
    method value { Any }
}

