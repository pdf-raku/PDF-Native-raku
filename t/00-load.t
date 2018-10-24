use v6;
use Test;
plan 1;

use PDF::Native;

is-deeply PDF::Native.lib-version , PDF::Native.^ver, 'version';
done-testing;