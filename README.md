# Take

Purpose of **take** is to provide a convenient way for selecting items
from a list within a text terminal interactively. **take** is the best
tool for filtering a selection when the list of items can't be matched
with wildcards or regexps, i.e. they don't share substrings.

Selection list can be build in number of ways (also from STDIN). When
selection is ready, **take** executes provided command using the
selection.


# Example screencast

![take screencast](take.gif)


# Installation

**take** is compiled and installed using the typical autotools flow:

    ./configure
    make
    make install

Use '--prefix' option for 'configure' if you don't want the default
installation location.

Take prefers 'termbox' library, but at minimum 'ncurses' is
required. Also a Command line parser library called Como is
used. Otherwise Take is "self-contained", i.e. does not depend on
external libs.

Options for 'configure', see: configure --help:

    mark-color  Color highlighting for marked lines (in red).

See the file 'INSTALL' for details.


# Documentation

**take** includes a comprehensive manual page.


# License

See: COPYING



Copyright (c) 2015 Tero Isannainen.
