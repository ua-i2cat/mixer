#!/usr/bin/env bash

libtoolize
aclocal
autoconf
automake -a -v
./configure $@

