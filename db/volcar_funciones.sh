#!/bin/sh
##
# Copyright (C) 2008 Rizoma Tecnologia Limitada <info@rizoma.cl>

# This file is part of rizoma.

# Rizoma is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

DB_NAME=$1
DB_USER=$2
DB_HOST=$3
DB_PORT=$4

if [ -z $1 ] ; then
    echo "* Usage : volcar_funciones.sh db_name db_user db_host db_port"
    exit -1
fi

psql8.3 -h $DB_HOST -p $DB_PORT $DB_NAME -U $DB_USER -W -c "\df public." | grep '|' | grep 'public' | awk -F'|' '{print "DROP FUNCTION" $2 "("$4");"}' >  /tmp/drop_functions.sql

psql8.3 -h $DB_HOST -p $DB_PORT $DB_NAME -U $DB_USER -W <  /tmp/drop_functions.sql
rm /tmp/drop_functions.sql

psql8.3 -h $DB_HOST -p $DB_PORT $DB_NAME -U $DB_USER -W < funciones.sql
