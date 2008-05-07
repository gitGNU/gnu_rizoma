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

#include<locale.h>
#include<string.h>
#include<math.h>

#include"tipos.h"
#include"config_file.h"
#include"factura_more.h"
#include"postgres-functions.h"

gint
PrintDocument (gint sell_type, gchar *rut, gint total, gint num, Productos *products)
{
  //  GnomePrintJob *job;
  gchar *client, *address, *giro, *comuna, *fono;
  gchar *file_to_print = NULL;
  gchar *aux_rut;
  PGresult *res;
  gchar *q;
  aux_rut = g_strdup(rut);

  q = g_strdup_printf("SELECT nombre || ' ' || apellido_paterno || ' ' || apellido_materno AS name, "
		      "direccion, giro, comuna, telefono FROM cliente where rut=%s",
		      strtok(aux_rut,"-"));
  res = EjecutarSQL(q);
  g_free (q);
  client = PQvaluebycol(res, 0, "name");
  address = PQvaluebycol(res, 0, "direccion");
  giro = PQvaluebycol(res, 0, "giro");
  comuna = PQvaluebycol(res, 0, "comuna");
  fono = PQvaluebycol(res, 0, "telefono");


  if (sell_type == FACTURA)
      file_to_print = PrintFactura(client, rut, address, giro, comuna, fono, products, num);
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

/**
 * Generate the information of a factura
 *
 * @param client the name of the client
 * @param rut the rut of the client
 * @param address the address of the client
 * @param giro the 'giro' of the client
 * @param comuna 
 * @param fono the contact phone of the client
 * @param products the list of products sold
 * @param num the number of the factura
 *
 * @return the path of the postscrip file generated to be printed
 */
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
  gchar *str_aux;
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

  psfont = PS_findfont (psdoc, "Helvetica", "builtin", 0);

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

  while (products != NULL)
    {
      PS_show_xy (psdoc, products->product->codigo, fact_code[0], initial);

      PS_show_xy (psdoc, products->product->producto, fact_desc[0], initial);

      str_aux = g_strdup_printf ("%.3f", products->product->cantidad);
      PS_show_xy (psdoc, str_aux, fact_cant[0], initial);
      g_free (str_aux);

      //check if must be used the mayorist price or not
      if (products->product->cantidad < products->product->cantidad_mayorista ||
	  products->product->mayorista == FALSE)
	precio = products->product->precio;///(((gdouble)products->product->iva/100)+1);
      else
	precio = products->product->precio_mayor;///(((gdouble)products->product->iva/100)+1);

      str_aux = g_strdup_printf ("%.0f", precio);
      PS_show_xy (psdoc, str_aux, fact_uni[0], initial);
      g_free (str_aux);

      str_aux = g_strdup_printf ("%.0f", products->product->cantidad * precio);
      PS_show_xy (psdoc, str_aux, fact_total[0], initial);
      g_free (str_aux);

      pepe = lround ((gdouble)precio/current_iva);


      subtotal += pepe * products->product->cantidad;

      iva += (precio - pepe) * products->product->cantidad;

      initial -= 10;

      products = products->next;

    };

  /* Finished the products detail*/

  fact_subtotal = rizoma_get_double_list ("FACTURA_SUTOTAL", 2);
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

  g_print("factura: %s", filename);
  return filename;
}
