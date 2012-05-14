/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
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
#include<stdlib.h>

#include"tipos.h"
#include"config_file.h"
#include"utils.h"
#include"postgres-functions.h"
#include"boleta.h"
#include"manejo_productos.h"

void
PrintVale (Productos *header, gint venta_id, gint boleta, gint total, gint tipo_pago, gint tipo_documento)
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
  gint id_documento;
  gdouble siva = 0.0, civa = 0.0;
  gboolean hay_selectivo = FALSE;
  gboolean impresora = rizoma_get_value_boolean ("IMPRESORA");
  gboolean is_imp1, is_imp2;

  if (impresora == FALSE)
    return;

  if (tipo_pago == MIXTO)
    {
      is_imp1 = (pago_mixto->tipo_pago1 == CHEQUE_RESTAURANT) ? FALSE : TRUE;
      is_imp2 = (pago_mixto->tipo_pago2 == CHEQUE_RESTAURANT) ? FALSE : TRUE;
    }

  id_documento = InsertNewDocument (venta_id, tipo_documento, tipo_pago);

  if (boleta != -1)
    set_ticket_number (boleta, tipo_documento);

  fp = fopen (vale_file, "w+");
  fprintf (fp, "%s", start);
  fprintf (fp, "\t CONTROL INTERNO \n");
  fprintf (fp, "Fecha: %s Hora: %s\n", CurrentDate(0), CurrentTime());
  fprintf (fp, "Num. venta: %d - Num. boleta: %d\n", venta_id, boleta);
  fprintf (fp, "Vendedor: %s\n", user_data->user);
  fprintf (fp, "==========================================\n\n");

  do {
    if (products->product->cantidad_mayorista > 0 && products->product->precio_mayor > 0 && 
	products->product->cantidad >= products->product->cantidad_mayorista && products->product->mayorista == TRUE)      
      precio = products->product->precio_mayor;
    else
      precio = products->product->precio;

    gchar *vale_selectivo = rizoma_get_value ("VALE_SELECTIVO");
    if ((vale_selectivo != NULL) && (g_str_equal(vale_selectivo, "YES")))
      {
	if (g_str_has_suffix(products->product->producto,"@"))
	  {
	    hay_selectivo = TRUE;
	    
	    fprintf (fp, "%s %s\nCant.: %.2f $%7s  $%7s\n",
		     g_strndup (products->product->producto, 30),
		     products->product->marca,
		     products->product->cantidad,
		     PutPoints (g_strdup_printf ("%d",precio)),
		     PutPoints (g_strdup_printf ("%lu",
						 lround ((double)(products->product->cantidad * precio)))));
	    if (products->product->iva != 0)
	      civa += (double)(products->product->cantidad * precio);
	    else if (products->product->iva == 0)
	      siva += (double)(products->product->cantidad * precio);
	  }
      }
    else
      {
	fprintf (fp, "%s %s\nCant.: %.2f $%7s  $%7s\n",
		 g_strndup (products->product->producto, 30),
		 products->product->marca,
		 products->product->cantidad,
		 PutPoints (g_strdup_printf ("%d",precio)),
		 PutPoints (g_strdup_printf ("%lu",
					     lround ((double)(products->product->cantidad * precio)))));

	if (products->product->iva != 0)
	  civa += (double)(products->product->cantidad * precio);
	else if (products->product->iva == 0)
	  siva += (double)(products->product->cantidad * precio);
      }

    //Guarda el detalle del documento emitido
    InsertNewDocumentDetail (id_documento, products->product->barcode, precio, products->product->cantidad);

    products = products->next;
  } while (products != header);

  gchar *vale_selectivo = rizoma_get_value ("VALE_SELECTIVO");
  //impresora = rizoma_get_value_boolean ("IMPRESORA");
  if (((vale_selectivo != NULL) && (g_str_equal(vale_selectivo, "YES")) && hay_selectivo) || impresora == TRUE)
    {
      //Si el pago es con cheque de restaurant, o ambos son mapgos no afectos a impuestos
      if (tipo_pago == CHEQUE_RESTAURANT ||
	  (tipo_pago == MIXTO && (is_imp1 == FALSE && is_imp2 == FALSE)))
	{
	  siva = total;
	  civa = 0;
	}
      else if (tipo_pago == MIXTO && ( (is_imp1 == TRUE && is_imp2 == FALSE) ||
				       (is_imp1 == FALSE && is_imp2 == TRUE) ))
	{
	  //Los productos no afectos a impuestos + la proporcion no afeta a impuestos de todos los productos afectos
	  siva = CalcularSoloNoAfecto (products) + CalcularTotalProporcionNoAfecta (products);
	  printf ("\nsiva: %ld, solo no afecto: %ld, total prop no afecta: %ld\n", 
		  lround (siva),
		  lround (CalcularSoloNoAfecto (products)),
		  lround (CalcularTotalProporcionNoAfecta (products)));
	  //El total afecto a impuestos de la proporcion de todos los productos afecto
	  civa = CalcularTotalProporcionAfecta (products);
	  printf ("\nciva: %ld\n", lround (civa));
	}

      fprintf (fp, "\n");
      fprintf (fp, "Sub Total no afecto:  $%7s\n", PutPoints (g_strdup_printf ("%lu",lround(siva))));
      fprintf (fp, "Sub Total afecto:     %s$%7s %s\n", size2, PutPoints (g_strdup_printf ("%lu",lround(civa))), size1);
      fprintf (fp, "\n\n");
      gdouble totalLocal = civa + siva;
      fprintf (fp, "Total Venta:          %s$%7s %s\n", size2, PutPoints (g_strdup_printf ("%lu",lround(totalLocal))), size1);
      fprintf (fp, "\n\n\tGracias por su compra! \n");
      fprintf (fp, "\n\n\n");
      fprintf (fp, "%s", cut); /* We cut the paper :) */
      fclose (fp);
    }

  if (((vale_selectivo != NULL) && (g_str_equal (vale_selectivo, "YES")) && hay_selectivo) || 
      (impresora == TRUE && !g_str_equal (vale_selectivo, "YES")))
    for (i = 0; i < n_copy; i++) 
      system(g_strdup_printf ("%s %s", print_command, vale_file));

  system (g_strdup_printf ("rm %s", vale_file));
}


void
PrintValeContinuo (Productos *header, gint venta_id, gint boleta, 
		   gint total, gint tipo_pago, gint tipo_documento, Productos *prod)
{
  FILE *fp;
  char *vale_dir = rizoma_get_value ("VALE_DIR");
  char *vale_copy = rizoma_get_value ("VALE_COPY");
  char *print_command = rizoma_get_value ("PRINT_COMMAND");
  gchar *vale_file = g_strdup_printf ("%s/Vale%d.txt", vale_dir, venta_id);
  char start[] = {0x1B, 0x40, 0x0};
  //char cut[] = {0x1B, 0x69, 0x0};
  char size2[] = {0x1B, 0x45, 0x1, 0x0};
  char size1[] = {0x1B, 0x45, 0x0, 0x0};
  //char pageSize[] = {0x1B, 0x43, 0x06, 0x0};
  //char pageSize2[] = {0x1B, 0x43, 0x30, 0x6,0x0};
  //char salto[] = {0x1B, 0x61, 20, 0x0};
  char f[] = {0x0A, 0x0};
  //char abrirCaja[] = {0x1B, 0x70, 0x0};
  //char abrirCaja2[] = {0x1B, 0x70, 0x0, 0x64, 0x64, 0x0};
  int n_copy = atoi (vale_copy);
  gint i, precio, cantProd = 1;
  gint id_documento;
  gdouble siva = 0.0, civa = 0.0;
  gdouble prop_afecta = 0.0, prop_no_afecta = 0.0;
  gboolean hay_selectivo = FALSE;
  gboolean impresora = rizoma_get_value_boolean ("IMPRESORA");
  gboolean continuar = TRUE;
  gboolean is_imp1, is_imp2;
  
  gint pph = 5; // Productos Por Hoja
  Productos *products;
  
  if (prod != NULL && prod == header)
    return;

  if (prod == NULL)
    products = header;
  else
    products = prod;

  if (impresora == FALSE)
    return;

  if (tipo_pago == MIXTO)
    {
      is_imp1 = (pago_mixto->tipo_pago1 == CHEQUE_RESTAURANT) ? FALSE : TRUE;
      is_imp2 = (pago_mixto->tipo_pago2 == CHEQUE_RESTAURANT) ? FALSE : TRUE;
    }

  id_documento = InsertNewDocument (venta_id, tipo_documento, tipo_pago);

  if (boleta != -1)
    set_ticket_number (boleta, tipo_documento);  

  fp = fopen (vale_file, "w+");
  //fprintf (fp, "\t CONTROL INTERNO \n");
  fprintf (fp, "%s", start);
  //fprintf (fp, "%s", pageSize);
  fprintf (fp, "%s%s%s%s",f,f,f,f);
  fprintf (fp, "Fecha: %s Hora: %s\n", CurrentDate(0), CurrentTime());
  fprintf (fp, "Num. venta: %d - Num. boleta: %d\n", venta_id, boleta);
  fprintf (fp, "Vendedor: %s\n", user_data->user);
  //fprintf (fp, "========================================\n\n");
  fprintf (fp, "\n");

  while ((products != header && continuar) || (cantProd != pph && continuar)) 
    {
      if (products->product->cantidad_mayorista > 0 && products->product->precio_mayor > 0 && 
	  products->product->cantidad >= products->product->cantidad_mayorista && products->product->mayorista == TRUE)      
	precio = products->product->precio_mayor;   
      else
	precio = products->product->precio;

      gchar *vale_selectivo = rizoma_get_value ("VALE_SELECTIVO");
      if ((vale_selectivo != NULL) && (g_str_equal(vale_selectivo, "YES")))
	{
	  if (g_str_has_suffix(products->product->producto,"@"))
	    {
	      hay_selectivo = TRUE;
	    
	      fprintf (fp, "%s %s\nCant.: %.2f $%7s\t$%7s\n",
		       g_strndup (products->product->producto, 30),
		       products->product->marca,
		       products->product->cantidad,
		       PutPoints (g_strdup_printf ("%d",precio)),
		       PutPoints (g_strdup_printf ("%lu",
						   lround ((double)(products->product->cantidad * precio)))));
	      if (products->product->iva != 0)
		{
		  civa += (double)(products->product->cantidad * precio);
		  if (tipo_pago == MIXTO && ( (is_imp1 == TRUE && is_imp2 == FALSE) ||
					      (is_imp1 == FALSE && is_imp2 == TRUE) ))
		    {
		      prop_afecta += products->product->proporcion_afecta_imp;
		      prop_no_afecta += products->product->proporcion_no_afecta_imp;
		    }
		}
	      else if (products->product->iva == 0)
		siva += (double)(products->product->cantidad * precio);	      
	    }
	}
      else
	{
	  fprintf (fp, "%s %s\nCant.: %.2f $%7s\t$%7s\n",
		   g_strndup (products->product->producto, 30),
		   products->product->marca,
		   products->product->cantidad,
		   PutPoints (g_strdup_printf ("%d",precio)),
		   PutPoints (g_strdup_printf ("%lu",
					       lround ((double)(products->product->cantidad * precio)))));

	  if (products->product->iva != 0)
	    {
	      civa += (double)(products->product->cantidad * precio);
	      if (tipo_pago == MIXTO && ( (is_imp1 == TRUE && is_imp2 == FALSE) ||
					  (is_imp1 == FALSE && is_imp2 == TRUE) ))
		{
		  prop_afecta += products->product->proporcion_afecta_imp;
		  prop_no_afecta += products->product->proporcion_no_afecta_imp;
		}
	    }
	  else if (products->product->iva == 0)
	    siva += (double)(products->product->cantidad * precio);
	}
      
      //Guarda el detalle del documento emitido
      InsertNewDocumentDetail (id_documento, products->product->barcode, precio, products->product->cantidad);

      //Si es el último producto
      if (prod != NULL && prod == header)
	products = products->back;

      products = products->next;
    
      if (cantProd < pph)
	{
	  cantProd++;
	  continuar = (products == header) ? FALSE : TRUE;
	}
      else
	{
	  cantProd++;
	  continuar = FALSE;
	}
    }

  //fprintf (fp, "\n\n");

  gchar *vale_selectivo = rizoma_get_value ("VALE_SELECTIVO");
  if (((vale_selectivo != NULL) && (g_str_equal(vale_selectivo, "YES")) && hay_selectivo) || impresora == TRUE)
    {
      //Si el pago es con cheque de restaurant, o ambos son mapgos no afectos a impuestos
      if (tipo_pago == CHEQUE_RESTAURANT ||
	  (tipo_pago == MIXTO && (is_imp1 == FALSE && is_imp2 == FALSE)))
	{
	  siva = total;
	  civa = 0;
	}
      else if (tipo_pago == MIXTO && ( (is_imp1 == TRUE && is_imp2 == FALSE) ||
				       (is_imp1 == FALSE && is_imp2 == TRUE) ))
	{ 
	  //Los productos no afectos a impuestos + la proporcion no afeta a impuestos de todos los productos afectos
	  siva = siva + prop_no_afecta;
	  //El total afecto a impuestos de la proporcion de todos los productos afecto
	  civa = prop_afecta;
	}

      fprintf (fp, "\nSub Total no afecto: \t$%7s\n", PutPoints (g_strdup_printf ("%lu",lround(siva))));
      fprintf (fp, "Sub Total afecto:      \t%s$%7s %s\n", size2, PutPoints (g_strdup_printf ("%lu",lround(civa))), size1);
      fprintf (fp, "\n\n");
      gdouble totalLocal = civa + siva;
      fprintf (fp, "Total Venta: \t\t%s$%7s %s\n", size2, PutPoints (g_strdup_printf ("%lu",lround(totalLocal))), size1);
      fprintf (fp, "\n\n\tGracias por su compra! \n");
      fprintf (fp, "\n\n");
      
      //fprintf (fp, "%s", cut); /* We cut the paper :) */
      //fprintf (fp, "%s", salto);
      
      if ((cantProd-1) == 1)
	fprintf (fp, "%s%s%s%s%s%s%s%s%s%s%s%s%s",
		 f,f,f,f,f,f,f,f,f,f,f,f,f);
      else if ((cantProd-1) == 2)
	fprintf (fp, "%s%s%s%s%s%s%s%s%s%s%s",
		 f,f,f,f,f,f,f,f,f,f,f);
      else if ((cantProd-1) == 3)
	fprintf (fp, "%s%s%s%s%s%s%s%s%s",
		 f,f,f,f,f,f,f,f,f);
      else if ((cantProd-1) == 4)
	fprintf (fp, "%s%s%s%s%s%s%s",
		 f,f,f,f,f,f,f);
      else if ((cantProd-1) == 5)
	fprintf (fp, "%s%s%s%s%s",
		 f,f,f,f,f);

      //fprintf (fp, "%s", abrirCaja);
      //fprintf (fp, "%s", abrirCaja2);
      fclose (fp);
    }

  if (((vale_selectivo != NULL) && (g_str_equal (vale_selectivo, "YES")) && hay_selectivo) || 
      (impresora == TRUE && !g_str_equal (vale_selectivo, "YES")))
    for (i = 0; i < n_copy; i++) 
      system(g_strdup_printf ("%s %s", print_command, vale_file));

  //system (g_strdup_printf ("cp %s /home/alumno/Escritorio/", vale_file));
  system (g_strdup_printf ("rm %s", vale_file));

  if (products != header)
    {
      boleta = get_ticket_number (tipo_documento);
      PrintValeContinuo (header, venta_id, boleta, total, tipo_pago, tipo_documento, products);
    }
}


void
print_cash_box_info (gint cash_id, gint monto_ingreso, gint monto_egreso, gchar *motivo)
{
  PGresult *res;
  gchar *query;
  gchar *total_ingreso;
  gchar *total_egreso;
  gchar *monto_caja;

  FILE *fp;
  char start[] = {0x1B, 0x40, 0x0};
  char cut[] = {0x1B, 0x69, 0x0};

  gboolean impresora = rizoma_get_value_boolean ("IMPRESORA");
  gboolean recibo_mov_caja = rizoma_get_value_boolean ("RECIBO_MOV_CAJA");
  char *vale_dir = rizoma_get_value ("VALE_DIR");
  char *print_command = rizoma_get_value ("PRINT_COMMAND");
  gchar *vale_file = g_strdup_printf ("%s/Vale%d.txt", vale_dir, cash_id);

  gchar *user_name = user_data->user;

  if (impresora == FALSE)
    return;
  if (recibo_mov_caja == FALSE)
    return;

  //Consulta información de la caja correspondiente al id
  if (monto_ingreso == 0 && monto_egreso == 0)
    {
      query = g_strdup_printf ("select to_char (open_date, 'DD-MM-YYYY HH24:MI') as open_date_formatted, "
			       "to_char (close_date, 'DD-MM-YYYY HH24:MI') as close_date_formatted, * from cash_box_report (%d)", cash_id);
      res = EjecutarSQL (query);
      g_free (query);

      //Obtiene el ingreso total
      total_ingreso = PutPoints (g_strdup_printf ("%d", atoi (PQvaluebycol (res, 0, "cash_sells"))
						  + atoi (PQvaluebycol (res, 0, "cash_income"))
						  + atoi (PQvaluebycol (res, 0, "cash_payed_money"))
						  + atoi (PQvaluebycol (res, 0, "cash_box_start"))
						  + atoi (PQvaluebycol (res, 0, "bottle_deposit"))
						  ));
      //Obtiene el egreso total
      total_egreso = PutPoints (g_strdup_printf ("%d", atoi (PQvaluebycol (res, 0, "cash_outcome"))
						 + atoi (PQvaluebycol (res, 0, "cash_loss_money"))
						 + atoi (PQvaluebycol (res, 0, "nullify_sell"))
						 + atoi (PQvaluebycol (res, 0, "current_expenses"))
						 + atoi (PQvaluebycol (res, 0, "bottle_return"))
						 ));

      //Obtiene el monto en caja
      monto_caja = PutPoints (g_strdup_printf ("%d", atoi (PQvaluebycol (res, 0, "cash_box_start"))
					       + atoi (PQvaluebycol (res, 0, "cash_sells"))
					       + atoi (PQvaluebycol (res, 0, "cash_payed_money"))
					       + atoi (PQvaluebycol (res, 0, "cash_income"))
					       + atoi (PQvaluebycol (res, 0, "bottle_deposit"))
					       - atoi (PQvaluebycol (res, 0, "cash_loss_money"))
					       - atoi (PQvaluebycol (res, 0, "cash_outcome"))
					       - atoi (PQvaluebycol (res, 0, "nullify_sell"))
					       - atoi (PQvaluebycol (res, 0, "current_expenses"))
					       - atoi (PQvaluebycol (res, 0, "bottle_return"))
					       ));
    }

  //genera comprobante caja
  fp = fopen (vale_file, "w+");
  fprintf (fp, "%s", start);
  fprintf (fp, "\t CONTROL INTERNO \n");
  fprintf (fp, "==========================================\n\n");
    
  if (monto_ingreso == 0 && monto_egreso == 0)
    {
      fprintf (fp, "  Nombre usuario: %s \n", user_name);
      fprintf (fp, "  ID caja: %d \n", cash_id);
      fprintf (fp, "  ------------------ \n");

      fprintf (fp, "  Fecha apertura: %s \n", PQvaluebycol (res, 0, "open_date_formatted"));
      fprintf (fp, "  Fecha cierre: %s \n\n", PQvaluebycol (res, 0, "close_date_formatted"));

      fprintf (fp, "  INGRESOS: \n");
      fprintf (fp, "  ------------------ \n");
      fprintf (fp, "  Inicio: %s \n", PutPoints (PQvaluebycol (res, 0, "cash_box_start")));
      fprintf (fp, "  Ventas Efectivo: %s \n", PutPoints (PQvaluebycol (res, 0, "cash_sells")));
      fprintf (fp, "  Abonos Credito: %s \n", PutPoints (PQvaluebycol (res, 0, "cash_payed_money")));
      fprintf (fp, "  Deposito Envase: %s \n", PutPoints (PQvaluebycol (res, 0, "bottle_deposit")));
      fprintf (fp, "  Ingresos Efectivo: %s \n", PutPoints (PQvaluebycol (res, 0, "cash_income")));
      fprintf (fp, "  Sub-total ingresos: %s \n\n", total_ingreso);

      fprintf (fp, "  EGRESOS: \n");
      fprintf (fp, "  ------------------ \n");
      fprintf (fp, "  Retiros: %s \n", PutPoints (PQvaluebycol (res, 0, "cash_outcome")));
      fprintf (fp, "  Ventas Anuladas: %s \n", PutPoints (PQvaluebycol (res, 0, "nullify_sell")));
      fprintf (fp, "  Gastos Corrientes: %s \n", PutPoints (PQvaluebycol (res, 0, "current_expenses")));
      fprintf (fp, "  Devolucion Envase: %s \n", PutPoints (PQvaluebycol (res, 0, "bottle_return")));
      fprintf (fp, "  Perdida: %s \n", PutPoints (PQvaluebycol (res, 0, "cash_loss_money")));
      fprintf (fp, "  Sub-total egresos: %s \n\n", total_egreso);

      fprintf (fp, "  Saldo en caja: %s \n", monto_caja);
    }
  else if (monto_ingreso > 0 && motivo != NULL)
    {
      fprintf (fp, "  INGRESO \n");
      fprintf (fp, "  ------------------ \n");
      fprintf (fp, "  Nombre usuario: %s \n", user_name);
      fprintf (fp, "  ID caja: %d \n", cash_id);
      fprintf (fp, "  ------------------ \n\n");

      fprintf (fp, "  Fecha: %s Hora: %s\n", CurrentDate(0), CurrentTime());
      fprintf (fp, "  Monto ingreso: %d \n", monto_ingreso);
      fprintf (fp, "  Motivo ingreso: %s \n", motivo);
    }
  else if (monto_egreso > 0 && motivo != NULL)
    {
      fprintf (fp, "  EGRESO \n");
      fprintf (fp, "  ------------------ \n");
      fprintf (fp, "  Nombre usuario: %s \n", user_name);
      fprintf (fp, "  ID caja: %d \n", cash_id);
      fprintf (fp, "  ------------------ \n\n");

      fprintf (fp, "  Fecha: %s Hora: %s\n", CurrentDate(0), CurrentTime());
      fprintf (fp, "  Monto egreso: %d \n", monto_egreso);
      fprintf (fp, "  Motivo egreso: %s \n", motivo);
    }

  fprintf (fp, "%s", cut); /* We cut the paper :) */
  fclose (fp);
  
  //imprime comprobante caja
  system (g_strdup_printf ("%s %s", print_command, vale_file));
  system (g_strdup_printf ("rm %s", vale_file));
}


void
PrintValeTraspaso (Productos *header, gint traspaso_id, gboolean traspaso_envio, gchar *origen, gchar *destino)
{
  Productos *products = header;
  FILE *fp;

  char *vale_dir = rizoma_get_value ("VALE_DIR");
  char *vale_copy = rizoma_get_value ("VALE_COPY");
  char *print_command = rizoma_get_value ("PRINT_COMMAND");
  gboolean impresora = rizoma_get_value_boolean ("IMPRESORA");
  gboolean recibo_traspaso = rizoma_get_value_boolean ("RECIBO_TRASPASO");

  gchar *vale_file = g_strdup_printf ("%s/Vale%d.txt", vale_dir, traspaso_id);
  gchar start[] = {0x1B, 0x40, 0x0};
  gchar cut[] = {0x1B, 0x69, 0x0};
  gchar size2[] = {0x1B, 0x45, 0x1, 0x0};
  gchar size1[] = {0x1B, 0x45, 0x0, 0x0};
  gint n_copy = atoi (vale_copy);

  gdouble total;
  gchar *tipo_traspaso;
  gint i;

  if (impresora == FALSE)
    return;
  if (recibo_traspaso == FALSE)
    return;

  if (traspaso_envio == TRUE)
    tipo_traspaso = g_strdup ("Envio");
  else
    tipo_traspaso = g_strdup ("Recibo");

  fp = fopen (vale_file, "w+");
  fprintf (fp, "%s", start);
  fprintf (fp, "\t CONTROL INTERNO \n");
  fprintf (fp, "\t TRASPASO DE MERCADERIA \n\n");
  fprintf (fp, "Fecha: %s Hora: %s\n", CurrentDate(0), CurrentTime());
  fprintf (fp, "Origen: %s\n", origen);
  fprintf (fp, "Destino: %s\n", destino);
  fprintf (fp, "Usuario: %s\n", user_data->user);
  fprintf (fp, "Id: %d - Tipo taspaso: %s\n", traspaso_id, tipo_traspaso);
  fprintf (fp, "==========================================\n\n");

  do {
    fprintf (fp, "%s %s\nCant.: %.2f $%7s  $%7s\n",
	     g_strndup (products->product->producto, 30),
	     products->product->marca,
	     products->product->cantidad,
	     PutPoints (g_strdup_printf ("%ld",lround (products->product->fifo))),
	     PutPoints (g_strdup_printf ("%ld",
					 lround ((double)(products->product->cantidad * products->product->fifo)))));

    total += (double)(products->product->cantidad * products->product->fifo);

    products = products->next;
  } while (products != header);

  //impresora = rizoma_get_value_boolean ("IMPRESORA");
  if (impresora == TRUE)
    {
      fprintf (fp, "\n");
      fprintf (fp, "Total Traspaso:       %s$%7s %s\n", size2, PutPoints (g_strdup_printf ("%lu",lround(total))), size1);
      fprintf (fp, "\n\n\n");
      fprintf (fp, "%s", cut); /* We cut the paper :) */
      fclose (fp);
    }

  for (i = 0; i < n_copy; i++) 
    system (g_strdup_printf ("%s %s", print_command, vale_file));

  system (g_strdup_printf ("rm %s", vale_file));
}


void
PrintValeCompra (Productos *header, gint compra_id, gint n_document, gchar *nombre_proveedor)
{
  Productos *products = header;
  FILE *fp;

  char *vale_dir = rizoma_get_value ("VALE_DIR");
  char *vale_copy = rizoma_get_value ("VALE_COPY");
  char *print_command = rizoma_get_value ("PRINT_COMMAND");
  gboolean impresora = rizoma_get_value_boolean ("IMPRESORA");
  gboolean recibo_compra = rizoma_get_value_boolean ("RECIBO_COMPRA");

  gchar *vale_file = g_strdup_printf ("%s/Vale%d.txt", vale_dir, compra_id);
  gchar start[] = {0x1B, 0x40, 0x0};
  gchar cut[] = {0x1B, 0x69, 0x0};
  gchar size2[] = {0x1B, 0x45, 0x1, 0x0};
  gchar size1[] = {0x1B, 0x45, 0x0, 0x0};
  gint n_copy = atoi (vale_copy);

  gdouble total;
  gint i;

  if (impresora == FALSE)
    return;
  if (recibo_compra == FALSE)
    return;

  fp = fopen (vale_file, "w+");
  fprintf (fp, "%s", start);
  fprintf (fp, "\t CONTROL INTERNO \n");
  fprintf (fp, "\t INGRESO DE MERCADERIA \n\n");
  fprintf (fp, "Fecha: %s Hora: %s\n", CurrentDate(0), CurrentTime());
  fprintf (fp, "Proveedor: %s\n", nombre_proveedor);
  fprintf (fp, "Usuario: %s\n", user_data->user);
  fprintf (fp, "Id Compra: %d - Num. Documento: %d\n", compra_id, n_document);
  fprintf (fp, "==========================================\n\n");

  do {
    fprintf (fp, "%s %s\nCant.: %.2f $%7s  $%7s\n",
	     g_strndup (products->product->producto, 30),
	     products->product->marca,
	     products->product->cantidad,
	     PutPoints (g_strdup_printf ("%ld",lround (products->product->fifo))),
	     PutPoints (g_strdup_printf ("%ld",
					 lround ((double)(products->product->cantidad * products->product->fifo)))));

    total += (double)(products->product->cantidad * products->product->fifo);

    products = products->next;
  } while (products != header);

  //impresora = rizoma_get_value_boolean ("IMPRESORA");
  if (impresora == TRUE)
    {
      fprintf (fp, "\n");
      fprintf (fp, "Total Ingresado:      %s$%7s %s\n", size2, PutPoints (g_strdup_printf ("%lu",lround(total))), size1);
      fprintf (fp, "\n\n\n");
      fprintf (fp, "%s", cut); /* We cut the paper :) */
      fclose (fp);
    }

  for (i = 0; i < n_copy; i++) 
    system (g_strdup_printf ("%s %s", print_command, vale_file));

  system (g_strdup_printf ("rm %s", vale_file));
}



/**
 * Esta función abre la gaveta
 */
void
abrirGaveta(void)
{
  // Abriendo la caja
  char *vale_dir = rizoma_get_value ("VALE_DIR");
  gchar *abrirGaveta = g_strdup_printf ("%s/abrirGaveta.sh", vale_dir);
  FILE *ac;
  ac = fopen (abrirGaveta, "w+");
  fprintf (ac, "echo \"300,3,8,1\" > %s", rizoma_get_value ("GAVETA_DEV"));
  fclose (ac);
  system (g_strdup_printf ("sh %s/abrirGaveta.sh", vale_dir));
}
