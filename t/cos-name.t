use PDF::Native::Cos;
use Test;

plan 5;

my CosName() $name = 'Hello!';
is $name.ref-count, 1;

is $name.type, +COS_NODE_NAME;
is-deeply $name.value[^$name.value-len], 'Hello!'.ords;
is-deeply $name.Str , ('/Hello!');

$name = 'Heydər Əliyev';
is-deeply $name.Str , ('/Heyd#c9#99r#20#c6#8fliyev');

done-testing;