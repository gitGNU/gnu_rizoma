/*vale.c
*
*    Copyright (C) 2004 Rizoma Tecnologia Limitada <info@rizoma.cl>
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
*    You should have received a 1copy of the GNU General Public License
*    along with this program; if not, write to the Free Software
*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#define _XOPEN_SOURCE 600
#include<features.h>

#include<math.h>
#include"tipos.h"
#include"config_file.h"

#include"tiempo.h"

int
PrintVale (Productos *header, gint venta_id, gint total)
{
  Productos *products = header;
  FILE *fp;
  char *vale_dir = rizoma_get_value ("VALE_DIR");
  char *vale_copy = rizoma_get_value ("VALE_COPY");
  char *print_command = rizoma_get_value ("PRINT_COMMAND");
  gchar *vale_file = g_strdup_printf ("%s/Vale%d.txt", vale_dir, venta_id);
  char start[] = {0x1B, 0x40, 0x0};
  char cut[] = {0x1B, 0x69, 0x0};
  char size2[] = {0x1B, 0x45, 0x1, 0x0};
  char size1[] = {0x1B, 0x45, 0x0, 0x0};
  int n_copy = atoi (vale_copy);
  int i, precio;
  gdouble siva = 0.0, civa = 0.0;

  fp = fopen (vale_file, "w+");

  fprintf (fp, "%s", start);
  fprintf (fp, "Fecha: %s Hora: %s\n", CurrentDate(), CurrentTime());
  fprintf (fp, "Numero de venta: %d\n", venta_id);
  fprintf (fp, "Vendedor: %s\n", user_data->user);
  fprintf (fp, "==========================================\n\n");

  do {

    if (products->product->iva != 0)
      {
	if (products->product->mayorista == FALSE)
	  precio = products->product->precio;
	else
	  precio = products->product->precio_mayor;

	fprintf (fp, "%s %s\n\tCant.: %.2f $ %d \t$ %lu\n", g_strndup (products->product->producto, 30), products->product->marca,
		 products->product->cantidad, precio, lround ((double)(products->product->cantidad * precio)));

	civa += (double)(products->product->cantidad * precio);
      }

    products = products->next;

  } while (products != header);

  fprintf (fp, "\n\n");

  do {

    if (products->product->iva == 0)
      {
	if (products->product->mayorista == FALSE)
	  precio = products->product->precio;
	else
	  precio = products->product->precio_mayor;

	fprintf (fp, "%s %s\n\tCant.: %.2f $ %d \t$ %lu\n", g_strndup (products->product->producto, 30), products->product->marca,
		 products->product->cantidad, precio, lround ((double)(products->product->cantidad * precio)));

	siva+= (double)(products->product->cantidad * precio);
      }

    products = products->next;

  } while (products != header);

  fprintf (fp, "\nSub Total no afecto: \t\t$ %lu\n", lround(siva));
  fprintf (fp, "Sub Total afecto:      \t\t%s$ %u %s\n", size2, lround(civa), size1);
  fprintf (fp, "\n\n");
  fprintf (fp, "Total Venta: \t\t\t%s$ %d %s\n", size2, total, size1);
  fprintf (fp, "\n\n\n\n\n");
  fprintf (fp, "%s", cut); /* We cut the paper :) */
  fclose (fp);


  for (i = 0; i < n_copy; i++)
    system (g_strdup_printf ("%s %s", print_command, vale_file));

  system (g_strdup_printf ("rm %s", vale_file));
}
