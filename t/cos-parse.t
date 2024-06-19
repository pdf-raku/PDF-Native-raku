use PDF::Grammar::COS;
use PDF::Native::COS;
use PDF::Native::COS::Actions;
use Test;

plan 3;

my PDF::Native::COS::Actions:D $actions .= new;

given PDF::Grammar::COS.parse('123', :rule<number>, :$actions) {
      my CosInt:D $value = .ast;
      is $value.Str, '123', 'parse int'; 
}

given PDF::Grammar::COS.parse('123.45', :rule<number>, :$actions) {
      my CosReal:D $value = .ast;
      is $value.Str, '123.45', 'parse real'; 
}

given PDF::Grammar::COS.parse('.45', :rule<number>, :$actions) {
      my CosReal:D $value = .ast;
      is $value.Str, '0.45', 'parse real fraction'; 
}
