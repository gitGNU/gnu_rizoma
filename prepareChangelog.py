#!/usr/bin/python
## prepareChangelog.git.py
## Login : <freyes@yoda>
## Started on  Tue Mar 25 15:05:46 2008 Felipe Reyes
##
## Copyright (C) 2008 Felipe Reyes <freyes@emacs.cl>
##
## $Id$
##
## Copyright (C) 2008 Felipe Reyes
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
##

import os
import time
import tempfile

CHANGELOG = 'ChangeLog'

f_status = os.popen('git status', 'r')

i=0
keep=False

curr_date = time.localtime(time.time())
change_date = change_date = str(curr_date[0]) + '-' +\
              str('%.2d' % curr_date[1]) + ' ' +  \
              str('%.2d' % curr_date[2])

f_author_name = os.popen('git config --get user.name').readline()
author_name = f_author_name[:len(f_author_name)-1]

f_author_email = os.popen('git config --get user.email').readline()
author_email = f_author_email[:len(f_author_email)-1]

modified_files = list()

for line in f_status.readlines():
    if ((i==4) or (keep)):
        if len(line) == 2:
            keep=False
        else:
            l = line.split(' ')
            f_file = l[len(l)-1]
            f_file = f_file[:len(f_file)-1]
            modified_files.append(f_file)
            keep=True
    i += 1

if len(l) == 0:
    print "There is no files waiting for commit"
else:
    header = change_date + "  " + author_name + "  <" + author_email + ">"
    header += "\n\n"

    for l in modified_files:
        header +="\t* " + l + ": \n"

    header += "\n"

tmp_dir = tempfile.mkdtemp()
os.system('cp ' + CHANGELOG + ' ' + tmp_dir)

changelog_file = open(CHANGELOG,'w')
changelog_file.write(header)

tmp_file = open(tmp_dir + "/" + CHANGELOG, 'r')

for l in tmp_file.readlines():
    changelog_file.write(l)

tmp_file.close()
changelog_file.close()
os.system('rm -rf ' + tmp_dir)

os.system("$EDITOR " + CHANGELOG)
