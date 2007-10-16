/*config_file.h
*
*    Copyright 2004 Rizoma Tecnologia Limitada <jonathan@rizoma.cl>
*
*    This file is part of rizoma.
*
*    Rizoma is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software
*    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _CONFIG_FILE_H

#define _CONFIG_FILE_H

typedef struct _rizoma_conf {
  struct _rizoma_conf *back;
  char *var_name;
  char *valor;

  struct _rizoma_conf *next;
} RizomaConf;

typedef struct __parms {
  char *var_name;
  char **value;
} Parms;

typedef struct __parms_dimensions {
  char *var_name;
  Position **values;
} ParmsDimensions;

RizomaConf *rizoma_config;

int read_conf (char *file, Parms parms[], int total);

int read_dimensions (char *file, ParmsDimensions parms[], int total);

RizomaConf * rizoma_read_conf (char *file);

char * rizoma_get_value (RizomaConf *header, char *var_name);

int rizoma_set_value (RizomaConf *header, char *var_name, char *new_value);

int rizoma_save_file (RizomaConf *header);

int rizoma_extract_xy (char *value, float *x, float *y);

#endif
