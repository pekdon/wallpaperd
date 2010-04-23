#!/bin/sh

die() {
    echo $@
    exit 1
}

aclocal || die "failed to run aclocal"
autoheader || die "failed to run autoheader"
autoconf || die "failed to run autoconf"
automake -a || die "failed to run automake"
