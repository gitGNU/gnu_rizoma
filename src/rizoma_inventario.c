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

int
main (int argc, char **argv)
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

  if (argc < 2)
    {
      g_printerr ("You have some parameters missing. The first argument needs to be a file and the seacond a profile from the ~/.rizoma file\n");
      return -1;
    }

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

  rizoma_set_profile (argv[2]);

  if (!DataExist ("SELECT rut FROM proveedor WHERE rut=99999999"))
    AddProveedorToDB ("99999999-9", "Inventario", "", "", "", "0", "", "", "Inventario", "Inventario");

  do {

    line = get_line (fp);

    if (line != NULL)
      {
        barcode = strtok (line, ",");

        pcomp = g_ascii_strtod (strtok (NULL, ","), &pEnd);

        precio = atoi (strtok (NULL, ","));

        cant = g_ascii_strtod (strtok (NULL, ","), &pEnd);

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
