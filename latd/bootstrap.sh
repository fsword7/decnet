#!/bin/sh
# Bootstrap the automake system.
aclocal && automake --gnu --add-missing && autoconf

