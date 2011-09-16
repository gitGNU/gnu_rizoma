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
#include"manejo_productos.h"

void
PrintVale (Productos *header, gint venta_id, gint total, gint tipo_pago)
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
  gboolean hay_selectivo = FALSE;
  gboolean impresora = rizoma_get_value_boolean ("IMPRESORA");

  if (impresora == FALSE)
    return;

  fp = fopen (vale_file, "w+");
  fprintf (fp, "%s", start);
  fprintf (fp, "\t CONTROL INTERNO \n");
  fprintf (fp, "Fecha: %s Hora: %s\n", CurrentDate(0), CurrentTime());
  fprintf (fp, "Numero de venta: %d\n", venta_id);
  fprintf (fp, "Vendedor: %s\n", user_data->user);
  fprintf (fp, "==========================================\n\n");

  do {
    if (products->product->iva != 0)
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
	    
		fprintf (fp, "%s %s\n\tCant.: %.2f $ %d \t$ %lu\n",
			 g_strndup (products->product->producto, 30),
			 products->product->marca,
			 products->product->cantidad,
			 precio,
			 lround ((double)(products->product->cantidad * precio)));
	      }
	    civa += (double)(products->product->cantidad * precio);
	  }
	else
	  {
	    fprintf (fp, "%s %s\n\tCant.: %.2f $ %d \t$ %lu\n",
		     g_strndup (products->product->producto, 30),
		     products->product->marca,
		     products->product->cantidad,
		     precio,
		     lround ((double)(products->product->cantidad * precio)));

	    civa += (double)(products->product->cantidad * precio);
	  }
      }

    products = products->next;

  } while (products != header);

  fprintf (fp, "\n\n");

  do {
    if (products->product->iva == 0)
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
		
		fprintf (fp, "%s %s\n\tCant.: %.2f $ %d \t$ %lu\n",
			 g_strndup (products->product->producto, 30),
			 products->product->marca,
			 products->product->cantidad,
			 precio,
			 lround ((double)(products->product->cantidad * precio)));
	      }
	    siva += (double)(products->product->cantidad * precio);
	  }
	else
	  {
	    fprintf (fp, "%s %s\n\tCant.: %.2f $ %d \t$ %lu\n",
		     g_strndup (products->product->producto, 30),
		     products->product->marca,
		     products->product->cantidad,
		     precio,
		     lround ((double)(products->product->cantidad * precio)));

	    siva += (double)(products->product->cantidad * precio);
	  }
      }

    products = products->next;

  } while (products != header);

  gchar *vale_selectivo = rizoma_get_value ("VALE_SELECTIVO");
  //impresora = rizoma_get_value_boolean ("IMPRESORA");
  if (((vale_selectivo != NULL) && (g_str_equal(vale_selectivo, "YES")) && hay_selectivo) || impresora == TRUE)
    {
      gint diferencia;
      if (tipo_pago == CHEQUE_RESTAURANT)
	{
	  siva = total;
	  civa = 0;
	}
      else if (tipo_pago == MIXTO)
	{ 
	  //Los productos no afectos a impuestos + la proporcion no afeta a impuestos de todos los productos afectos
	  siva = lround (CalcularSoloNoAfecto (products)) + lround (CalcularTotalProporcionNoAfecta (products->product->proporcion_no_afecta_imp));
	  //El total afecto a impuestos de la proporcion de todos los productos afecto
	  civa = lround (CalcularTotalProporcionAfecta (products->product->proporcion_afecta_imp));
	}

      fprintf (fp, "\nSub Total no afecto: \t\t$ %lu\n", lround(siva));
      fprintf (fp, "Sub Total afecto:      \t\t%s$ %u %s\n", size2, lround(civa), size1);
      fprintf (fp, "\n\n");
      fprintf (fp, "Total Venta: \t\t\t%s$ %d %s\n", size2, total, size1);
      fprintf (fp, "\n\n\t\tGracias por su compra! \n");
      fprintf (fp, "\n\n\n\n\n");
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

  //Consulta informaciÃ³n de la caja correspondiente al id
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


/**
 * Esta funciÃ³n abre la gaveta
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
