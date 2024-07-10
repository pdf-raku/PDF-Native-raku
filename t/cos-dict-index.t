use PDF::Native::Cos;
use Test;

sub parse(Str:D $str) {
    CosNode.parse: $str;
}

sub index(CosDict:D $dict) {
    $dict.build-index;
    $dict.index[^$dict.index-len];
}

is-deeply parse("<< >>").&index, (), 'empty dict';
is-deeply parse("<< /a 0 >>").&index, (0,), 'single elem';
is-deeply parse("<< /a 0 /b 1 /c 2 >>").&index, (0,1,2), 'ordered elems';
is-deeply parse("<< /a 0 /b 1 /c 2 /d 3 >>").&index, (0,1,2,3), 'ordered elems';
is-deeply parse("<< /c 0 /b 1 /a 2 >>").&index, (2,1,0), 'reversed elems';
is-deeply parse("<< /a 0 /b null /c 2 >>").&index, (0,2), 'some null entries';
is-deeply parse("<< /a null /b null /c null /b null >>").&index, (), 'all null entries';
is-deeply parse("<< /a 0 /aa 1 /b 2 >>").&index, (1,0,2), 'different length keys';

subtest 'duplicate entries (non-conformant)', {
    is-deeply parse("<< /a 0 /a 1 >>").&index, (0,1), 'singlar duplicate';
    is-deeply parse("<< /a 0 /b 1 /b 2 /c 3 /c 4 >>").&index, (0,1,2,3,4), 'multiple duplicates';
    is-deeply parse("<< /a 0 /c 1 /b 2 /a 4 /c 5 /b 6 >>").&index, (0,3,2,5,1,4), 'more multiple dupls';
}

done-testing;
