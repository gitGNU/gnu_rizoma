/*boleta.c
*
*    Copyright 2004 Rizoma Tecnologia Limitada <zeus@emacs.cl>
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
*    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include<gtk/gtk.h>

#include<stdlib.h>

#include"tipos.h"
#include"postgres-functions.h"

gint
get_ticket_number (gint document_type)
{
  PGresult *res;
  gint ticket_number;

  switch (document_type)
    {
    case SIMPLE:
      res = EjecutarSQL ("SELECT num_boleta FROM numeros_documentos");
      break;
    case FACTURA:
      res = EjecutarSQL ("SELECT num_factura FROM numeros_documentos");
      break;
    case GUIA:
      res = EjecutarSQL ("SELECT num_guias FROM numeros_documentos");
      break;
    default:
      return -1;
      break;
    }

  ticket_number = atoi (PQgetvalue (res, 0, 0)) + 1;

  return ticket_number;
}

gboolean
set_ticket_number (gint ticket_number, gint document_type)
{
  PGresult *res;

  switch (document_type)
    {
    case SIMPLE: 
      res = EjecutarSQL (g_strdup_printf ("UPDATE numeros_documentos SET num_boleta=%d", ticket_number));
      break;
    case FACTURA:
      res = EjecutarSQL (g_strdup_printf ("UPDATE numeros_documentos SET num_factura=%d", ticket_number));
      break;
    case GUIA:
      res = EjecutarSQL (g_strdup_printf ("SELECT numeros_documentos SET num_guias=%d", ticket_number));
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
