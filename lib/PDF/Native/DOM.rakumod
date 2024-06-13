unit class PDF::Native::DOM;

use PDF::Native::Defs :types;
use NativeCall;

enum PDF_NODE_TYPE «
    PDF_NODE_ANY
    PDF_NODE_REFERENCE
    PDF_NODE_ARRAY
    PDF_NODE_DICT
    PDF_NODE_BOOL
    PDF_NODE_REAL
    PDF_NODE_LITERAL
    PDF_NODE_HEX_STRING
    PDF_NODE_NULL
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

class PdfNode is repr('CStruct') is export {
    has uint8 $.type;

    method delegate {
        my $class := @ClassMap[$!type];
        nativecast($class, self);
    }
    method cast(Pointer:D $p) {
        my $type := nativecast(PdfNode, $p).type;
        my $class := @ClassMap[$type];
        nativecast($class, $p);
    }
}

#| Indirect object or object reference
class PdfNodeRef is repr('CStruct') is PdfNode is export {
    also does domNode[$?CLASS, PDF_NODE_REFERENCE];
    has uint64 $.obj-num;
    has uint32 $.gen-num;
    has PdfNode $.value;
}

class PdfNodeArray is repr('CStruct') is PdfNode is export {
    also does domNode[$?CLASS, PDF_NODE_ARRAY];
    # naive implementation for now
    has size_t $.len;
    has CArray[PdfNode] $.value;
}

class PdfNodeDict is repr('CStruct') is PdfNode is export {
    # naive implementation for now
    also does domNode[$?CLASS, PDF_NODE_DICT];
    has size_t $.len;
    has CArray[Str] $.keys;
    has CArray[PdfNode] $.values;
    method value {
        (^$!len).map: { $!keys[$_] => $!values[$_] }
    }
}

class PdfNodeBool is repr('CStruct') is PdfNode is export {
    also does domNode[$?CLASS, PDF_NODE_BOOL];
    has PDF_TYPE_BOOL $.value;
}

class PdfNodeReal is repr('CStruct') is PdfNode is export {
    also does domNode[$?CLASS, PDF_NODE_REAL];
    has PDF_TYPE_REAL $.value;
}

class PdfNodeLiteral is repr('CStruct') is PdfNode is export {
    also does domNode[$?CLASS, PDF_NODE_LITERAL];
    has Str $.value;
}

class PdfNodeHexString is repr('CStruct') is PdfNode is export {
    also does domNode[$?CLASS, PDF_NODE_HEX_STRING];
    has Str $.value;
}

class PdfNodeNull is repr('CStruct') is PdfNode is export {
    also does domNode[$?CLASS, PDF_NODE_NULL];
    method value { Any }
}

