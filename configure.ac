# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# This file is part of libflashsupport-pulse.
#
# libflashsupport-pulse is free software; you can redistribute it
# and/or modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either version
# 2 of the License, or (at your option) any later version.
#
# libflashsupport-pulse is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with libflashsupport-pulse; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA.

AC_PREREQ(2.57)

AC_INIT([libflashsupport-jack], 000, [mzcbylcnhqvb (at) 0pointer (dot) de])
AC_CONFIG_SRCDIR([flashsupport.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign -Wall])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_GCC_TRADITIONAL
AC_GNU_SOURCE

# GCC flags

test_gcc_flag() {
    AC_LANG_CONFTEST([int main() {}])
    $CC -c conftest.c $CFLAGS $@ > /dev/null 2> /dev/null
    ret=$?
    rm -f conftest.o
    return $ret
}

# If using GCC specify some additional parameters
if test "x$GCC" = "xyes" ; then

    # We use gnu99 instead of c99 because many have interpreted the standard
    # in a way that int64_t isn't defined on non-64 bit platforms.
    DESIRED_FLAGS="-pipe -Wformat -Wold-style-definition -Wdeclaration-after-statement -Wfloat-equal -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls -Wshadow -Wendif-labels -Wpointer-arith -Wcast-align -Wwrite-strings -Winline -Wno-unused-parameter -ffast-math"

    for flag in $DESIRED_FLAGS ; do
        AC_MSG_CHECKING([whether $CC accepts $flag])
        if test_gcc_flag $flag ; then 
           CFLAGS="$CFLAGS $flag"
           AC_MSG_RESULT([yes])
        else
           AC_MSG_RESULT([no])
        fi
    done 
fi

AM_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_HEADER_STDC

AC_CHECK_PROG(have_pkg_config, pkg-config, yes, no)

if test x"$have_pkg_config" = "xno"; then
    AC_MSG_ERROR(pkg-config is required to install this program)
fi

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(JACK, [jack])

AC_SUBST(JACK_LIBS)
AC_SUBST(JACK_CFLAGS)

PKG_CHECK_MODULES(SRC, [samplerate >= 0.1])

AC_SUBST(SRC_LIBS)
AC_SUBST(SRC_CFLAGS)


PKG_CHECK_MODULES(LIBSSL, [ libssl ])

AC_SUBST(LIBSSL_LIBS)
AC_SUBST(LIBSSL_CFLAGS)

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
