#!/bin/sh
#
#    Copyright (C) 2004 Rizoma Tecnologia Limitada <info@rizoma.cl>
#
#    This file is part of rizoma.
#
#    Rizoma is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

if [ "$1" = "--help" ]; then
    echo "Usage: autogen.sh [options]"
    echo "Options:"
    echo "--help         : Display this help message"
    echo "--with-errors  : Show verbose information"
    echo ""
    exit;
fi

if [ "$1" = "--with-errors" ]; then
    ERROR="YES"
else
    ERROR="NO"
fi

if autoconf --version > /dev/null 2>&1; then

    if autoconf --version | grep '2.[5670]' > /dev/null>&1; then
	echo "autoconf   ... yes"
    else
	echo "autoconf   ... no"
	echo "Error: You must have installed a version >= 2.5"
	exit;
    fi
else
    echo "Error: You must have autoconf installed"
    exit;
fi

if automake --version > /dev/null 2>&1; then

    if automake --version | grep '1.[567890]*' > /dev/null>&1; then
	echo "automake   ... yes"
    else
	echo "automake   ... no"
	echo "Error: You must have installed a version >= 1.5"
	exit;
    fi
else
    echo "Error: You must have automake installed"
    exit;
fi

if aclocal --version > /dev/null 2>&1; then

    if aclocal --version | grep '1.[567890]*' > /dev/null>&1; then
	echo "aclocal    ... yes"
    else
	echo "aclocal    ... no"
	echo "Error: You must have installed a version >= 1.5"
	exit;
    fi
else
    echo "Error: You must have aclocal installed"
    exit;
fi

if autoheader --version> /dev/null 2>&1; then

    if autoheader --version | grep '2.[5-9]' > /dev/null>&1; then
	echo "autoheader ... yes"
    else
	echo "autoheader ... no"
	echo "Error: You must have installed a version >= 2.5"
	exit;
    fi
else
    echo "Error: You must have autoheader installed"
    exit;
fi

echo "Creating aclocal.m4"
aclocal -I m4/
echo "Creating config.h.in"
autoheader
echo "Adding missing standards files to package"
if [ "$ERROR" = "YES" ]; then
    automake --add-missing --copy
else
    automake --add-missing --copy > /dev/null 2>&1;
fi
echo "Creating configure file"
if [ "$ERROR" = "YES" ]; then
    autoconf
else
    autoconf > /dev/null 2>&1;
fi
echo "Now we going to run the configure script"
./configure $@
