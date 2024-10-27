use v6;

#| native implementations of PDF functions.
unit class PDF::Native:ver<0.1.11>;

method lib-version is DEPRECATED<^ver> {
    self.^ver;
}

