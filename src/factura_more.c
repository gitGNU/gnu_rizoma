/*facutra_more.c
*
*    Copyright (C) 2005 Rizoma Tecnologia Limitada <jonathan@rizoma.cl>
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

#include<gtk/gtk.h>
#include<libps/pslib.h>

#include<locale.h>

//#include<libgnomeprint/gnome-print.h>
//#include<libgnomeprint/gnome-print-job.h>

#include"tipos.h"

#include"config_file.h"

#include"factura_more.h"
#include"postgres-functions.h"

gint
PrintDocument (gint sell_type, gchar *rut, gint total, gint num, Productos *products)
{
  //  GnomePrintJob *job;
  gchar *client, *address, *giro, *comuna, *fono;
  gint subtotal, iva;
  
  gchar *file_to_print = NULL;
  
  client = GetDataByOne ("SELECT nombre || ' ' || apellido_paterno || ' ' || apellido_materno AS name FROM cliente");
  address = GetDataByOne ("SELECT direccion FROM cliente");
  giro = GetDataByOne ("SELECT giro FROM cliente");
  //  comuna = GetDataByOne ("SELECT co
  comuna = "Santiago";
  fono = GetDataByOne ("SELECT telefono FROM cliente");

  
  switch (sell_type)
    {
    case FACTURA:
      {
	file_to_print = PrintFactura 
	  (client, rut, address, giro, comuna, fono, products, num);
	break;
      }
      }
  
  /*  
  if (file_to_print != NULL)
    {
      job = gnome_print_job_new (NULL);
      
      gnome_print_job_set_file (job, file_to_print);
      
      gnome_print_job_print (job);
    }
  */
  return 0;
}

gchar *
PrintFactura (gchar *client, gchar *rut, gchar *address, gchar *giro, gchar *comuna, gchar *fono,
	      Productos *products, gint num)
{
  gchar *filename;
  PSDoc *psdoc;
  gint psfont;
  gfloat initial;
  gdouble precio, subtotal = 0, iva = 0, total;
  gint pepe;
  gchar *temp_directory = rizoma_get_value ("TEMP_FILES");
  gchar *factura_prefix = rizoma_get_value ("FACTURA_FILE_PREFIX");
  gfloat x, y;
  gfloat x_codigo, x_cant, x_desc, x_uni, x_total;

  Productos *header = products;

  filename = g_strdup_printf ("%s/%s%d.ps", temp_directory, factura_prefix, num);

  setlocale (LC_NUMERIC, "C");

  PS_boot ();

  psdoc = PS_new ();
  
  PS_open_file (psdoc, filename);

  printf ("%s\n", rizoma_get_value ("FACTURA_SIZE"));
  rizoma_extract_xy (rizoma_get_value ("FACTURA_SIZE"), &x, &y);
  PS_set_info (psdoc, "BoundingBox", 
	       g_strdup_printf ("0 0 %f %f", x, y));

  psfont = PS_findfont (psdoc, "Helvetica", "", 0);

  PS_begin_page (psdoc, x, y);
  PS_setfont (psdoc, psfont, 10);

  rizoma_extract_xy (rizoma_get_value ("FACTURA_CLIENTE"), &x, &y);
  PS_show_xy (psdoc, client, x, y);

  rizoma_extract_xy (rizoma_get_value ("FACTURA_RUT"), &x, &y);
  PS_show_xy (psdoc, rut, x, y);
  
  rizoma_extract_xy (rizoma_get_value ("FACTURA_ADDRESS"), &x, &y);
  PS_show_xy (psdoc, address, x, y);

  rizoma_extract_xy (rizoma_get_value ("FACTURA_GIRO"), &x, &y);
  PS_show_xy (psdoc, giro, x, y);
  
  //  PS_show_xy (psdoc, comuna, dimension->factura_giro.x, dimension->factura_giro.y);
  
  //PS_show_xy (psdoc, fono, dimension->factura_fono->xizoma, dimension->factura_fono->y);

  /* We draw the products detail */
  rizoma_extract_xy (rizoma_get_value ("FACTURA_DETAIL"), &x, &y);
  initial = y;

  rizoma_extract_xy (rizoma_get_value ("FACTURA_CODIGO"), &x_codigo, &y);
  rizoma_extract_xy (rizoma_get_value ("FACTURA_DESCRIPCION"), &x_desc, &y);
  rizoma_extract_xy (rizoma_get_value ("FACTURA_CANT"), &x_cant, &y);
  rizoma_extract_xy (rizoma_get_value ("FACTURA_UNI"), &x_uni, &y);
  rizoma_extract_xy (rizoma_get_value ("FACTURA_TOTAL"), &x_total, &y);
  
  do 
    {
      PS_show_xy (psdoc, products->product->codigo, x_codigo, initial);

      PS_show_xy (psdoc, products->product->producto, x_desc, initial);
      
      PS_show_xy (psdoc, g_strdup_printf ("%.3f", products->product->cantidad),
		  x_cant, initial);

      if (products->product->cantidad < products->product->cantidad_mayorista ||
	  products->product->mayorista == FALSE)
	precio = products->product->precio;///(((gdouble)products->product->iva/100)+1);
      else
	precio = products->product->precio_mayor;///(((gdouble)products->product->iva/100)+1);

      
      PS_show_xy (psdoc, g_strdup_printf ("%.0f", precio),
		  x_uni, initial);
      
      PS_show_xy (psdoc, g_strdup_printf ("%.0f", products->product->cantidad * precio),
		  x_total, initial);      
      
      pepe = lround ((gdouble)precio/1.19);
      
      
      subtotal += pepe * products->product->cantidad;

      iva += (precio - pepe) * products->product->cantidad;

      initial -= 10;

      products = products->next;
      
    } while (products != header);
  
  /* Finished the products detail*/

  rizoma_extract_xy (rizoma_get_value ("FACTURA_SUTOTAL"), &x, &y);
  PS_show_xy (psdoc, g_strdup_printf ("%.0f", subtotal), x, y);


  total = subtotal + iva;

  rizoma_extract_xy (rizoma_get_value ("FACTURA_IVA"), &x, &y);
  PS_show_xy (psdoc, g_strdup_printf ("%.0f", iva), x, y);

  rizoma_extract_xy (rizoma_get_value ("FACTURA_TOTAL_FINAL"), &x, &y);
  PS_show_xy (psdoc, g_strdup_printf ("%.0f", total), x, y);

  PS_end_page (psdoc);
  PS_delete (psdoc);
  PS_shutdown ();
  
  return filename;
}
