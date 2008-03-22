/* -*- Mode: C; tab-width: 4; ident-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
/*boleta.c
 *
 *    Copyright (C) 2004 Rizoma Tecnologia Limitada <info@rizoma.cl>
 *
 *       This program is free software; you can redistribute it and/or modify
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

#include<stdlib.h>

#include"tipos.h"
#include"postgres-functions.h"
#include"rizoma_errors.h"

gint
get_ticket_number (gint document_type)
{
  PGresult *res;
  gint ticket_number;

  switch (document_type)
    {
    case SIMPLE:
      res = EjecutarSQL ("SELECT num_boleta FROM obtener_num_boleta() "
                         "as (num_boleta int4)");
      break;
    case FACTURA:
      res = EjecutarSQL ("SELECT num_factura FROM obtener_num_factura() "
                         "as (num_factura int4)");
      break;
    case GUIA:
      res = EjecutarSQL ("SELECT num_guias FROM obtener_num_guias() "
                         "as (num_guias int4)");
      break;
    default:
      return -1;
      break;
    }

  if( PQntuples( res ) == 0 ) {
    rizoma_errors_set( "Ocurrio un error al intentar obtener el numero de documento", "get_ticket_number()", ALERT );
    rizoma_error_window( NULL );
    return 0;
  } else {
    ticket_number = atoi (PQgetvalue (res, 0, 0)) + 1;

    return ticket_number;
  }
}

gboolean
set_ticket_number (gint ticket_number, gint document_type)
{
  PGresult *res;
  gchar *q = NULL;

  switch (document_type)
    {
    case SIMPLE:
      q = g_strdup_printf ("SELECT update_num_boleta(%d)", ticket_number);
      res = EjecutarSQL (q);
      g_free(q);
      break;
    case FACTURA:
      q = g_strdup_printf ("SELECT update_num_factura(%d)", ticket_number);
      res = EjecutarSQL (q);
      g_free (q);
      break;
    case GUIA:
      q = g_strdup_printf ("SELECT update_num_guias(%d)", ticket_number);
      res = EjecutarSQL (q);
      g_free (q);
      break;
    default:
      return -1;
      break;
    }

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}
