/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
       c-indentation-style: gnu -*- */
/*facutra_more.c
*
*    Copyright (C) 2005 Rizoma Tecnologia Limitada <info@rizoma.cl>
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
#define _XOPEN_SOURCE 600
#include<features.h>

#include<gtk/gtk.h>
#include<libps/pslib.h>

#include<stdlib.h>
#include<locale.h>
#include<string.h>
#include<math.h>

#include"tipos.h"
#include"config_file.h"
#include"factura_more.h"
#include"postgres-functions.h"
#include"boleta.h"

/**
 * Generate the neccesary invoice postscript files to be printed.
 *
 * @param sell_type the type of sell, at the momment must be always FACTURA
 * @param rut the rut of the client
 * @param total the total amount of the sale (not neccesary the total
 * amount of the invoice
 * @param num the num of the invoice (currently not being used)
 * @param products the list of products
 *
 * @return 0 if the invoices could be generated and printed
 */
gint
PrintDocument (gint sell_type, gchar *rut, gint total, gint num, Productos *products)
{
  //  GnomePrintJob *job;
  gchar *client, *address, *giro, *comuna, *fono;
  gchar *file_to_print = NULL;
  gchar *aux_rut;
  PGresult *res;
  gchar *q;
  Productos *header;
  GList *aux_list;
  gint max_lines = rizoma_get_value_int ("FACTURA_LINEAS");
  gint fact_num = num;
  gint i;
  gchar *print_command;

  aux_rut = g_strdup(rut);
  print_command = rizoma_get_value ("PRINT_COMMAND");

  q = g_strdup_printf("SELECT nombre || ' ' || apell_p || ' ' || apell_m AS name, "
		      "direccion, giro, comuna, telefono FROM cliente where rut=%s",
		      strtok(aux_rut,"-"));
  res = EjecutarSQL(q);
  g_free (q);
  client = PQvaluebycol(res, 0, "name");
  address = PQvaluebycol(res, 0, "direccion");
  giro = PQvaluebycol(res, 0, "giro");
  comuna = PQvaluebycol(res, 0, "comuna");
  fono = PQvaluebycol(res, 0, "telefono");

  header = products;
  do
    {
      aux_list = NULL;
      i = 0;
      do {
	aux_list = g_list_append(aux_list, products->product);
	products = products->next;
	i++;
      } while ((i < max_lines) && (products != header));

      if (sell_type == FACTURA)
	{
	  fact_num = get_ticket_number (FACTURA);
	  file_to_print = PrintFactura(client, rut, address, giro, comuna, fono, aux_list, fact_num);
	  set_ticket_number (fact_num, FACTURA);
	  system (g_strdup_printf ("%s %s", print_command, file_to_print));
	}
      else
	g_printerr ("%s: calling without the proper sell_type\n", G_STRFUNC);

    } while (products != header);
  return 0;
}

/**
 * Generate the information of a factura
 *
 * @param client the name of the client
 * @param rut the rut of the client
 * @param address the address of the client
 * @param giro the 'giro' of the client
 * @param comuna the comuna of the client
 * @param fono the contact phone of the client
 * @param products the list of products sold
 * @param num the number of the factura
 *
 * @return the path of the postscrip file generated to be printed
 */
gchar *
PrintFactura (gchar *client, gchar *rut, gchar *address, gchar *giro, gchar *comuna, gchar *fono,
	      GList *products, gint num)
{
  gchar *filename;
  PSDoc *psdoc;
  gint psfont;
  gfloat initial;
  gdouble precio, subtotal = 0, iva = 0, total;
  gint pepe;
  gchar *temp_directory = rizoma_get_value ("TEMP_FILES");
  gchar *factura_prefix = rizoma_get_value ("FACTURA_FILE_PREFIX");
  gchar *str_aux;
  gchar *font;
  gdouble *fact_address = NULL;
  gdouble *fact_giro = NULL;
  gdouble *fact_comuna = NULL;
  gdouble *fact_phone = NULL;
  gdouble *fact_detail = NULL;
  gdouble *fact_code = NULL;
  gdouble *fact_desc = NULL;
  gdouble *fact_cant = NULL;
  gdouble *fact_uni = NULL;
  gdouble *fact_total = NULL;
  gdouble *fact_iva = NULL;
  gdouble *fact_total_final = NULL;
  gdouble *fact_size = NULL;
  gdouble *fact_client = NULL;
  gdouble *fact_rut = NULL;
  gdouble *fact_subtotal = NULL;
  gdouble current_iva;
  Producto *product;
  gint i;

  current_iva = g_ascii_strtod(GetDataByOne("select monto from impuesto where id = 0"), NULL)/100 + 1.0;

  //Productos *header = products;

  filename = g_strdup_printf ("%s/%s%d.ps", temp_directory, factura_prefix, num);

  setlocale (LC_NUMERIC, "C");

  PS_boot ();

  psdoc = PS_new ();

  PS_open_file (psdoc, filename);

  /* rizoma_extract_xy (rizoma_get_value ("FACTURA_SIZE"), &x, &y); */
  fact_size = rizoma_get_double_list("FACTURA_SIZE", 2);
  g_assert(fact_size != NULL);

  str_aux = g_strdup_printf ("0 0 %f %f", fact_size[0], fact_size[1]);
  PS_set_info (psdoc, "BoundingBox", str_aux);
  g_free(str_aux);

  font = g_strconcat(rizoma_get_value("FONT_METRICS"), rizoma_get_value("FONT"), NULL);
  psfont = PS_findfont (psdoc, font, "", 0);

  if (psfont == 0)
    {
      g_printerr("%s: the font could not be found", G_STRFUNC);
      return NULL;
    }

  PS_begin_page (psdoc, fact_size[0], fact_size[1]);
  PS_setfont (psdoc, psfont, 10);

  fact_client = rizoma_get_double_list ("FACTURA_CLIENTE", 2);
  g_assert (fact_client != NULL);
  PS_show_xy (psdoc, client, fact_client[0], fact_client[1]);

  fact_rut = rizoma_get_double_list ("FACTURA_RUT", 2);
  g_assert (fact_rut != NULL);
  PS_show_xy (psdoc, rut, fact_rut[0], fact_rut[1]);

  fact_address = rizoma_get_double_list ("FACTURA_ADDRESS", 2);
  g_assert (fact_address != NULL);
  PS_show_xy (psdoc, address, fact_address[0], fact_address[1]);

  fact_giro = rizoma_get_double_list ("FACTURA_GIRO", 2);
  g_assert (fact_giro != NULL);
  PS_show_xy (psdoc, giro, fact_giro[0], fact_giro[1]);

  fact_comuna = rizoma_get_double_list("FACTURA_COMUNA", 2);
  g_assert (fact_comuna != NULL);
  PS_show_xy (psdoc, comuna, fact_comuna[0], fact_comuna[1]);

  fact_phone = rizoma_get_double_list("FACTURA_PHONE", 2);
  g_assert (fact_phone != NULL);
  PS_show_xy (psdoc, fono, fact_phone[0], fact_phone[1]);

  /* We draw the products detail */
  fact_detail = rizoma_get_double_list ("FACTURA_DETAIL", 2);
  g_assert (fact_detail != NULL);
  initial = fact_detail[1];

  fact_code = rizoma_get_double_list ("FACTURA_CODIGO", 2);
  fact_desc = rizoma_get_double_list ("FACTURA_DESCRIPCION", 2);
  fact_cant = rizoma_get_double_list ("FACTURA_CANT", 2);
  fact_uni = rizoma_get_double_list ("FACTURA_UNI", 2);
  fact_total = rizoma_get_double_list ("FACTURA_TOTAL", 2);

  for (i = 0 ; i < g_list_length(products) ; i++)
    {
      product = g_list_nth_data (products, i);

      PS_show_xy (psdoc, product->codigo, fact_code[0], initial);

      PS_show_xy (psdoc, product->producto, fact_desc[0], initial);

      str_aux = g_strdup_printf ("%.3f", product->cantidad);
      PS_show_xy (psdoc, str_aux, fact_cant[0], initial);
      g_free (str_aux);

      //check if must be used the mayorist price or not
      if ((product->cantidad < product->cantidad_mayorista) || (product->mayorista == FALSE))
	precio = product->precio;              ///(((gdouble)products->product->iva/100)+1);
      else
	precio = product->precio_mayor;        ///(((gdouble)products->product->iva/100)+1);

      str_aux = g_strdup_printf ("%.0f", precio);
      PS_show_xy (psdoc, str_aux, fact_uni[0], initial);
      g_free (str_aux);

      str_aux = g_strdup_printf ("%.0f", product->cantidad * precio);
      PS_show_xy (psdoc, str_aux, fact_total[0], initial);
      g_free (str_aux);

      pepe = lround ((gdouble)precio/current_iva);


      subtotal += pepe * product->cantidad;

      iva += (precio - pepe) * product->cantidad;

      initial -= 10;
    }

  /* Finished the products detail*/

  fact_subtotal = rizoma_get_double_list ("FACTURA_SUBTOTAL", 2);
  g_assert (fact_subtotal != NULL);
  str_aux = g_strdup_printf ("%.0f", subtotal);
  PS_show_xy (psdoc, str_aux, fact_subtotal[0], fact_subtotal[1]);

  total = subtotal + iva;

  fact_iva = rizoma_get_double_list ("FACTURA_IVA", 2);
  PS_show_xy (psdoc, g_strdup_printf ("%.0f", iva), fact_iva[0], fact_iva[1]);

  fact_total_final = rizoma_get_double_list ("FACTURA_TOTAL_FINAL", 2);
  g_assert (fact_total_final != NULL);
  str_aux = g_strdup_printf ("%.0f", total);
  PS_show_xy (psdoc, str_aux, fact_total_final[0], fact_total_final[1]);
  g_free (str_aux);

  PS_end_page (psdoc);
  PS_delete (psdoc);
  PS_shutdown ();

  g_printerr("factura: %s\n", filename);
  return filename;
}
