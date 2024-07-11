use PDF::Native::COS;
use Test;

plan 7;

my COSName() $name = 'Hello!';
is $name.ref-count, 1;

is $name.type, +COS_NODE_NAME;
is-deeply $name.value[^$name.value-len], 'Hello!'.ords;
is-deeply $name.Str , 'Hello!';
is-deeply $name.write , ('/Hello!');

my $str = 'Heydər Əliyev';
$name = $str;
is-deeply $name.Str , $str;
is-deeply $name.write , ('/Heyd#c9#99r#20#c6#8fliyev');

done-testing;