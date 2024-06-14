unit class PDF::Native::COS;

use PDF::Native::Defs :types;
use NativeCall;

enum COS_NODE_TYPE «
    COS_NODE_ANY
    COS_NODE_ARRAY
    COS_NODE_BOOL
    COS_NODE_DICT
    COS_NODE_HEX_STRING
    COS_NODE_LITERAL
    COS_NODE_NAME
    COS_NODE_NULL
    COS_NODE_REAL
    COS_NODE_REFERENCE
»;

our @ClassMap;

role domNode[$class, UInt:D $type] is export {
    @ClassMap[$type] = $class;
    method delegate {
        fail "expected node of type $type, got {self.type}"
            unless self.type == $type;
        self
    }
}

class CosNode is repr('CStruct') is export {
    has uint8 $.type;

    method delegate {
        my $class := @ClassMap[$!type];
        nativecast($class, self);
    }
    method cast(Pointer:D $p) {
        my $type := nativecast(CosNode, $p).type;
        my $class := @ClassMap[$type];
        nativecast($class, $p);
    }
}

#| Indirect object or object reference
class CosRef is repr('CStruct') is CosNode is export {
    also does domNode[$?CLASS, COS_NODE_REFERENCE];
    has uint64 $.obj-num;
    has uint32 $.gen-num;
    has CosNode $.value;
}

class CosArray is repr('CStruct') is CosNode is export {
    also does domNode[$?CLASS, COS_NODE_ARRAY];
    # naive implementation for now
    has size_t $.len;
    has CArray[CosNode] $.value;
}

class CosDict is repr('CStruct') is CosNode is export {
    # naive implementation for now
    also does domNode[$?CLASS, COS_NODE_DICT];
    has size_t $.len;
    has CArray[Str] $.keys;
    has CArray[CosNode] $.values;
    method value {
        (^$!len).map: { $!keys[$_] => $!values[$_] }
    }
}

class CosBool is repr('CStruct') is CosNode is export {
    also does domNode[$?CLASS, COS_NODE_BOOL];
    has PDF_TYPE_BOOL $.value;
}

class CosReal is repr('CStruct') is CosNode is export {
    also does domNode[$?CLASS, COS_NODE_REAL];
    has PDF_TYPE_REAL $.value;
}

class _CosStringy is repr('CStruct') is CosNode {
    has Str $.value;
}

class CosLiteral is repr('CStruct') is _CosStringy is export {
    also does domNode[$?CLASS, COS_NODE_LITERAL];
}

class CosHexString is repr('CStruct') is _CosStringy is export {
    also does domNode[$?CLASS, COS_NODE_HEX_STRING];
}

class CosName is repr('CStruct') is _CosStringy is export {
    also does domNode[$?CLASS, COS_NODE_NAME];
}

class CosNull is repr('CStruct') is CosNode is export {
    also does domNode[$?CLASS, COS_NODE_NULL];
    method value { Any }
}

