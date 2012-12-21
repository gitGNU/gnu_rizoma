/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
       c-indentation-style: gnu -*- */
/*encriptar.c
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

#include<gtk/gtk.h>

#include<stdio.h>
#include<string.h>

#include"tipos.h"

#include<rizoma_errors.h>
#include"postgres-functions.h"

gboolean
CompararPassword (gchar *passwd_db, gchar *passwd)
{
  if (strcmp (passwd_db, passwd) == 0)
    return TRUE;
  else
    return FALSE;
}

gboolean
AcceptPassword (gchar *passwd, gchar *user)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT * FROM users WHERE passwd=md5('%s') AND usuario='%s'", passwd, user));

  if (res != NULL && PQntuples (res) == 0)
    rizoma_errors_set ("El usuario o la contraseña no son correctos", "AcceptPassword ()", ALERT);

  if (res != NULL && PQntuples (res) != 0)
    return TRUE;
  else
    return FALSE;
}

gchar *
AcceptOnlyPassword (gchar *passwd)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT usuario FROM users WHERE passwd=md5('%s')", passwd));

  if (res != NULL && PQntuples (res) == 0)
    {
      rizoma_errors_set ("La contraseña no existe", "AcceptPassword ()", ALERT);
      return g_strdup ("");
    }
  else
    return g_strdup (PQvaluebycol (res, 0, "usuario"));
}
