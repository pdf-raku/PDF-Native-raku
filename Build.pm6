#! /usr/bin/env perl6
use v6;

class Build {
    need LibraryMake;
    # adapted from deprecated Native::Resources

    #| Sets up a C<Makefile> and runs C<make>.  C<$folder> should be
    #| C<"$folder/resources/libraries"> and C<$libname> should be the name of the library
    #| without any prefixes or extensions.
    sub make(Str $folder, Str $destfolder, IO() :$libname!, :$gcc) {
        my %vars = LibraryMake::get-vars($destfolder);

        if $gcc {
            note "Setting to GCC...";
            %vars<MAKE> = 'make';
            %vars<CC> = 'gcc';
            %vars<CCFLAGS> = '-fPIC -O3 -DNDEBUG --std=gnu99 -Wextra -Wall -Wno-unused-parameter';
            %vars<LD> = 'gcc';
            %vars<LDSHARED> = '-shared';
            %vars<LDFLAGS> = "-fPIC -O3 -Lresources/libraries";
            %vars<CCOUT> = '-o ';
            %vars<LDOUT> = '-o ';
 
        }

        mkdir($destfolder);
	LibraryMake::process-makefile($folder, %vars);

	%vars<DEST> = "../../resources/libraries";
        %vars<LIB_NAME> = ~ $*VM.platform-library-name($libname);

	LibraryMake::process-makefile($folder~'/src/pdf', %vars);
	shell(%vars<MAKE>);
    }

    method build($workdir, :$rebuild, :$gcc) {
        if $rebuild {
            my $destdir = 'resources/libraries';
            mkdir $destdir;
            make($workdir, "$destdir", :libname<pdf>, :$gcc);
        }
        else {
            note "Using pre-built library ('{$?FILE.IO.basename} --rebuild' to override)";
        }
        True;
    }
}

# Build.pm6 can also be run standalone
sub MAIN(Str $working-directory = '.', :$rebuild = !Rakudo::Internals.IS-WIN, :$gcc ) {
    Build.new.build($working-directory, :$rebuild, :$gcc);
}
