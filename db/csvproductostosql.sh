#!/bin/sh

cat $1 | awk -F, '{print ("INSERT INTO productos VALUES("$1","$2","$3","$4","$5","$6",0,0,0,0,TRUE,-1,0,FALSE,0,0,0,FALSE,FALSE,0);")}'
