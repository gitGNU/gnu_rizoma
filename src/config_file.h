/* -*- Mode: C; tab-width: 4; indent-tabs-mode: f; c-basic-offset: 4 -*- */
/*config_file.h
*
*    Copyright (C) 2004,2008 Rizoma Tecnologia Limitada <info@rizoma.cl>
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
*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _CONFIG_FILE_H

#define _CONFIG_FILE_H

typedef struct __parms {
  char *var_name;
  char **value;
} Parms;

typedef struct __parms_dimensions {
  char *var_name;
  Position **values;
} ParmsDimensions;

int read_dimensions (char *file, ParmsDimensions parms[], int total);

char * rizoma_get_value (char *var_name);

int rizoma_set_value (char *var_name, char *new_value);

void rizoma_set_profile (gchar *group_name);

int rizoma_extract_xy (char *value, float *x, float *y);

#endif
