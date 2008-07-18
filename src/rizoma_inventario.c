/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
/*rizoma-inventario.c
 *
 *    Copyright (C) 2008 Rizoma Tecnologia Limitada <info@rizoma.cl>
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

#include <glib.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "tipos.h"

#include "postgres-functions.h"
#include "config_file.h"
#include "utils.h"
#include "manejo_productos.h"

char *
get_line (FILE *fd)
{
  int c;
  int i = 0;
  char *line = (char *) malloc (255);

  memset (line, '\0', 255);

  for (c = fgetc (fd); c != '#' && c != '\n' && c != EOF; c = fgetc (fd))
    {
      *line++ = c;
      i++;
    }


  for (; c != '\n' && c != EOF; c = fgetc (fd));


  if (i > 0 || c != EOF)
    {
      for (; i != 0; i--)
        *line--;

      return line;
    }
  else
    return NULL;
}

/* void */
/* IngresarFakeCompra (void) */
/* { */
/*   Productos *products = compra->header_compra; */
/*   gint id, doc; */
/*   gchar *rut_proveedor; */

/*   PGresult *res; */

/*   res = EjecutarSQL ("SELECT id FROM compras ORDER BY id DESC LIMIT 1"); */

/*   id = atoi (PQgetvalue (res, 0, 0)); */

/*   if (products != NULL) */
/*     { */
/*       do { */

/*         IngresarProducto (products->product, id); */

/*         IngresarDetalleDocumento (products->product, id, */
/*                                   -1, */
/*                                   -1); */

/*         products = products->next; */
/*       } */
/*       while (products != compra->header_compra); */

/*       //      SetProductosIngresados (); */

/*     } */

/*   rut_proveedor = GetDataByOne (g_strdup_printf ("SELECT rut_proveedor FROM compras WHERE id=%d", */
/*                                                  id)); */


/*   doc = IngresarFactura ("-1", id, rut_proveedor, -1, "25", "3", "06", 0); */


/*   CompraIngresada (); */

/* } */

int main (int argc, char **argv)
{
  GKeyFile *key_file;

  FILE *fp;
  char *line = NULL;

  char *barcode;
  double cant = 100.0;
  double pcomp = 0.0;
  int precio = 1000;
  int margen = 20;
  char * pEnd;

  fp = fopen (argv[1], "r");

  compra = (Compra *) g_malloc (sizeof (Compra));
  compra->header = NULL;
  compra->products_list = NULL;
  compra->header_compra = NULL;
  compra->products_compra = NULL;
  compra->current = NULL;

  key_file = rizoma_open_config();

  if (key_file == NULL)
    {
      g_error ("Cannot open config file\n");
      return -1;
    }

  rizoma_set_profile ("DEFAULT");

  if (!DataExist ("SELECT rut FROM proveedor WHERE rut=99999999"))
    AddProveedorToDB ("99999999-9", "Inventario", "Portugal 201", "Santiago", "Santiago", "", "", "", "Victor", "Inventario");

  do {

    line = get_line (fp);

    if (line != NULL)
      {
        barcode = strtok (line, ",");

        pcomp = strtod (strtok (NULL, ","), &pEnd);

        precio = atoi (strtok (NULL, ","));

        cant = strtod (strtok (NULL, ","), &pEnd);

        if ((DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE barcode=%s", barcode))) == TRUE)
          CompraAgregarALista (barcode, cant, precio, pcomp, margen, FALSE);
        else
          printf ("El producto %s no esta en la bd\n", barcode);
      }

  } while (line != NULL);

  AgregarCompra ("99999999", "", 1);

  //IngresarFakeCompra ();

  fclose (fp);

  return 0;
}
