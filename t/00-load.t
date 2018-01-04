use v6;
use Test;
plan 1;

use Lib::PDF;

is-deeply Lib::PDF.lib-version , Lib::PDF.^ver, 'version';
done-testing;