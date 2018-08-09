#! /usr/bin/env perl6
use v6;

class Build {
    need LibraryMake;
    # adapted from deprecated Native::Resources

    #| Sets up a C<Makefile> and runs C<make>.  C<$folder> should be
    #| C<"$folder/resources/lib"> and C<$libname> should be the name of the library
    #| without any prefixes or extensions.
    sub make(Str $folder, Str $destfolder, Str :$libname) {
        my %vars = LibraryMake::get-vars($destfolder);

        mkdir($destfolder);
	LibraryMake::process-makefile($folder, %vars);

	%vars<DEST> = "../../resources/lib";
	LibraryMake::process-makefile($folder~'/src/pdf', %vars);
	shell(%vars<MAKE>);

        my @fake-lib-exts = <.so .dll .dylib>.grep(* ne %vars<SO>);
        "resources/lib/lib$libname$_".IO.open(:w) for @fake-lib-exts;
    }

    method build($workdir) {
        my $destdir = 'resources/lib';
        mkdir $destdir;
        make($workdir, "$destdir", :libname<pdf>);
        True;
    }
}

# Build.pm can also be run standalone
sub MAIN(Str $working-directory = '.' ) {
    Build.new.build($working-directory);
}
