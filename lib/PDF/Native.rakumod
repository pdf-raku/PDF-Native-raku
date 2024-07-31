use v6;

#| native implementations of PDF functions.
unit class PDF::Native:ver<0.1.10>;

method lib-version is DEPRECATED<^ver> {
    self.^ver;
}

