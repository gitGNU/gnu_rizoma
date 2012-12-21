/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
/*postgres-functions.c
 *
 *    Copyright (C) 2004,2008 Rizoma Tecnologia Limitada <info@rizoma.cl>
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

#include<glib.h>

#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<locale.h>

#include"tipos.h"
#include"config_file.h"
#include"postgres-functions.h"
#include"boleta.h"
#include"vale.h"
#include"rizoma_errors.h"
#include"utils.h"
#include"factura_more.h"
#include"caja.h"
#include"manejo_productos.h"

PGconn *connection;
PGconn *connection2;

gchar *
CutComa (gchar *number)
{
  gchar *num = g_strdup (number);
  gint len = (int)strlen (num);

  while (len != -1)
    {
      if (num[len] == ',')
        {
          num[len]='.';
          break;
        }
      len--;
    }

  return num;
}

gchar *
PutComa (gchar *number)
{
  struct lconv *locale  = localeconv ();
  gchar *decimal_point = locale->decimal_point;

  gchar *num = g_strdup (number);
  gint len = (int)strlen (num);

  while (len != -1)
    {
      if (num[len] == '.' || num[len] == ',')
        {
          num[len] = *decimal_point;
          break;
        }
      len--;
    }

  return num;
}


/**
 * Elimina el separador '.' o ',' siempre y cuando no represente un
 * decimal.
 *
 * @param number : cifra numérica como string
 */
gchar *
DropDelimiter (gchar *number)
{
  struct lconv *locale  = localeconv ();
  gchar *no_decimal_point = locale->decimal_point;

  if (no_decimal_point[0] == '.')
    no_decimal_point = g_strdup(",");
  else
    no_decimal_point = g_strdup(".");

  gchar *num = g_strdup (number);
  gint len = (int)strlen (num);
  gint sublen, points=0;

  while (len != -1)
    {
      if (num[len] == no_decimal_point[0])
        {
	  points += points;
	  sublen = len;
	  while (sublen != 0)
	    {
	      num[sublen] = num[sublen-1];
	      sublen--;
	    }
        }
      len--;
    }

  while (points != 0)
    {
      num [points-1] = ' ';
      points--;
    }
  g_strstrip(num);
  return num;
}


gchar *
SpecialChar (gchar *string)
{
  gchar *find = strchr (string, '\'');

  if (find  != NULL)
    {
      int length = strlen (string) + 1;
      gchar *new_string = (gchar *) g_malloc0 (length);
      int i, o = 0;

      for (i = 0; i < length; i++, o++)
        {
          if (string[o] != '\'')
            new_string[i] = string[o];
          else
            {
              new_string[i] = string[o];
              i++;
              new_string[i] = '\'';
            }
        }

      return new_string;
    }
  else
    return string;
}

PGresult *
EjecutarSQL (gchar *sentencia)
{
  PGresult *res;
  ConnStatusType status;
  ExecStatusType status_res;

  char *host = rizoma_get_value ("SERVER_HOST");
  char *name = rizoma_get_value ("DB_NAME");
  char *user = rizoma_get_value ("USER");
  char *pass = rizoma_get_value ("PASSWORD");
  char *port = rizoma_get_value ("PORT");

  if (port == NULL)
    port = g_strdup("5432");

  char *sslmode = rizoma_get_value ("SSLMODE");
  if (sslmode == NULL)
    sslmode = g_strdup("require");

  status = PQstatus( connection );

  if (status == CONNECTION_OK)
    {
      res = PQexec (connection, sentencia);
      status_res = PQresultStatus(res);

      if ((status_res != PGRES_COMMAND_OK) && (status_res != PGRES_TUPLES_OK))
        g_printerr("SQL: %s\nErr: %s\nMsg: %s",
                   sentencia,
                   PQresStatus(status_res),
                   PQresultErrorMessage(res));

      if (res == NULL)
        {
          rizoma_errors_set (PQerrorMessage (connection), (gchar *)G_STRFUNC, ERROR);
        }
      else
        {
          return res;
        }
    }
  else
    {
      gchar *strconn = g_strdup_printf ("host=%s port=%s dbname=%s user=%s password=%s sslmode=%s",
                                        host, port, name, user, pass,sslmode);

      connection = PQconnectdb (strconn);

      g_free( strconn );

      status = PQstatus(connection);

      switch (status)
        {
        case CONNECTION_OK:
          res = PQexec (connection, sentencia);
          if (res == NULL)
            rizoma_errors_set (PQerrorMessage (connection), G_STRFUNC, ERROR);
          else
            return res;
          break;
        case CONNECTION_BAD:
          rizoma_errors_set (PQerrorMessage (connection), G_STRFUNC, ERROR);
          break;
        default:
          return NULL;
        }
    }

  return NULL;
}

PGresult *
EjecutarSQL2 (gchar *sentencia)
{
  PGresult *res;
  ConnStatusType status;
  ExecStatusType status_res;

  char *host = rizoma_get_value ("SERVER_HOST");
  char *name = rizoma_get_value ("DB_NAME");
  char *user = rizoma_get_value ("USER");
  char *pass = rizoma_get_value ("PASSWORD");
  char *port = rizoma_get_value ("PORT");

  if (port == NULL)
    port = g_strdup("5432");

  char *sslmode = rizoma_get_value ("SSLMODE");
  if (sslmode == NULL)
    sslmode = g_strdup("require");

  status = PQstatus( connection2 );

  if (status == CONNECTION_OK)
    {
      res = PQexec (connection2, sentencia);
      status_res = PQresultStatus(res);

      if ((status_res != PGRES_COMMAND_OK) && (status_res != PGRES_TUPLES_OK))
        g_printerr("SQL: %s\nErr: %s\nMsg: %s",
                   sentencia,
                   PQresStatus(status_res),
                   PQresultErrorMessage(res));

      if (res == NULL)
        {
          rizoma_errors_set (PQerrorMessage (connection2), (gchar *)G_STRFUNC, ERROR);
        }
      else
        {
          return res;
        }
    }
  else
    {
      gchar *strconn = g_strdup_printf ("host=%s port=%s dbname=%s user=%s password=%s sslmode=%s",
                                        host, port, name, user, pass,sslmode);

      connection2 = PQconnectdb (strconn);

      g_free( strconn );

      status = PQstatus(connection2);

      switch (status)
        {
        case CONNECTION_OK:
          res = PQexec (connection2, sentencia);
          if (res == NULL)
            rizoma_errors_set (PQerrorMessage (connection2), G_STRFUNC, ERROR);
          else
            return res;
          break;
        case CONNECTION_BAD:
          rizoma_errors_set (PQerrorMessage (connection2), G_STRFUNC, ERROR);
          break;
        default:
          return NULL;
        }
    }

  return NULL;
}


gboolean
DataExist (gchar *sentencia)
{
  PGresult *res;

  res = EjecutarSQL (sentencia);

  if (res == NULL)
    return FALSE;
  else if ((PQntuples (res)) == 0)
    return FALSE;
  else
    return TRUE;

}

gchar *
GetDataByOne (gchar *setencia)
{
  PGresult *res;

  res = EjecutarSQL (setencia);

  if (res != NULL && PQntuples (res) != 0)
    return PQgetvalue (res, 0, 0);
  else
    return NULL;
}

/**
 * Actualiza el producto y cambia el atributo estado a false, si el producto
 * tiene su estado false significa que no existe para el sistema
 *
 * @param codigo codigo de barras del producto que quiere ser borrado
 *
 * @return TRUE on succesfull deletetion
 */
gboolean
DeleteProduct (gchar *codigo)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("UPDATE producto SET estado=false WHERE barcode=%s", codigo);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gint
InsertNewDocument (gint sell_id, gint document_type, gint sell_type, gchar *rut_cliente)
{
  PGresult *res;
  gint total;

  total = CalcularTotal (venta->header);
  res = EjecutarSQL (g_strdup_printf
                     ("INSERT INTO documentos_emitidos (id_venta, id_factura, monto, rut_cliente, tipo_documento, forma_pago, num_documento, fecha_emision)"
                      "VALUES (%d, 0, %d, %s, %d, %d, %d, NOW()) RETURNING id", sell_id, total, rut_cliente, document_type, sell_type, get_ticket_number (document_type) + 1));

  if (res != NULL)
    return atoi (PQvaluebycol (res, 0, "id"));
  else
    return -1;
}


gint
InsertNewDocumentVoid (gint sell_id, gint document_type, gint sell_type, gchar *rut_cliente)
{
  PGresult *res;
  res = EjecutarSQL (g_strdup_printf
                     ("INSERT INTO documentos_emitidos (id_venta, id_factura, monto, rut_cliente, tipo_documento, forma_pago, num_documento, fecha_emision)"
                      "VALUES (%d, 0, %d, %s, %d, %d, %d, NOW()) RETURNING id", sell_id, 0, rut_cliente, document_type, sell_type, get_ticket_number (document_type) + 1));

  if (res != NULL)
    return atoi (PQvaluebycol (res, 0, "id"));
  else
    return -1;
}


gint
get_last_sell_id ()
{
  PGresult *res;
  res = EjecutarSQL ("SELECT last_value FROM venta_id_seq");

  if (res != NULL)
    return atoi (PQgetvalue (res, 0, 0));
  else
    return 1;
}


gboolean
InsertNewDocumentDetail (gint document_id, gchar *barcode, gint precio, gdouble cantidad)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf
                     ("INSERT INTO documentos_emitidos_detalle (id_documento, barcode_product, precio, cantidad) "
                      "VALUES (%d, %s, %d, %s)", document_id, barcode, precio, CUT (g_strdup_printf ("%.2f",cantidad))));

  if (res != NULL)
    return TRUE;
  else
    return FALSE;

}


/**
 * Es llamada por las funciones on_sell_button_clicked[ventas.c],
 * on_btn_credit_sale_clicked[ventas.c], on_btn_make_invoice_clicked[ventas.c]
 * Esta funciom  registra los datos en la tabla venta.
 *
 * @param total entero que contiene el precio total de los productos vendidos.
 * @param machine entero que contiene el id de la maquina.
 * @param seller entero que contiene el id del vendedor.
 * @param tipo_venta entero que contiene el tipo de venta que es.
 * @param rut cadena que contiene el rut del cliente deudor
 * @param discount cadena que contiene el descuento
 * @param boleta entero que contien el id de la boleta
 * @param tipo_documento cadena que contiene tipo de documento
 * @param cheque_date cadena que contiene los cheque_date
 * @param cheques boolean que dice si (1) es o no(0) cheque
 * @param cancele boolean que dice si (1) la venta es o no (0) cancelada
 * @return 1 si se realizo correctamente la operacion 0 si hay error
 */
gboolean
SaveSell (gint total, gint machine, gint seller, gint tipo_venta, gchar *rut, gchar *discount, gint boleta,
          gint tipo_documento, gchar *cheque_date, gboolean cheques, gboolean canceled, gboolean venta_reserva)
{
  gint venta_id, monto, id_documento;
  gint day, month, year;
  gchar *serie, *numero, *banco, *plaza;
  gchar *inst, *fecha;
  gchar *q;
  gchar *vale_dir = rizoma_get_value ("VALE_DIR");
  gboolean vale_continuo = rizoma_get_value_boolean ("VALE_CONTINUO");

  gint rut1, rut2, monto2;
  gchar *rut1_gc, *rut2_gc, *dv1, *dv2;
  gboolean afecto_impuesto1, afecto_impuesto2;


  /*
    TODO: id_documento no se justifica en la tabla venta.
    la tabla documentos_emitidos hace referencia a ventas, puesto
    que es una relación de uno a muchos.
    id_documento debe ser eliminado de la tabla venta, actualmente se
    mantiene para no dificultar las migraciones.
  */
  id_documento = 0;
  q = g_strdup_printf( "SELECT inserted_id FROM registrar_venta( %d, %d, %d, "
                       "%d::smallint, %d::smallint, %s::smallint, %d, '%d' )",
                       total, machine, seller, tipo_documento, tipo_venta,
                       CUT(discount), id_documento, canceled);
  venta_id = atoi (GetDataByOne (q));
  g_free (q);


  if (venta_reserva == TRUE)
    pagar_deuda_reserva (venta_id, venta->id_reserva);

  /* Solo se registra un 'Documento' cuando se imprime una boleta o factura */

  /* id_documento = InsertNewDocument (venta_id, tipo_documento, tipo_venta); */
  /* if (id_documento == -1) */
  /*   return FALSE; */


  /* Registra el detalle de venta y pobla la estructura de producto con
     los monto afecto y no afecto de cada producto (cuando es pago mixto) */
  SaveProductsSell (venta->header, venta_id, tipo_venta);

  // Imprimir vale
  if (vale_dir != NULL && !g_str_equal(vale_dir, "") && boleta != -1)
    {
      if (vale_continuo)
	PrintValeContinuo (venta->header, venta_id, rut, boleta, total, tipo_venta, tipo_documento, NULL);
      else
	PrintVale (venta->header, venta_id, rut, boleta, total, tipo_venta, tipo_documento);
    }

  // Abrir Gaveta
  switch (tipo_venta)
    {
    case CHEQUE:
      abrirGaveta ();
      break;
    case TARJETA:
      break;
    case CREDITO:
      break;
    case CASH:
      //abrirGaveta ();
      break;
    case CHEQUE_RESTAURANT:
      abrirGaveta ();
      break;
    case MIXTO:
      //abrirGaveta ();
      break;
    }

  switch (tipo_venta)
    {
    case CHEQUE:
      {
	serie = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->cheque_serie)));
	numero = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->cheque_numero)));
	banco = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->cheque_banco)));
	plaza = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->cheque_plaza)));
	monto = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->cheque_monto))));

	day = atoi (g_strdup_printf ("%c%c", cheque_date[0], cheque_date[1]));
	month = atoi (g_strdup_printf ("%c%c", cheque_date[3], cheque_date[4]));
	year = atoi (g_strdup_printf ("%c%c%c%c", cheque_date[6], cheque_date[7], cheque_date[8],
				      cheque_date[9]));

	SaveDataCheque (venta_id, serie, atoi (numero), banco, plaza, monto, day, month, year);
      }
      break;

    case TARJETA:
      {
	inst = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->tarjeta_inst)));
	numero = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->tarjeta_numero)));
	fecha = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->tarjeta_fecha)));

	SaveVentaTarjeta (venta_id, inst, numero, fecha);
      }
      break;

    case CREDITO:
      {
	InsertDeuda (venta_id, atoi (rut), seller);

	if (GetResto (atoi (rut)) != 0)
	  CancelarDeudas (0, atoi (rut));
      }
      break;

    case CHEQUE_RESTAURANT:
      {
	//Ingresar el o los cheques
	ingresar_cheques (pago_chk_rest->id_emisor, venta_id, pago_chk_rest->header);
      }
      break;

    case MIXTO:
      {
	//Ingreso de la info en pagos mixtos
	gchar **str_splited;

	//Se agrega el monto del pago credito a la deuda
	if (pago_mixto->tipo_pago1 == CREDITO)
	  {
	    rut1_gc = g_strdup (pago_mixto->rut_credito1);
	    str_splited = parse_rut (pago_mixto->rut_credito1);
	    InsertDeuda (venta_id, atoi (str_splited[0]), seller);

	    if (GetResto (atoi (str_splited[0])) != 0)
	      CancelarDeudas (0, atoi (str_splited[0]));

	    afecto_impuesto1 = TRUE;
	  }

	if (pago_mixto->tipo_pago2 == CREDITO)
	  {
	    rut2_gc = g_strdup (pago_mixto->rut_credito2);
	    str_splited = parse_rut (pago_mixto->rut_credito2);
	    InsertDeuda (venta_id, atoi (str_splited[0]), seller);

	    if (GetResto (atoi (str_splited[0])) != 0)
	      CancelarDeudas (0, atoi (str_splited[0]));

	    afecto_impuesto2 = TRUE;
	  }

	//Se ingresan los cheques correspondientes
	if (pago_mixto->tipo_pago1 == CHEQUE_RESTAURANT)
	  {
	    rut1_gc = g_strdup (pago_mixto->check_rest1->rut_emisor);
	    ingresar_cheques (pago_mixto->check_rest1->id_emisor, 
			      venta_id, pago_mixto->check_rest1->header);
	    afecto_impuesto1 = FALSE;
	  }
	if (pago_mixto->tipo_pago2 == CHEQUE_RESTAURANT)
	  {
	    rut2_gc = g_strdup (pago_mixto->check_rest2->rut_emisor);
	    ingresar_cheques (pago_mixto->check_rest2->id_emisor, 
			      venta_id, pago_mixto->check_rest2->header);
	    afecto_impuesto2 = FALSE;
	  }

	//Si el segundo pago es en efectivo
	if (pago_mixto->tipo_pago2 == CASH)
	  {
	    rut2_gc = g_strdup_printf ("0-0");
	    afecto_impuesto2 = TRUE;
	  }
	  
	//Se registra el detalle del pago mixto
	str_splited = parse_rut (rut1_gc);
	rut1 = atoi (str_splited[0]);
	dv1 = str_splited[1];

	str_splited = parse_rut (rut2_gc);
	rut2 = atoi (str_splited[0]);
	dv2 = str_splited[1];
	
	/* Se registra el monto exacto el dinero suficiente para completar la venta
	   a no ser que sea cheque de restaurant, ya que éste permite registrar "excesos"
	   puesto que no se le entrega vuelto */
	if (pago_mixto->tipo_pago2 != CHEQUE_RESTAURANT)
	  monto2 = pago_mixto->total_a_pagar - pago_mixto->monto_pago1;
	else
	  monto2 = pago_mixto->monto_pago2;

	/*Se registra el detalle del pago mixto*/
	EjecutarSQL (g_strdup_printf ("INSERT INTO pago_mixto VALUES (DEFAULT, %d, %d, %d, %d, %d, '%s', '%s', %s, %s, %d, %d)",
				      venta_id,
				      pago_mixto->tipo_pago1, pago_mixto->tipo_pago2,
				      rut1, rut2, dv1, dv2,
				      (afecto_impuesto1==TRUE) ? "true" : "false",
				      (afecto_impuesto2==TRUE) ? "true" : "false",
				      pago_mixto->monto_pago1, monto2));

      }
      break;

    case CASH:
      break;
    default:
      g_printerr("%s: Trying to sale without a proper sell type", G_STRFUNC);
      return FALSE;
    }

  //TODO: en el case FACTURA, el id_documento debería funcionar de otra forma.
  switch (tipo_documento)
    {
    case FACTURA: //specific operations for invoice
      InsertNewDocument (venta_id, tipo_documento, tipo_venta, strtok (g_strdup (rut),"-"));
      if (rizoma_get_value_boolean ("PRINT_FACTURA"))  //print the invoice
	//PrintDocument(tipo_documento, rut, total, id_documento, venta->header);
      break;

    case SIMPLE: //specific operations for cash
      break;

    case GUIA: //specific operations for guide
      InsertNewDocument (venta_id, tipo_documento, tipo_venta, strtok (g_strdup (rut),"-"));
      break;

    case VENTA:
      break;
    default:
      g_printerr("%s: Trying to sale without the proper document type", G_STRFUNC);
      return FALSE;
    }
  return TRUE;
}



/**
 *
 *
 */
gboolean
registrar_reserva (gint maquina, gint vendedor, gint rut_cliente, GDate *fecha_entrega)
{
  gchar *q;
  gint id_reserva;
  gint monto;

  id_reserva = 0;
  monto = CalcularTotal (venta->header);

  q = g_strdup_printf ("INSERT INTO reserva (id, monto, fecha, fecha_entrega, maquina, vendedor, rut_cliente, vendido) "
		       "VALUES (DEFAULT, %d, NOW(), to_timestamp('%.2d %.2d %.4d', 'DD MM YYYY'), %d, %d, %d, FALSE) RETURNING id", 
		       monto, g_date_get_day(fecha_entrega), g_date_get_month(fecha_entrega), g_date_get_year(fecha_entrega),
		       maquina, vendedor, rut_cliente);
  
  id_reserva = atoi (GetDataByOne (q));

  //Se registra la deuda de la reserva
  EjecutarSQL (g_strdup_printf ("INSERT INTO deuda_reserva (id, id_reserva, rut_cliente, vendedor, monto_adeudado, fecha_deuda, pagado) "
				"VALUES (DEFAULT, %d, %d, %d, %d, NOW(), FALSE)", id_reserva, rut_cliente, vendedor, monto));

  if (id_reserva > 0 && registrar_reserva_detalle (id_reserva))
    return TRUE;
  else
    return FALSE;
}



/**
 *
 *
 */
gboolean
actualizar_fecha_reserva (gint id_reserva, GDate *fecha_entrega)
{
  gchar *q;

  q = g_strdup_printf ("UPDATE reserva SET fecha_entrega = to_timestamp('%.2d %.2d %.4d', 'DD MM YYYY') WHERE id=%d", 
		       g_date_get_day(fecha_entrega), g_date_get_month(fecha_entrega), g_date_get_year(fecha_entrega), id_reserva);
  

  if (EjecutarSQL (q) != NULL)
    return TRUE;
  else
    return FALSE;
}


/**
 *
 *
 */
gboolean
registrar_reserva_detalle (gint id_reserva)
{
  gchar *q;
  Productos *products = venta->header;
  gdouble precio;

  if (id_reserva <= 0)
    return FALSE;

  do
    {
      if (products->product->cantidad_mayorista > 0 && products->product->precio_mayor > 0 &&
	  products->product->cantidad >= products->product->cantidad_mayorista &&
	  products->product->mayorista == TRUE)
	precio = products->product->precio_mayor;
      else
	precio = products->product->precio;

      
      q = g_strdup_printf ("INSERT INTO reserva_detalle (id, id_reserva, barcode, cantidad, precio, precio_neto, costo_promedio) "
			   "VALUES (DEFAULT, %d, %s, %s, %s, %s, %s)",
			   id_reserva,
			   products->product->barcode,
			   CUT (g_strdup_printf ("%.3f", products->product->cantidad)),
			   CUT (g_strdup_printf ("%.3f", precio)),
			   CUT (g_strdup_printf ("%.3f", products->product->precio_neto)),
			   CUT (g_strdup_printf ("%.3f", products->product->fifo)));
      EjecutarSQL (q);
    } while (products != venta->header);

  return TRUE;
}


/**
 *
 */
gboolean
registrar_pago_reserva (gint id_reserva, gint monto_pagado, gint tipo_pago)
{
  gchar *q;
  gint id_deuda_reserva, monto_adeudado, total_pagado;
  PGresult *res;

  q = g_strdup_printf ("SELECT id, monto_adeudado FROM deuda_reserva WHERE id_reserva = %d", id_reserva);
  res = EjecutarSQL (q);

  id_deuda_reserva = atoi (PQvaluebycol (res, 0, "id"));
  monto_adeudado = atoi (PQvaluebycol (res, 0, "monto_adeudado"));

  q = g_strdup_printf ("INSERT INTO pago_deuda_reserva (id, id_deuda_reserva, monto_pagado, fecha_pago, tipo_pago) "
		       "VALUES (DEFAULT, %d, %d, NOW(), %d)", id_deuda_reserva, monto_pagado, tipo_pago);

  EjecutarSQL (q);

  total_pagado = atoi (GetDataByOne (g_strdup_printf ("SELECT SUM (monto_pagado) FROM pago_deuda_reserva WHERE id_deuda_reserva = %d", id_deuda_reserva)));

  if (total_pagado >= monto_adeudado)
    return TRUE;
  else
    return FALSE;
}


/**
 *
 *
 */
void
pagar_deuda_reserva (gint id_venta, gint id_reserva)
{
  gchar *q;
  PGresult *res;

  q = g_strdup_printf ("UPDATE reserva SET vendido = TRUE, id_venta = %d WHERE id = %d", id_venta, id_reserva);
  res = EjecutarSQL (q);

  q = g_strdup_printf ("UPDATE deuda_reserva SET pagado = TRUE WHERE id_reserva = %d", id_reserva);
  EjecutarSQL (q);
}


/**
 * This function returns a PGresult with the sells
 * If anuladas parameter is NULL return all sells,
 * if TRUE, return nullified sales only and
 * if FALSE, return sales not nullified only
 *
 * @param: gint begin year
 * @param: gint begin month
 * @param: gint begin day
 * @param: gint end year
 * @param: gint end month
 * @param: gint end day
 * @param: gchar date colum
 * @param: gchar fields
 * @param: gchar grupo, "Anuladas", "Ingresadas", "TODAS"
 */
PGresult *
SearchTuplesByDate (gint from_year, gint from_month, gint from_day,
                    gint to_year, gint to_month, gint to_day,
                    gchar *date_column, gchar *fields, gchar *grupo)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT %s FROM venta WHERE "
		       "%s>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND "
		       "%s<=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')",
		       fields, date_column, from_day, from_month, from_year,
		       date_column, to_day+1, to_month, to_year);


  if (g_str_equal (grupo, "TODAS"))
    printf ("Todas las ventas\n");

  else if (g_str_equal (grupo, "Anuladas"))
    q = g_strdup_printf ("%s AND id IN (SELECT id_sale FROM venta_anulada)", q);

  else if (g_str_equal (grupo, "Vigentes"))
    q = g_strdup_printf ("%s AND id NOT IN (SELECT id_sale FROM venta_anulada)", q);

  q = g_strdup_printf ("%s ORDER BY fecha DESC", q);

  res = EjecutarSQL (q);

  if (res != NULL)
    return res;
  else
    return NULL;
}

PGresult *
exempt_sells_on_date (gint from_year, gint from_month, gint from_day,
		      gint to_year, gint to_month, gint to_day)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT v.id, v.maquina, v.vendedor, v.tipo_venta, "
		       "       (SELECT SUM (vd.cantidad * vd.precio) "
		       "               FROM venta_detalle vd "
		       "               WHERE vd.id_venta = v.id "
		       "               AND vd.impuestos = false) AS monto, "
		       "       to_char (fecha, 'DD/MM/YY HH24:MI:SS') AS fmt_fecha "
		       "FROM venta v "
		       "WHERE v.fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') "
		       "AND v.fecha<=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') "
		       "AND v.id NOT IN (SELECT id_sale FROM venta_anulada) "
		       "AND (SELECT SUM (vd.cantidad * vd.precio) "
		       "            FROM venta_detalle vd "
		       "            WHERE vd.id_venta = v.id "
		       "            AND vd.impuestos = false) IS NOT NULL "
		       "ORDER BY v.fecha DESC",
		       from_day, from_month, from_year,
		       to_day+1, to_month, to_year);

  res = EjecutarSQL (q);

  if (res != NULL)
    return res;
  else
    return NULL;
}


PGresult *
inmovilizados_en_periodo (gint from_year, gint from_month, gint from_day, gchar *max_avg_sell, gchar *max_unid_sell)
{
  PGresult *res;
  gchar *q;

  //TODO: solo debe ser para mercaderías corrientes
  q = g_strdup_printf ("SELECT barcode, codigo_corto, (descripcion||' '||marca||' '||cont_un) AS descripcion, "
		       "       stock_inicial, stock_teorico, "
	               "       (ventas_periodo-anulaciones_periodo) AS unidades_vendidas, "
		       "       ((ventas_periodo-anulaciones_periodo)/CASE WHEN (stock_inicial*100) = 0 THEN 1 ELSE stock_inicial*100 END) AS porcentaje_vendido, "
		       "       (SELECT precio FROM producto WHERE producto.barcode = pep.barcode::bigint) AS precio, "
		       "       (SELECT costo FROM obtener_costo_promedio_desde_barcode (barcode::bigint)) AS costo_promedio "
		       "FROM producto_en_periodo ('%.4d-%.2d-%.2d') AS pep ",
		       from_year, from_month, from_day);

  /*Se aplica a la un filtro a la consulta*/
  if (!g_str_equal (max_unid_sell, "") || !g_str_equal (max_avg_sell, ""))
    {
      q = g_strdup_printf ("%s WHERE ", q);
      
      if (!g_str_equal (max_unid_sell, "") && g_str_equal (max_avg_sell, ""))
	q = g_strdup_printf ("%s (ventas_periodo-anulaciones_periodo) < %s ", q, CUT(max_unid_sell));
      else if (g_str_equal (max_unid_sell, "") && !g_str_equal (max_avg_sell, ""))
	q = g_strdup_printf ("%s ((ventas_periodo-anulaciones_periodo)/stock_inicial*100) < %s ", q, CUT(max_avg_sell));
      else
	q = g_strdup_printf ("%s ((ventas_periodo-anulaciones_periodo)/stock_inicial*100) < %s "
			     "OR (ventas_periodo-anulaciones_periodo) < %s ", q, CUT(max_avg_sell), CUT(max_unid_sell));
    }

  /*Los datos se ordenan por su porcentaje de venta*/
  q = g_strdup_printf ("%s "
		       "ORDER BY ((ventas_periodo-anulaciones_periodo)/CASE WHEN (stock_inicial*100) = 0 THEN 1 ELSE stock_inicial*100 END) ASC", q);

  res = EjecutarSQL (q);

  if (res != NULL)
    return res;
  else
    return NULL;
}


gint
GetTotalCashSell (guint from_year, guint from_month, guint from_day,
                  guint to_year, guint to_month, guint to_day, gint *total)
{
  PGresult *res;

  res = EjecutarSQL
    (g_strdup_printf
     ("SELECT round (SUM(vd.precio * vd.cantidad)), "
      "count (Distinct(v.id)) "
      "FROM venta v,venta_detalle vd "
      "WHERE fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') "
      "AND fecha<to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') and v.id = id_venta "
      "AND tipo_venta = %d "
      "AND v.id NOT IN (select id_sale from venta_anulada)",
      from_day, from_month, from_year, to_day+1, to_month, to_year, CASH));

  if (res == NULL)
    return 0;

  *total = atoi (PQgetvalue (res, 0, 1));

  return atoi (PQgetvalue (res, 0, 0));
}

gint
GetTotalCreditSell (guint from_year, guint from_month, guint from_day,
                    guint to_year, guint to_month, guint to_day, gint *total)
{
  PGresult *res;

  res = EjecutarSQL
    (g_strdup_printf
     ("SELECT COALESCE (SUM ((SELECT SUM(cantidad * precio) FROM venta_detalle WHERE id_venta=venta.id)), 0), "
      "       COALESCE (count (*), 0) "
      "FROM venta "
      "WHERE fecha>=to_timestamp('%.2d %.2d %.4d', 'DD MM YYYY') "
      "AND fecha<to_timestamp('%.2d %.2d %.4d', 'DD MM YYYY') "
      "AND ((tipo_venta)=%d OR (tipo_venta)=%d) "
      "AND venta.id NOT IN (SELECT id_sale FROM venta_anulada)",
      from_day, from_month, from_year, to_day+1, to_month, to_year, CREDITO, TARJETA));

  if (res == NULL)
    return 0;

  *total = atoi (PQgetvalue (res, 0, 1));

  return atoi (PQgetvalue (res, 0, 0));
}


/**
 * This function get the subtotal of taxes (IVA and Others) 
 * from sells within a date range.
 *
 * @param: guint from_year, guint from_month, guint from_day
 * @param: guint to_year, guint to_month, guint to_day, 
 * @param: gdouble *total_iva: In which to store the result (IVA total)
 * @param: gdouble *total_otros: In which to store the result (Others total)
 */
void
total_taxes_on_time_interval (guint from_year, guint from_month, guint from_day,
			      guint to_year, guint to_month, guint to_day, 
			      gint *total_iva, gint *total_otros)
{
  PGresult *res;

  res = EjecutarSQL
    (g_strdup_printf
     ("SELECT ROUND (SUM((SELECT SUM(iva)   FROM venta_detalle WHERE id_venta=v.id))) AS total_iva, "
      "       ROUND (SUM((SELECT SUM(otros) FROM venta_detalle WHERE id_venta=v.id))) AS total_otros "
      "FROM venta v "
      "WHERE fecha >= TO_TIMESTAMP ('%.2d %.2d %.4d', 'DD MM YYYY') "
      "AND fecha   <  TO_TIMESTAMP ('%.2d %.2d %.4d', 'DD MM YYYY') "
      "AND v.id NOT IN (SELECT id_sale FROM venta_anulada)",
      from_day, from_month, from_year, to_day+1, to_month, to_year));

  if (res == NULL)
    return;

  *total_iva = atoi(PQvaluebycol (res, 0, "total_iva"));
  *total_otros = atoi (PQvaluebycol (res, 0, "total_otros"));
}


gint
GetTotalSell (guint from_year, guint from_month, guint from_day,
              guint to_year, guint to_month, guint to_day, gint *total)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf
                     ("SELECT COALESCE (ROUND(SUM(vd.precio * vd.cantidad)), 0), "
		      "       COALESCE (COUNT(DISTINCT(v.id)), 0) "
                      "FROM venta v, venta_detalle vd "
                      "WHERE fecha>=to_timestamp('%.2d %.2d %.4d', 'DD MM YYYY') "
                      "AND fecha<to_timestamp('%.2d %.2d %.4d', 'DD MM YYYY') and v.id = id_venta "
                      "AND  v.id NOT IN (select id_sale from venta_anulada)",
                      from_day, from_month, from_year, to_day+1, to_month, to_year));

  if (res == NULL)
    return 0;

  *total = atoi (PQgetvalue (res, 0, 1));

  return atoi (PQgetvalue (res, 0, 0));
}

gboolean
InsertClient (gchar *nombres, gchar *paterno, gchar *materno, gchar *rut, gchar *ver,
              gchar *direccion, gchar *fono, gint credito, gchar *giro, gint client_type)
{
  PGresult *res;
  gchar *q;
  gchar *tipo;

  if (client_type == INVOICE)
    tipo = g_strdup ("factura");
  else if (client_type == CREDIT)
    tipo = g_strdup ("credito");
  else //Si por algún motivo esta seteada la bandera de ALL o cualquier otra cosa, lo tomará como credito
    tipo = g_strdup ("credito");

  q = g_strdup_printf ("INSERT INTO cliente (rut, dv, nombre, apell_p, apell_m, giro, abonado, direccion, telefono, credito, tipo) "
                       "VALUES (%s, '%s', '%s', '%s', '%s', '%s', 0, '%s', '%s', %d, '%s')",
                       rut, ver, nombres, paterno, materno, giro, direccion, fono, credito, tipo);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gboolean
insert_emisores (gchar *rut, gchar *dv, gchar *razon_social, gchar *telefono, gchar *direccion,
		 gchar *comuna, gchar *ciudad, gchar *giro)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("INSERT INTO emisor_cheque (rut, dv, razon_social, telefono, direccion, comuna, ciudad, giro) "
                       "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')",
                       rut, dv, razon_social, telefono, direccion, comuna, ciudad, giro);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gboolean
RutExist (const gchar *rut)
{
  PGresult *res;
  gchar *q;
  gchar *rut2 = g_strdup(rut);
  q = g_strdup_printf ("SELECT * FROM cliente WHERE rut=%s", strtok(rut2,"-"));
  res = EjecutarSQL (q);
  g_free (q);

  if (res == NULL)
    return FALSE;
  else if (PQntuples (res) != 0)
    return TRUE;
  else
    return FALSE;
}

gint
InsertDeuda (gint id_venta, gint rut, gint vendedor)
{
  EjecutarSQL (g_strdup_printf ("INSERT INTO deuda (id_venta, rut_cliente, vendedor) "
				"VALUES (%d, %d, %d)",
				id_venta, rut, vendedor));

  return 0;
}

gint
DeudaTotalCliente (gint rut)
{
  PGresult *res;
  gint deuda;

  res = EjecutarSQL (g_strdup_printf ("SELECT SUM (monto) as monto "
				      "FROM search_deudas_cliente (%d, true)",
                                      rut));

  deuda = atoi (PQgetvalue (res, 0, 0));
  deuda -= GetResto (rut);

  return deuda;
}


gboolean
tiene_limite_credito (gint rut)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT credito FROM cliente "
				      "WHERE rut=%d", rut));

  if (atoi (PQgetvalue (res, 0, 0)) == 0)
    return FALSE;
  else
    return TRUE;
}


PGresult *
SearchDeudasCliente (gint rut)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT id, monto, maquina, vendedor, tipo_venta, tipo_complementario, monto_complementario, "
		       "date_part('day', fecha) AS day, date_part('month', fecha) AS month, date_part('year', fecha) AS year, "
                       "date_part('hour', fecha) AS hour, date_part('minute', fecha) AS minute, date_part ('second', fecha) AS second "
		       "FROM search_deudas_cliente (%d, true) "
		       "ORDER BY id DESC", rut);
  res = EjecutarSQL (q);
  g_free (q);

  return res;
}

PGresult *
search_deudas_guias_facturas_cliente (gint rut, gchar *filtro, gint tipo_documento)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT id_venta_out AS id_venta, id_documento_out AS id_documento, monto_out AS monto, id_factura_out AS id_factura, "
		       "       maquina_out AS maquina, vendedor_out AS vendedor, tipo_documento_out AS tipo_documento, "
		       "       date_part('day', fecha_emision_out) AS day, date_part('month', fecha_emision_out) AS month, date_part('year', fecha_emision_out) AS year, "
                       "       date_part('hour', fecha_emision_out) AS hour, date_part('minute', fecha_emision_out) AS minute, date_part ('second', fecha_emision_out) AS second "
		       "FROM search_facturas_guias (%d, true, %d, %d, %d, '%s'::varchar) ", rut, GUIA, FACTURA, tipo_documento, filtro);
  res = EjecutarSQL (q);
  g_free (q);

  return res;
}


gint
PagarDeuda (gchar *id_venta)
{
  gchar *q;

  q = g_strdup_printf ("UPDATE deuda SET pagada='t' WHERE id_venta=%s", id_venta);
  EjecutarSQL (q);

  return 0;
}

gint
pagar_factura (gint id_factura, gint id_venta)
{
  gchar *q;

  // La factura y sus guias asociadas se marcan como pagadas
  q = g_strdup_printf ("UPDATE documentos_emitidos SET pagado='t' "
		       "WHERE id=%d OR id_factura=%d", id_factura, id_factura);
  EjecutarSQL (q);
  g_free (q);

  //Revisa que todas las facturas asociadas a esa venta esten pagadas
  if (id_venta != 0)
    { //Se paga la deuda total de la venta si todos los documentos asociados a esa venta estan pagados
      if (!DataExist (g_strdup_printf ("SELECT id FROM documentos_emitidos "
				       "WHERE id_venta=%d AND pagado='f'", id_venta)))
	PagarDeuda (g_strdup_printf ("%d", id_venta));
    }
  else
    { /* Se paga la deuda total de la venta si todas las guias asociadas a esta factura estan pagadas
	 y además cada venta asociada a esas guias tenga todos sus documentos como pagados. */
      if (!DataExist (g_strdup_printf ("SELECT id FROM documentos_emitidos "
				       "WHERE id_venta IN (SELECT id_venta FROM documentos_emitidos "
				       "                   WHERE id_factura = %d) "
				       "AND pagado='f'", id_factura)))
	{
	  q = g_strdup_printf ("UPDATE deuda SET pagada='t' "
			       "WHERE id_venta IN (SELECT id_venta FROM documentos_emitidos "
			       "                   WHERE id_factura = %d)", id_factura);
	  EjecutarSQL (q);
	}
    }
  return 0;
}

gint
facturar_guia (gint id_factura, gint id_guia, gint monto_guia)
{
  // Se asocia la guia a la factura
  EjecutarSQL (g_strdup_printf ("UPDATE documentos_emitidos "
				"SET id_factura=%d "
				"WHERE id=%d", id_factura, id_guia));

  EjecutarSQL (g_strdup_printf ("UPDATE documentos_emitidos "
				"SET monto=monto+%d "
				"WHERE id=%d", monto_guia, id_factura));

  return 0;
}


gint
CancelarDeudas (gint abonar, gint rut)
{
  PGresult *res;
  //  gint deuda = DeudaTotalCliente (rut);
  gint monto_venta;
  gint i, tuples;
  gchar *q;

  q = g_strdup_printf ("INSERT INTO abono VALUES (DEFAULT, %d, %d, NOW())", rut, abonar);
  res = EjecutarSQL (q);
  g_free (q);

  q = g_strdup_printf ("SELECT * FROM search_deudas_cliente (%d, true) "
		       "ORDER BY fecha asc", rut);
  res = EjecutarSQL (q);
  g_free (q);

  abonar += GetResto (rut);

  if (CreditoDisponible (rut) == 0 &&
      DeudaTotalCliente (rut) <= abonar)
    {

    }

  tuples = PQntuples (res);

  for (i = 0; i < tuples; i++)
    {
      monto_venta = atoi (PQvaluebycol (res, i, "monto"));

      if (monto_venta <= abonar)
        {
          abonar -= monto_venta;

          PagarDeuda (PQvaluebycol (res, i, "id"));
        }
    }

  SaveResto (abonar, rut);

  return 0;
}

gint
GetResto (gint rut)
{
  PGresult *res;
  gint resto;

  res = EjecutarSQL (g_strdup_printf ("SELECT abonado FROM cliente WHERE rut=%d", rut));

  resto = atoi (PQgetvalue (res, 0, 0));

  return resto;
}

gboolean
SaveResto (gint resto, gint rut)
{
  PGresult *res;

  //  resto += GetResto (rut);

  res = EjecutarSQL (g_strdup_printf ("UPDATE cliente SET abonado=%d WHERE rut=%d", resto, rut));

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gchar *
ReturnClientName (gint rut)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT nombre || '\n' || apellido_paterno || '\n' || apellido_materno AS "
                                      "nombrecompleto FROM cliente WHERE rut=%d", rut));

  return PQgetvalue (res, 0, 0);
}

gchar *
ReturnClientPhone (gint rut)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT telefono FROM cliente WHERE rut=%d", rut));

  return PQgetvalue (res, 0, 0);
}

gchar *
ReturnClientAdress (gint rut)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT direccion FROM cliente WHERE rut=%d", rut));

  return PQgetvalue (res, 0, 0);
}

gint
ReturnClientCredit (gint rut)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT credito FROM cliente WHERE rut=%d", rut));

  return atoi (PQgetvalue (res, 0, 0));
}

gchar *
ReturnClientStatusCredit (gint rut)
{
  PGresult *res;
  gchar *value;

  res = EjecutarSQL (g_strdup_printf ("SELECT credito_enable FROM cliente WHERE rut=%d", rut));

  value = g_strdup (PQgetvalue (res, 0, 0));

  if (strcmp (value, "t") == 0)
    return "Habilitado";
  else
    return "Inhabilitado";
}

gint
CreditoDisponible (gint rut)
{
  PGresult *res;
  gint credito;

  res = EjecutarSQL (g_strdup_printf ("SELECT credito FROM cliente WHERE rut=%d", rut));

  credito = atoi (PQgetvalue (res, 0, 0));

  // Si credito es 0, no tiene limite de credito
  if (credito == 0)
    return 0;
  else
    return credito - DeudaTotalCliente (rut);
}

gchar *
ReturnPasswd (gchar *user)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT passwd FROM users WHERE usuario='%s'", user));

  if (res != NULL && PQntuples (res) != 0)
    return PQgetvalue (res, 0, 0);
  else
    return NULL;
}

gchar *
ReturnLlave (gchar *user)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT llave FROM users WHERE usuario='%s'", user));

  if (PQntuples (res) != 0)
    return PQgetvalue (res, 0, 0);
  else
    return NULL;
}

gint
ReturnUserId (gchar *user)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT id FROM users WHERE usuario='%s'", user));

  return atoi (PQgetvalue (res, 0, 0));
}

gint
ReturnUserLevel (gchar *user)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT level FROM users WHERE usuario='%s'", user));

  return atoi (PQgetvalue (res, 0, 0));
}
gchar *
ReturnUsername (gint id)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT usuario FROM users WHERE id=%d", id));

  if (res != NULL || PQntuples (res) != 0)
    return PQgetvalue (res, 0, 0);
  else
    return NULL;
}

/**
 * Change the password of the seller (by user id)
 *
 * @param passwd the new password
 * @param user the user id, NOT the username
 *
 * @return TRUE on succesfull operation
 */
gboolean
SaveNewPassword (gchar *passwd, gchar *user)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("UPDATE users SET passwd=md5('%s')WHERE id='%s'",
                                      passwd, user));

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gboolean
AddNewSeller (gchar *rut, gchar *nombre, gchar *apell_p, gchar *apell_m,
              gchar *username, gchar *passwd, gchar *id)
{
  PGresult *res;
  gchar *q;
  gchar **rut_splited;

  rut_splited = g_strsplit(rut, "-", 0);

  if (strcmp (id, "") != 0)
    {
      res = EjecutarSQL (g_strdup_printf
                         ("SELECT * FROM users WHERE id='%s'", id));

      if (PQntuples (res) != 0)
        {
          rizoma_errors_set (g_strdup_printf ("Ya existe un vendedor con el id: %s", id), G_STRFUNC, ERROR);

          return FALSE;
        }
    }

  q = g_strdup_printf ("INSERT INTO users (id, usuario, passwd, rut, dv, nombre, apell_p, apell_m, fecha_ingreso, \"level\") "
                       "VALUES (%s, '%s', md5('%s'), %s, '%s', '%s', '%s', '%s', NOW(), 1)",
                       strcmp (id, "") != 0 ? id : "DEFAULT", username, passwd, rut_splited[0], rut_splited[1], nombre, apell_p, apell_m);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    {
      rizoma_errors_set ("El vendedor se ha agregado con exito", G_STRFUNC, APPLY);
      return TRUE;
    }
  else
    {
      rizoma_errors_set ("Error al intentar agregar un nuevo vendedor", G_STRFUNC, ERROR);
      return FALSE;
    }
}


gboolean
add_to_pedido_temporal (gchar *barcode, gdouble cantidad, gdouble precio_compra, gdouble margen, gdouble precio)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("INSERT INTO pedido_temporal (id, barcode, cantidad, costo_promedio, margen, precio) "
                       "VALUES (DEFAULT, %s, %s, %s, %s, %s)",
                       barcode, 
		       CUT (g_strdup_printf ("%.3f", cantidad)), 
		       CUT (g_strdup_printf ("%.3f", precio_compra)),
		       CUT (g_strdup_printf ("%.3f", margen)), 
		       CUT (g_strdup_printf ("%.3f", precio)));
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}


gboolean
del_to_pedido_temporal (gchar *barcode)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("DELETE FROM pedido_temporal WHERE barcode = %s", barcode);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}


gboolean
clean_pedido_temporal (void)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("DELETE FROM pedido_temporal");
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

PGresult *
get_pedido_temporal (void)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT * FROM pedido_temporal");
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return res;
  else
    return NULL;
}



gboolean
ReturnUserExist (gchar *user)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT usuario FROM users WHERE usuario='%s'", user));

  if (PQntuples (res) == 1)
    return TRUE;
  else
    return FALSE;
}

void
ChangeEnableCredit (gboolean status, gint rut)
{
  gchar *q;

  q = g_strdup_printf ("UPDATE cliente SET credito_enable='%d' WHERE rut=%d",
                       (gint) status, rut);
  EjecutarSQL (q);
  g_free (q);
}

gboolean
ClientDelete (gint rut)
{
  PGresult *res = NULL;

  if (DeudaTotalCliente (rut) == 0)
    res = EjecutarSQL (g_strdup_printf ("UPDATE cliente SET activo = 'f' WHERE rut=%d", rut));

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gboolean
emisor_delete (gint id)
{
  //La consulta comprueba si hay facturas que no esten facturadas ni pagadas que pertenezcan al proveedor especificado
  if (!DataExist (g_strdup_printf ("SELECT * FROM cheque_rest WHERE id_emisor = %d AND facturado = 'f' AND pagado = 'f'", id)))
    {
      EjecutarSQL (g_strdup_printf ("UPDATE emisor_cheque SET activo = 'f' WHERE id=%d", id));
      return TRUE;
    }
  else
    return FALSE;
}


gboolean
fact_cheque_rest (gint id)
{
  //La consulta comprueba si hay facturas que no esten facturadas ni pagadas que pertenezcan al proveedor especificado
  if (!DataExist (g_strdup_printf ("SELECT * FROM cheque_rest WHERE id = %d AND facturado = 't'", id)))
    {
      EjecutarSQL (g_strdup_printf ("UPDATE cheque_rest SET facturado = 't' WHERE id=%d", id));
      return TRUE;
    }
  else
    return FALSE;
}


gboolean
DataProductUpdate (gchar *barcode, gchar *codigo, gchar *description, gint precio)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("UPDATE producto SET codigo_corto='%s', descripcion='%s', precio=%d WHERE barcode='%s'",
                       codigo, SPE(description), precio, barcode);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}


/**
 * Called by "Save" function in compras.c
 * Update the product's modifications
 *
 * @param Product attributes
 *
 * TODO: Sería más eficiente si recibiera algún tipo de array "diccionario" de distintos tipos
 */
void
SaveModifications (gchar *codigo, gchar *description, gchar *marca, gchar *unidad,
                   gchar *contenido, gchar *precio, gboolean iva, gint otros, gchar *barcode,
                   gboolean perecible, gboolean fraccion, gint familia)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT COALESCE (costo_promedio,0) AS costo_promedio FROM producto WHERE barcode ='%s'", barcode);
  res = EjecutarSQL(q);
  g_free(q);

  // Recalcular el porcentaje de ganancia a partir del precio final del producto
  // TODO: Unificar este cálculo, pues se realiza una y otra vez en distintas partes
  gdouble porcentaje;
  gdouble costo_promedio = strtod (PUT (PQvaluebycol (res, 0, "costo_promedio")), (char **)NULL);
  gdouble precio_local = strtod (PUT (g_strdup (precio)), (char **)NULL);
  gdouble precio_neto = precio_local; //Esto por defecto, se actualiza su valor dependiendo de si tiene impuesto

  // Impuestos
  gdouble iva_local;
  gdouble otros_local;

  /*Tiene IVA y Otros es la opcion "Sin Impuestos"*/
  if (iva == TRUE && (otros == 0 || otros == -1))
    {
      // Obtención IVA
      q = g_strdup_printf ("SELECT monto FROM impuesto WHERE id = 1");
      res = EjecutarSQL(q);
      iva_local = (gdouble) atoi (PQvaluebycol (res, 0, "monto"));
      g_free(q);

      // Calculo
      if (costo_promedio != 0)
	{
	  iva_local = (iva_local / 100) + 1;
	  porcentaje = (gdouble) ((precio_local / (gdouble)(iva_local * costo_promedio)) -1) * 100;
	  precio_neto = precio_local / iva_local;
	}
      else
	porcentaje = 0;
    }

  /*Tiene IVA y Otros es una opcion distinta a "Sin Impuestos"*/
  else if (iva == TRUE && (otros != 0 || otros != -1))
    {
      // Obtención Impuestos
      q = g_strdup_printf ("SELECT monto FROM impuesto WHERE id = 1 OR id = %d", otros);
      res = EjecutarSQL(q);
      iva_local = (gdouble) atoi (PQvaluebycol (res, 0, "monto"));
      otros_local = (gdouble) atoi (PQvaluebycol (res, 1, "monto"));
      g_free(q);

      // Calculo
      if (costo_promedio != 0)
	{
	  iva_local = (gdouble) (iva_local / 100);
	  otros_local = (gdouble) (otros_local / 100);

	  precio_neto = (gdouble) precio_local / (gdouble)(iva_local + otros_local + 1);
	  porcentaje = (gdouble) precio_neto - costo_promedio;
	  porcentaje = lround ((gdouble)(porcentaje / costo_promedio) * 100);
	}
      else
	porcentaje = 0;
    }

  /*No tiene IVA y Otros es la opcion "Sin Impuestos"*/
  else if (iva == FALSE && (otros == 0 || otros == -1))
    {
      if (costo_promedio != 0)
	{
	  porcentaje = (gdouble) ((precio_local / costo_promedio) - 1) * 100;
	  precio_neto = precio_local;
	}
      else
	porcentaje = 0;
    }

  q = g_strdup_printf ("UPDATE producto SET codigo_corto='%s', descripcion=UPPER('%s'),"
                       "marca=UPPER('%s'), unidad=UPPER('%s'), contenido='%s', "
                       "precio=%s, precio_neto=%s, impuestos='%d', otros=%d, familia=%d, "
                       "perecibles='%d', fraccion='%d', margen_promedio=%s WHERE barcode='%s'",
                       codigo, SPE(description), SPE(marca), unidad, contenido,
		       CUT (g_strdup_printf ("%.2f", precio_local)), CUT (g_strdup_printf ("%.2f", precio_neto)), 
		       iva, otros, familia,
		       (gint)perecible, (gint)fraccion, CUT (g_strdup_printf ("%.2f", porcentaje)), barcode);
  res = EjecutarSQL(q);
  g_free(q);
}

gboolean
AddNewProductToDB (gchar *codigo, gchar *barcode, gchar *description, gchar *marca,
                   gchar *contenido, gchar *unidad, gboolean iva, gint otros, gint familia,
                   gboolean perecible, gboolean fraccion, gint tipo)
{
  gint insertado;
  gchar *q;

  q = g_strdup_printf ("SELECT insertar_producto::integer FROM insertar_producto (%s::bigint, '%s'::varchar, "
                       "upper('%s')::varchar, upper('%s')::varchar, '%s'::varchar, upper('%s')::varchar, "
                       "%d::boolean, %d, %d::smallint, %d::boolean, "
                       "%d::boolean, %d::int)",barcode, codigo, SPE(marca), SPE(description), contenido, unidad, iva,
                       otros, familia, perecible, fraccion, tipo);
  insertado = atoi (GetDataByOne (q));
  g_free(q);

  if (insertado == 1)
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

void
AgregarCompra (gchar *rut, gchar *nota, gint dias_pago)
{
  gint id_compra;
  gchar *q;

  q = g_strdup_printf("SELECT * FROM insertar_compra( '%s', '%s', '%d' )",
                      rut, nota, dias_pago); // dias_pago == 0 ? dias_pago - 1 : dias_pago);
  id_compra = atoi(GetDataByOne(q));

  g_free(q);

  SaveBuyProducts (compra->header_compra, id_compra);
}

void
SaveBuyProducts (Productos *header, gint id_compra)
{
  Productos *products = header;
  gdouble iva = 0, otros = 0;
  gchar *cantidad;
  gchar *costo, *precio, *precio_neto, *margen;
  gchar *q;

  do
    {
      if (products->product->iva != 0)
        iva = (gdouble) (products->product->precio_compra *
                         products->product->cantidad) *
          (gdouble)products->product->iva / 100;
      else
	iva = 0;

      if (products->product->otros != 0)
        otros = (gdouble) (products->product->precio_compra *
                           products->product->cantidad) *
          (gdouble)products->product->otros / 100;
      else
	otros = 0;

      precio_neto = g_strdup_printf ("%.2f", products->product->precio / 
				     (products->product->otros/100 + products->product->iva/100 + 1));

      cantidad = g_strdup_printf ("%.3f", products->product->cantidad);
      costo = g_strdup_printf ("%.2f", products->product->precio_compra);
      precio = g_strdup_printf ("%.2f", products->product->precio);
      margen = g_strdup_printf ("%.2f", products->product->margen);

      q = g_strdup_printf("SELECT * FROM insertar_detalle_compra(%d, "
                          "%s::double precision, %s::double precision, %s::double precision, %s::double precision, "
                          "0::double precision, 0::smallint, %s, %s, %ld, %ld)",
                          id_compra, CUT(cantidad), CUT (costo),
                          CUT (precio), CUT (precio_neto), products->product->barcode,
                          CUT (margen), lround (iva),
                          lround (otros));
      EjecutarSQL(q);

      g_free(costo);
      g_free(cantidad);
      g_free(q);

      products = products->next;
    }
  while (products != header);
}

gboolean
CompraIngresada (void)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("UPDATE compra SET ingresada='t' WHERE id IN "
                       "(SELECT id_compra FROM compra_detalle WHERE cantidad=cantidad_ingresada) "
                       "AND ingresada='f'");
  res = EjecutarSQL (q);
  g_free(q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gboolean
IngresarDetalleDocumento (Producto *product, gint compra, gint doc, gboolean factura)
{
  PGresult *res;
  gchar *cantidad;
  gchar *q;
  gdouble iva = 0, otros = 0;

  if (product->iva != 0)
    iva = (gdouble) (product->precio_compra * product->cantidad) *
      (gdouble)product->iva / 100;
  else
    iva = 0;

  if (product->otros != 0)
    otros = (gdouble) (product->precio_compra * product->cantidad) *
      (gdouble)product->otros / 100;
  else
    otros = 0;


  cantidad = CUT (g_strdup_printf ("%.2f", product->cantidad));

  if (factura)
    {
      q = g_strdup_printf
        ("INSERT INTO factura_compra_detalle (id, id_factura_compra, barcode, cantidad, precio, iva, otros, costo_promedio) "
         "VALUES (DEFAULT, %d, %s, %s, %s, %ld, %ld, %s)",
         doc, product->barcode, cantidad, CUT (g_strdup_printf ("%.2f", product->precio_compra)), 
	 lround (iva), lround (otros), CUT (g_strdup_printf ("%.3f", product->fifo)));
    }
  else
    {
      q = g_strdup_printf
        ("INSERT INTO guias_compra_detalle (id, id_guias_compra, barcode, cantidad, precio, iva, otros) "
         "VALUES (DEFAULT, %d, %s, %s, '%s', %ld, %ld)",
         doc, product->barcode, cantidad, CUT (g_strdup_printf ("%.2f", product->precio_compra)),lround (iva), lround (otros));
    }

  res = EjecutarSQL (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gboolean
IngresarProducto (Producto *product, gint compra, gchar *rut_proveedor)
{
  PGresult *res;
  //  gint old_stock, stock_final;
  gdouble fifo;
  gdouble stock_pro, canjeado;
  gdouble imps, ganancia, margen_promedio;
  gchar *q;
  gchar *cantidad;


  //Asociamos el producto al proveedor (Si ya esta asociado dará un aviso por terminal, pero no detendra nada)
  q = g_strdup_printf ("INSERT INTO producto_proveedor (barcode_producto, rut_proveedor, costo_compra) "
		       "       VALUES (%s, %s, %s) RETURNING barcode_producto",
                       product->barcode, rut_proveedor, CUT (g_strdup_printf ("%.2f", product->precio_compra)));
  
  //Se ejecuta la consulta y si la relación ya existía, se actualiza el precio de compra del producto
  if (PQntuples (EjecutarSQL (q)) == 0)
    {
      q = g_strdup_printf ("UPDATE producto_proveedor SET costo_compra = %s "
			   "WHERE  barcode_producto = %s AND rut_proveedor = %s",
			   CUT (g_strdup_printf ("%.2f", product->precio_compra)), product->barcode, rut_proveedor);
      EjecutarSQL (q);
    }

  cantidad = CUT (g_strdup_printf ("%.2f", product->cantidad));

  if (product->stock_pro != 0 && product->tasa_canje != 0)
    canjeado = product->stock_pro * ((double)1 / product->tasa_canje);
  else
    canjeado = 0;

  stock_pro = product->stock_pro - (product->cuanto - canjeado);

  /*
    Calculamos el margen Promedio
  */
  q = g_strdup_printf ("UPDATE compra_detalle SET cantidad_ingresada=cantidad_ingresada+%s, canjeado=%s "
                       "WHERE barcode_product IN (SELECT barcode  FROM producto WHERE barcode='%s' AND id_compra=%d)",
                       cantidad, CUT (g_strdup_printf ("%.2f", canjeado)),product->barcode, compra);
  res = EjecutarSQL (q);

  /*
    Calculamos el precio ponderado
  */

  fifo = FiFo (product->barcode, compra);
  product->fifo = fifo;

  if (product->otros != 0)
    imps = (gdouble) product->iva / 100 + (gdouble)product->otros / 100;
  else
    imps = (gdouble) product->iva / 100;

  ganancia = (product->precio / (imps + 1));

  margen_promedio = (gdouble)((ganancia - fifo) / fifo) * 100;

  if (product->canjear == TRUE)
    {
      q = g_strdup_printf ("UPDATE producto SET margen_promedio=%s, costo_promedio=%s, stock=stock+%s, stock_pro=%s WHERE barcode=%s",
                           CUT (g_strdup_printf ("%.2f", margen_promedio)), CUT (g_strdup_printf ("%.2f", fifo)), cantidad, CUT (g_strdup_printf ("%.2f", stock_pro)), product->barcode);
      res = EjecutarSQL (q);
      g_free (q);
    }
  else
    {
      q = g_strdup_printf ("UPDATE producto SET margen_promedio=%s, costo_promedio=%s, stock=stock+%s WHERE barcode=%s",
                           CUT (g_strdup_printf ("%2f", margen_promedio)), CUT (g_strdup_printf ("%.2f", fifo)), cantidad, product->barcode);
      res = EjecutarSQL (q);
      g_free (q);
    }

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gboolean
DiscountStock (Productos *header)
{
  Productos *products = header;
  PGresult *res;
  gchar *cantidad;

  do
    {
      cantidad = CUT (g_strdup_printf ("%.2f", products->product->cantidad));

      res = EjecutarSQL (g_strdup_printf ("UPDATE productos SET stock=stock-%s WHERE barcode=%s",
                                          cantidad, products->product->barcode));

      if (res == NULL)
        return FALSE;

      products = products->next;

    }
  while (products != header);

  return TRUE;
}

gchar *
GetUnit (gchar *barcode)
{
  PGresult *res;
  char *unit;

  res = EjecutarSQL (g_strdup_printf ("SELECT contenido, unidad FROM select_producto(%s)", barcode));

  unit = g_strdup_printf ("%s %s",
                          PQvaluebycol (res, 0, "contenido"),
                          PQvaluebycol (res, 0, "unidad"));

  return unit;
}

gdouble
GetCurrentStock (gchar *barcode)
{
  PGresult *res;
  gdouble stock;

  res = EjecutarSQL (g_strdup_printf ("SELECT disponible FROM obtener_stock_desde_barcode (%s)", barcode));

  stock = strtod (PUT(PQgetvalue (res, 0, 0)), (char **)NULL);

  return stock;
}

/**
 * Returns the current price of the product
 * @param barcode barcode of the product that must return the price
 * @return the price, if the could not be found the price returns -1
 */
char *
GetCurrentPrice (gchar *barcode)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT precio FROM select_producto(%s)",
                       barcode);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL && PQntuples (res) != 0)
    return PQgetvalue (res, 0, 0);
  else
    return "-1";
}

gdouble
FiFo (gchar *barcode, gint compra)
{
  PGresult *res;
  gchar *q;
  gchar *costo;
  gdouble fifo;

  q = g_strdup_printf ("select calculate_fifo (%s,%d)", barcode, compra);
  res = EjecutarSQL (q);
  g_free (q);

  costo = PUT (g_strdup_printf ("%s", PQgetvalue (res, 0, 0)));
  fifo = strtod (PUT(costo), (char **)NULL);

  return fifo;
}

/**
 * Es llamada por la funcion SaveSell
 * Esta funciom calcula el iva, otros, ademas de descontar los productos
 * vendidos del stock y luego registra la venta en la tabla venta_detalle
 *
 * @param products puntero que apunta a los valores del producto que se va vender
 * @param id_venta es el id de la venta que se esta realizando
 * @param tipo_venta es un enum (tipo_pago), para registrar la venta de acuerdo 
 *        a la forma en que se paga
 * @param detalle_compuesto es un boolean que indica si se esta registrando el detalle 
 *        de la venta o el detalle de un compuesto (dentro del detalle de la venta)
 * @return 1 si se realizo correctamente la operacion 0 si hay error
 */

gboolean
SaveProductsSell (Productos *products, gint id_venta, gint tipo_venta)
{
  //PGresult *res;
  Productos *header = products;
  gchar *q;

  //Datos Venta
  gint monto_venta;
  gint monto_descuento;

  //Datos Producto
  gdouble iva, otros, iva_residual, otros_residual, iva_percent, otros_percent;
  gdouble precio;

  //Para el pago mixto
  gint pago1, pago2, total_venta;
  gboolean is_imp1, is_imp2;

  //gdouble iva_promedio, otros_promedio;
  gdouble impuestos, ganancia, proporcion_iva, proporcion_otros;
  gdouble proporcion_producto, neto, precio_neto;
  gdouble monto_afecto, monto_no_afecto, total_prod_afecto, total_prod_no_afecto;

  //Tipo de mercadería
  //gint corriente;

  //Se inicializan los impuestos
  iva = otros = 0;
  iva_percent = otros_percent = 0;

  //Se obtiene el monto pagado y el posible descuento
  monto_venta = atoi (g_strdup (PQvaluebycol (EjecutarSQL (g_strdup_printf ("SELECT monto FROM venta WHERE id = %d", id_venta)), 0, "monto")));
  monto_descuento = atoi (g_strdup (PQvaluebycol (EjecutarSQL (g_strdup_printf ("SELECT descuento FROM venta WHERE id = %d", id_venta)), 0, "descuento")));

  //Se obtiene el id del tipo mercadería (corriente)
  //corriente = atoi (g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE'"), 0, "id")));

  if (tipo_venta == MIXTO)
    {
      total_venta = pago_mixto->total_a_pagar;
      pago1 = pago_mixto->monto_pago1;
      
      /* Se asegura que el pago sea justo puesto que en casos, como el segundo pago con
	 cheque de restaurant, puede haber un ingreso mayor al pago requerido, ese pago excesivo se
         registra pero no participa del los calculos aquí requeridos */
      pago2 = total_venta - pago_mixto->monto_pago1;

      total_prod_afecto = CalcularSoloAfecto (products);
      total_prod_no_afecto = CalcularSoloNoAfecto (products);
      is_imp1 = (pago_mixto->tipo_pago1 == CHEQUE_RESTAURANT) ? FALSE : TRUE;
      is_imp2 = (pago_mixto->tipo_pago2 == CHEQUE_RESTAURANT) ? FALSE : TRUE;

      printf ("\nTotal a pagar: %d\n", total_venta);
      printf ("\nTotal afecto: %ld\n", lround (total_prod_afecto));
      printf ("\nTotal no afecto: %ld\n", lround (total_prod_no_afecto));
      printf ("\nMonto 1: %d\n", pago1);
      printf ("\nMonto 2: %d\n", pago2);
    }

  do
    {
      iva_percent = GetIVA (products->product->barcode);
      otros_percent = GetOtros (products->product->barcode);
      
      if (iva_percent != -1 || iva_percent != 0)
	iva_percent = (gdouble) iva_percent / 100;
      else
	iva_percent = 0;

      if (otros_percent != -1 || otros_percent != 0)
        otros_percent = (gdouble) otros_percent / 100;
      else 
	otros_percent = 0;

      /*Obtiene el precio de venta del producto*/
      if (products->product->cantidad_mayorista > 0 && products->product->precio_mayor > 0 &&
          products->product->cantidad >= products->product->cantidad_mayorista &&
          products->product->mayorista == TRUE)
        precio = products->product->precio_mayor;
      else
        precio = products->product->precio;


      /*Se obtiene el precio proporcional en caso de haber un descuento en el monto total de la venta*/
      if (monto_descuento > 0) // subtotal - (descuento proporcional)
	precio = (((products->product->cantidad * precio) - 
		   (monto_descuento * ( (products->product->cantidad * precio) / 
					(monto_venta + monto_descuento)))) / products->product->cantidad);

      /*Se obtiene el IVA*/
      if (iva_percent != 0)
	iva = (precio / (iva_percent+otros_percent+1) * iva_percent) * products->product->cantidad;
      else
	iva = 0;

      /*Se obtiene otros*/
      if (otros_percent != 0)
        otros = (precio / (iva_percent+otros_percent+1) * otros_percent) * products->product->cantidad;
      else
	otros = 0;

      /*Pago de impuestos (pasados a gchar *)*/
      //Si se realizó solamene con cheque de restaurant
      if (tipo_venta == CHEQUE_RESTAURANT)
	{
	  iva_residual = 0;
	  otros_residual = 0;
	  proporcion_iva = 0;
	  proporcion_otros = 0;
	}
      //Si se realizó de forma mixta
      else if (tipo_venta == MIXTO)
	{
	  // Si ambos pagos son afecto a impuesto, no se hacen tratamientos especiales
	  if (is_imp1 == TRUE && is_imp2 == TRUE)
	    {
	      iva_residual = iva;
	      otros_residual = otros;
	      proporcion_iva = 1;
	      proporcion_otros = 1;
	    }
	  //Si niguno es afecto a impuesto
	  else if (is_imp1 == FALSE && is_imp2 == FALSE)
	    {
	      iva_residual = 0;
	      otros_residual = 0;
	      proporcion_iva = 0;
	      proporcion_otros = 0;
	    }
	  //Si uno es afecto a impuesto y el otro no (y además el producto esta afecto a impuestos)
	  else if (GetIVA (products->product->barcode) != 0)
	    { //TODO: Calcular solo con el total_afecto

	      if (is_imp1 == FALSE && is_imp2 == TRUE)
		{
		  //El pago no afecto debe ser menor o igual al total de los productos afectos
		  if (pago1 <= lround (total_prod_afecto)) //RESTRICCION IMPUESTA POR JAIME!
		    {
		      monto_afecto = total_prod_afecto - pago1;
		      monto_no_afecto = pago1;
		    }
		  else
		    {
		      printf ("\nNo se completó la venta %d: \n"
			      "No se debe permitir una venta donde el pago no afecto a impuesto "
			      "sea mayor al total de productos afectos a impuesto\n", id_venta);
		      return FALSE;
		    }
		}
	      else if (is_imp1 == TRUE && is_imp2 == FALSE)
		{
		  //El pago no afecto debe ser menor o igual al total de los productos afectos
		  if (pago2 <= lround (total_prod_afecto)) //RESTRICCION IMPUESTA POR JAIME!
		    {
		      monto_afecto = total_prod_afecto - pago2;
		      monto_no_afecto = pago2;
		    }
		  else
		    {
		      printf ("\nNo se completó la venta %d: \n"
			      "No se debe permitir una venta donde el pago no afecto a impuesto "
			      "sea mayor al total de productos afectos a impuesto\n", id_venta);
		      return FALSE;
		    }
		}
	      
	      proporcion_producto  = ((precio * products->product->cantidad) / total_prod_afecto);
	
	      //Se calculan los montos afecto y no a impuestos
	      monto_afecto = monto_afecto * proporcion_producto;
	      monto_no_afecto = monto_no_afecto * proporcion_producto;

	      //Se registran los montos en la estructura de productos, para ser impresos en la boleta
	      products->product->proporcion_afecta_imp = monto_afecto;
	      products->product->proporcion_no_afecta_imp = monto_no_afecto;

	      printf ("\nafecto: %ld, no afecto: %ld, subtotal: %ld\n", 
		      lround (monto_afecto), lround (monto_no_afecto), lround (monto_afecto + monto_no_afecto));

	      //Se obtienen los impuestos del producto
	      /* iva = GetIVA (products->product->barcode); */
	      /* if (iva != -1 || iva != 0) */
	      /* 	iva = iva / 100; */
	      /* else */
	      /* 	iva = 0; */

	      /* otros = GetOtros (products->product->barcode); */
	      /* if (otros != -1 || otros != 0) */
	      /* 	otros = otros / 100; */
	      /* else */
	      /* otros = 0; */

	      impuestos = (iva_percent+otros_percent+1);
	      neto = monto_afecto / impuestos;
	      iva_residual = neto * iva_percent;
	      otros_residual = neto * otros_percent;
	      
	      //                        iva porpocional al monto afecto / iva normal // TODOOOOO
	      proporcion_iva = (iva_residual / (precio/(iva_percent+otros_percent+1)*iva_percent*products->product->cantidad));
	      proporcion_otros = (otros_residual / (precio/(iva_percent+otros_percent+1)*(otros_percent == 0 ? 1 : otros_percent)
						    *products->product->cantidad));
	    }
	  else //Si es un pago mixto (con un pago afecto y el otro no), pero el producto no esta afecto a impuestos
	    {
	      iva_residual = 0;
	      otros_residual = 0;
	      proporcion_iva = 0;
	      proporcion_otros = 0;
	    }
	}
      //cualquier tipo de pago afecto a impuesto
      else 
	{
	  iva_residual = iva;
	  otros_residual = otros;
	  proporcion_iva = 1;
	  proporcion_otros = 1;
	}

      precio_neto = precio / (iva_percent + otros_percent + 1);
      ganancia = precio_neto - products->product->fifo;
      ganancia = ganancia * products->product->cantidad;

      /* Registra los productos con sus respectivos datos(barcode,cantidad,
	 precio,fifo,iva,otros) en la tabla venta_detalle y venta_mc_detalle */
      q = g_strdup_printf ("select registrar_venta_detalle(%d, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %d, %s, %s, %s)",
			   id_venta, products->product->barcode,
			   CUT (g_strdup_printf ("%.3f", products->product->cantidad)),
			   CUT (g_strdup_printf ("%.3f", precio)),
			   CUT (g_strdup_printf ("%.3f", precio_neto)),
			   CUT (g_strdup_printf ("%.3f", products->product->fifo)),
			   CUT (g_strdup_printf ("%.3f", iva)),
			   CUT (g_strdup_printf ("%.3f", otros)),
			   CUT (g_strdup_printf ("%.3f", iva_residual)),
			   CUT (g_strdup_printf ("%.3f", otros_residual)),
			   CUT (g_strdup_printf ("%.3f", ganancia)),
			   products->product->tipo, (products->product->impuestos==TRUE) ? "true" : "false",
			   CUT (g_strdup_printf ("%.5f", proporcion_iva)),
			   CUT (g_strdup_printf ("%.5f", proporcion_otros)));

      printf ("la consulta es %s\n", q);
      EjecutarSQL (q);
      g_free (q);

      products = products->next;
    }
  while (products != header);

  return TRUE;
}

/*--------- TRANSFER RANK START -------------------*/

PGresult *
ReturnTransferRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gboolean traspaso_envio)
{
  PGresult *res;
  gchar *q, *traspaso_envio_q;

  if (traspaso_envio == TRUE)
    traspaso_envio_q = g_strdup ("'t'");
  else
    traspaso_envio_q = g_strdup("'f'");

  q = g_strdup_printf
    ("SELECT * FROM ranking_traspaso (to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, %s)",
     from_day, from_month, from_year, to_day+1, to_month, to_year, traspaso_envio_q);

  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return res;
  else
    return NULL;
}

PGresult *
ReturnMpTransferRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gboolean traspaso_envio)
{
  PGresult *res;
  gchar *q, *traspaso_envio_q;

  if (traspaso_envio == TRUE)
    traspaso_envio_q = g_strdup ("'t'");
  else
    traspaso_envio_q = g_strdup ("'f'");

  q = g_strdup_printf
    ("SELECT * FROM ranking_traspaso_mp (to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, %s)",
     from_day, from_month, from_year, to_day+1, to_month, to_year, traspaso_envio_q);

  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return res;
  else
    return NULL;
}

PGresult *
ReturnMcTransferRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gboolean traspaso_envio)
{
  PGresult *res;
  gchar *q, *traspaso_envio_q;

  if (traspaso_envio == TRUE)
    traspaso_envio_q = g_strdup ("'t'");
  else
    traspaso_envio_q = g_strdup ("'f'");

  q = g_strdup_printf
    ("SELECT * FROM ranking_traspaso_mc (to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, %s)",
     from_day, from_month, from_year, to_day+1, to_month, to_year, traspaso_envio_q);

  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return res;
  else
    return NULL;
}

/*-------- TRANSFER RANK END ----------------*/


PGresult *
ReturnProductsRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gint family)
{
  PGresult *res;
  gchar *q, *family_filter;

  if (family == 0)
    family_filter = g_strdup ("");
  else
    family_filter = g_strdup_printf("WHERE familia = %d", family);

  q = g_strdup_printf
    ("SELECT * FROM ranking_ventas (to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date) %s",
     from_day, from_month, from_year, to_day+1, to_month, to_year, family_filter);

  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return res;
  else
    return NULL;
}

PGresult *
ReturnMpProductsRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gint family)
{
  PGresult *res;
  gchar *q, *family_filter;

  if (family == 0)
    family_filter = g_strdup ("");
  else
    family_filter = g_strdup_printf("WHERE familia = %d", family);

  q = g_strdup_printf
    ("SELECT * FROM ranking_ventas_mp (to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date) %s",
     from_day, from_month, from_year, to_day+1, to_month, to_year, family_filter);

  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return res;
  else
    return NULL;
}

PGresult *
ReturnMcProductsRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gint family)
{
  PGresult *res;
  gchar *q, *family_filter;

  if (family == 0)
    family_filter = g_strdup ("");
  else
    family_filter = g_strdup_printf("WHERE familia = %d", family);

  q = g_strdup_printf
    ("SELECT * FROM ranking_ventas_mc (to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date) %s",
     from_day, from_month, from_year, to_day+1, to_month, to_year, family_filter);

  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return res;
  else
    return NULL;
}


/*----------- TRANSFER RANK ON CHANGE ROW START ---------------*/

/**
 *
 *
 */
PGresult *
ReturnDerivTransferRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gchar *barcode_madre)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf
    ("SELECT * FROM ranking_traspaso_deriv (to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, '%s')",
     from_day, from_month, from_year, to_day+1, to_month, to_year, barcode_madre);

  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return res;
  else
    return NULL;
}


/**
 *
 *
 */
PGresult *
ReturnCompTransferRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gchar *barcode_madre)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf
    ("SELECT * FROM ranking_traspaso_comp (to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, '%s')",
     from_day, from_month, from_year, to_day+1, to_month, to_year, barcode_madre);

  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return res;
  else
    return NULL;
}

/*----------- TRANSFER RANK ON CHANGE ROW END -----------------*/

/**
 *
 *
 */
PGresult *
ReturnDerivProductsRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gchar *barcode_madre)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf
    ("SELECT * FROM ranking_ventas_deriv (to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, '%s')",
     from_day, from_month, from_year, to_day+1, to_month, to_year, barcode_madre);

  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return res;
  else
    return NULL;
}


/**
 *
 *
 */
PGresult *
ReturnCompProductsRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gchar *barcode_madre)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf
    ("SELECT * FROM ranking_ventas_comp (to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, '%s')",
     from_day, from_month, from_year, to_day+1, to_month, to_year, barcode_madre);

  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return res;
  else
    return NULL;
}


gboolean
AddProveedorToDB (gchar *rut, gchar *nombre, gchar *direccion, gchar *ciudad, gchar *comuna,
                  gchar *telefono, gchar *email, gchar *web, gchar *contacto, gchar *giro)
{
  PGresult *res;
  gchar *q;
  gchar **aux;

  aux = g_strsplit(rut, "-", 0);

  q = g_strdup_printf ("INSERT INTO proveedor(rut, dv, nombre, direccion, ciudad,"
                       "comuna, telefono, email, web, contacto, giro) VALUES "
                       "('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')",
                       aux[0], aux[1], nombre, direccion, ciudad,
                       comuna, telefono, email, web, contacto, giro);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gboolean
SetProductosIngresados (void)
{
  EjecutarSQL ("UPDATE compra_detalle SET ingresado='t' WHERE cantidad_ingresada=cantidad");

  return TRUE;
}

gint
GetMinStock (gchar *barcode)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT COALESCE ((dias_stock * select_ventas_dia(producto.barcode, TRUE)::float), 0) AS stock_min "
		       "FROM producto WHERE barcode='%s'",
                       barcode);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL && PQntuples (res) != 0)
    return atoi (PQgetvalue (res, 0, 0));
  else
    return -1;
}

gboolean
SaveDataCheque (gint id_venta, gchar *serie, gint number, gchar *banco, gchar *plaza, gint monto,
                gint day, gint month, gint year)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("INSERT INTO cheques VALUES (DEFAULT, %d, '%s', %d, '%s', '%s', %d, "
                       "to_timestamp('%.2d %.2d %.4d', 'DD MM YYYY'))", id_venta, serie, number,
                       banco, plaza, monto, day, month, year);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}


gboolean
ingresar_cheques (gint id_emisor, gint id_venta, ChequesRestaurant *header)
{
  ChequesRestaurant *cheques = header;
  PGresult *res;
  gchar *q;

  do
    {
      //Si no hay fecha de vencimiento, no guarda una fecha
      if (g_str_equal (cheques->cheque->fecha_vencimiento, ""))
	q = g_strdup_printf ("INSERT INTO cheque_rest (id, id_emisor, id_venta, codigo, monto, facturado)"
			     "VALUES (DEFAULT, %d, %d, '%s', %d, false)", 
			     id_emisor, id_venta, cheques->cheque->codigo, cheques->cheque->monto);
      else
	q = g_strdup_printf ("INSERT INTO cheque_rest (id, id_emisor, id_venta, codigo, monto, fecha_vencimiento, facturado)"
			     "VALUES (DEFAULT, %d, %d, '%s', %d, to_timestamp('%s', 'DDMMYY'), false)", 
			     id_emisor, id_venta, cheques->cheque->codigo, cheques->cheque->monto, cheques->cheque->fecha_vencimiento);

      res = EjecutarSQL (q);
      g_free (q);
      
      if (res == NULL)
	return FALSE;
      
      cheques = cheques->next;

    } while (cheques != header);

  if (res != NULL)
    return TRUE;
  
  return FALSE;
}


gint
ReturnIncompletProducts (gint id_venta)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT count(*) FROM compra_detalle WHERE id_compra=%d "
                       "AND cantidad_ingresada>0 AND cantidad_ingresada<cantidad",
                       id_venta);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return atoi (PQgetvalue (res, 0, 0));
  else
    return -1;
}

Proveedor *
ReturnProveedor (gint id_compra)
{
  Proveedor *proveedor = (Proveedor *) g_malloc (sizeof (Proveedor));
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT * FROM proveedor WHERE rut=(SELECT rut_proveedor FROM compra"
                       " WHERE id=%d", id_compra);
  res = EjecutarSQL (q);
  g_free (q);

  if (res == NULL)
    return NULL;

  proveedor->nombre = g_strdup (PQgetvalue (res, 0, 1));
  proveedor->rut = g_strdup (PQgetvalue (res, 0, 2));
  proveedor->direccion = g_strdup (PQgetvalue (res, 0, 3));
  proveedor->comuna = g_strdup (PQgetvalue (res, 0, 4));
  proveedor->telefono = g_strdup (PQgetvalue (res, 0, 5));
  proveedor->email = g_strdup (PQgetvalue (res, 0, 6));
  proveedor->web = g_strdup (PQgetvalue (res, 0, 7));
  proveedor->contacto = g_strdup (PQgetvalue (res, 0, 8));
  proveedor->ciudad = g_strdup (PQgetvalue (res, 0, 9));
  proveedor->ciudad = g_strdup (PQgetvalue (res, 0, 10));


  return proveedor;
}

gint
IngresarFactura (gint n_doc, gint id_compra, gchar *rut_proveedor, gint total_productos, gint d_emision, gint m_emision, gint y_emision, gint guia, gint costo_bruto_transporte)
{
  PGresult *res;
  gchar *q;
  gdouble costo_neto_transporte;
  gdouble monto_iva_transporte;
  gdouble iva;
  
  if (costo_bruto_transporte > 0)
    {
      iva = (gdouble) atoi (PQvaluebycol (EjecutarSQL ("SELECT monto FROM impuesto WHERE id = 1"), 0,"monto")); // X%
      iva = (iva / 100); // 0.X
      costo_neto_transporte = costo_bruto_transporte / (iva+1); //Bruto / 1.X
      monto_iva_transporte = costo_bruto_transporte - costo_neto_transporte;
    }
  else
    {
      costo_neto_transporte = 0;
      monto_iva_transporte = 0;
    }

  //"to_timestamp('%.2d %.2d %.4d', 'DD MM YYYY')" //d_emision, m_emision, y_emision, 

  q = g_strdup_printf ("INSERT INTO factura_compra (id, id_compra, rut_proveedor, num_factura, fecha, fecha_documento, valor_neto, "
                       "valor_iva, descuento, pagada, monto, costo_neto_transporte, iva_transporte) "
		       "VALUES (DEFAULT, %d, '%s', %d, now(), to_timestamp('%.2d %.2d %.4d', 'DD MM YYYY'), "
		       "        0, 0, 0,'f', %d, %s, %s)",
                       id_compra, rut_proveedor, n_doc, d_emision, m_emision, y_emision,
		       total_productos, 
		       CUT (g_strdup_printf ("%.3f", costo_neto_transporte)),
		       CUT (g_strdup_printf ("%.3f", monto_iva_transporte)));
  res = EjecutarSQL (q);
  g_free (q);

  if (id_compra != 0)
    {
      q = g_strdup_printf ("UPDATE factura_compra SET forma_pago=(SELECT compra.forma_pago FROM compra WHERE id=%d) "
                           "WHERE id_compra=%d", id_compra, id_compra);
      res = EjecutarSQL (q);
      g_free (q);

      q = g_strdup_printf ("UPDATE factura_compra SET fecha_pago=DATE(fecha)+(SELECT days FROM formas_pago WHERE id=forma_pago) WHERE id_compra=%d", id_compra);
      res = EjecutarSQL (q);
      g_free (q);
    }
  else
    {
      q = g_strdup_printf ("UPDATE factura_compra SET forma_pago=(SELECT compra.forma_pago FROM compra WHERE id=(SELECT "
                           "id_compra FROM guias_compra WHERE numero=%d AND rut_proveedor='%s')) WHERE num_factura=%d AND "
                           "rut_proveedor='%s'", guia, rut_proveedor, n_doc, rut_proveedor);
      res = EjecutarSQL (q);
      g_free (q);

      q = g_strdup_printf ("UPDATE factura_compra SET fecha_pago=DATE(fecha)+(SELECT days FROM formas_pago WHERE id=forma_pago) WHERE num_factura=%d", n_doc);
      res = EjecutarSQL (q);
      g_free (q);
    }

  res = EjecutarSQL ("SELECT last_value FROM factura_compra_id_seq");

  if (res != NULL)
    return atoi (PQgetvalue (res, 0, 0));
  else
    return -1;
}

gint
IngresarGuia (gint n_doc, gint id_compra, gint d_emision, gint m_emision, gint y_emision)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("INSERT INTO guias_compra (id, numero, id_compra, id_factura, rut_proveedor, fecha_emision) "
                                      "VALUES (DEFAULT, %d, %d, 0, (SELECT rut_proveedor FROM compra WHERE id=%d), "
                                      "to_timestamp('%.2d %.2d %.4d', 'DD MM YYYY'))",
                                      n_doc, id_compra, id_compra, d_emision, m_emision, y_emision));

  res = EjecutarSQL ("SELECT last_value FROM guias_compra_id_seq");

  if (res != NULL)
    return atoi (PQgetvalue (res, 0, 0));
  else
    return -1;

}

gboolean
AsignarFactAGuia (gint id_guia, gint id_factura)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT guide_invoice( %d, ARRAY[%d])", id_factura, id_guia);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gdouble
GetIVA (gchar *barcode)
{
  PGresult *res;
  gint tuples;

  res = EjecutarSQL (g_strdup_printf ("SELECT * FROM get_iva( %s )", barcode));

  tuples = PQntuples (res);

  if (tuples != 0)
    return strtod (PUT(PQvaluebycol (res, 0, "valor")), (char **)NULL);
  else
    return 0;
}

gdouble
GetOtros (gchar *barcode)
{
  PGresult *res;
  gint tuples;

  res = EjecutarSQL (g_strdup_printf ("SELECT * FROM get_otro_impuesto( %s )", barcode));

  tuples = PQntuples (res);

  if (tuples != 0)
    return strtod (PUT(PQvaluebycol (res, 0, "valor")), (char **)NULL);
  else
    return 0;
}

gint
GetOtrosIndex (gchar *barcode)
{
  PGresult *res;
  gint tuples;

  res = EjecutarSQL (g_strdup_printf ("SELECT otros FROM producto WHERE barcode='%s'", barcode));

  tuples = PQntuples (res);

  if (tuples != 0)
    return atoi (PQgetvalue (res, 0, 0));
  else
    return 0;
}

gchar *
GetOtrosName (gchar *barcode)
{
  PGresult *res;
  gint tuples;

  res = EjecutarSQL (g_strdup_printf ("SELECT descripcion FROM impuesto WHERE id=(SELECT otros FROM producto WHERE barcode='%s')", barcode));

  tuples = PQntuples (res);

  if (tuples != 0)
    return PQgetvalue (res, 0, 0);
  else
    return NULL;
}

gdouble
GetNeto (gchar *barcode)
{
  PGresult *res;
  gint tuples;

  res = EjecutarSQL (g_strdup_printf ("SELECT cd.precio "
				      "FROM compra_detalle cd "
				      "INNER JOIN compra c "
				      "ON cd.id_compra = c.id "
				      "WHERE barcode_product = %s "
				      "ORDER BY c.fecha DESC", barcode));

  tuples = PQntuples (res);

  if (tuples != 0)
    return strtod (PUT (PQgetvalue (res, 0, 0)), (char **)NULL);
  else
    return -1;
}

gdouble
obtener_costo_promedio (gchar *barcode)
{
  PGresult *res;
  gint tuples;

  res = EjecutarSQL (g_strdup_printf ("SELECT costo FROM obtener_costo_promedio_desde_barcode (%s)", barcode));

  tuples = PQntuples (res);

  if (tuples != 0)
    return strtod (PUT (PQgetvalue (res, 0, 0)), (char **)NULL);
  else
    return -1;
}

gboolean
CheckCompraIntegrity (gchar *compra)
{
  PGresult *res;
  gint tuples;

  res = EjecutarSQL (g_strdup_printf ("SELECT count(*) from compra_detalle WHERE id_compra=%s AND cantidad_ingresada<cantidad", compra));

  tuples = atoi (PQgetvalue (res, 0, 0));

  if (tuples != 0)
    return FALSE;
  else
    return TRUE;
}

gboolean
CheckProductIntegrity (gchar *compra, gchar *barcode)
{
  PGresult *res;
  gint tuples;

  res = EjecutarSQL (g_strdup_printf ("SELECT count(*) from compra_detalle WHERE id_compra=%s AND barcode_product='%s' AND cantidad_ingresada<cantidad", compra, barcode));

  tuples = atoi (PQgetvalue (res, 0, 0));

  if (tuples != 0)
    return FALSE;
  else
    return TRUE;
}

gint
GetFami (gchar *barcode)
{
  PGresult *res;
  gint tuples;

  res = EjecutarSQL (g_strdup_printf ("SELECT familia FROM producto WHERE barcode='%s'", barcode));

  tuples = PQntuples (res);

  if (tuples != 0)
    return atoi (PQgetvalue (res, 0, 0));
  else
    return -1;
}

gchar *
GetLabelImpuesto (gchar *barcode)
{
  PGresult *res;
  gint tuples;

  res = EjecutarSQL (g_strdup_printf ("SELECT descripcion FROM impuesto WHERE id=(SELECT otros FROM producto WHERE barcode='%s')", barcode));

  tuples = PQntuples (res);

  if (tuples != 0)
    return PQgetvalue (res, 0, 0);
  else
    return "Ninguno";

}

gchar *
GetLabelFamilia (gchar *barcode)
{
  PGresult *res;
  gint tuples;

  res = EjecutarSQL (g_strdup_printf ("SELECT nombre FROM familias WHERE id=(SELECT familia FROM producto WHERE barcode='%s')", barcode));

  tuples = PQntuples (res);

  if (tuples != 0)
    return PQgetvalue (res, 0, 0);
  else
    return "Ninguna";
}

gchar *
GetPerecible (gchar *barcode)
{
  PGresult *res;
  gint tuples;
  gchar *result;

  res = EjecutarSQL (g_strdup_printf ("SELECT perecibles FROM producto WHERE barcode='%s'",
                                      barcode));

  tuples = PQntuples (res);

  if (tuples != 0)
    result = PQgetvalue (res, 0, 0);
  else
    return NULL;

  if (strcmp (result, "") == 0)
    return "No Estimado";
  else if (strcmp (result, "t") == 0)
    return "Perecible";
  else if (strcmp (result, "f") == 0)
    return "No Perecible";
  else
    return NULL;
}

gint
GetTotalBuys (gchar *barcode)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT SUM(precio * cantidad) FROM compra_detalle "
                       "WHERE barcode_product='%s'", barcode);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return atoi (PQgetvalue (res, 0, 0));
  else
    return 0;
}

gchar *
GetElabDate (gchar *barcode, gint current_stock)
{
  PGresult *res;
  gint tuples;
  //  gint stock = 0;
  gchar *date = NULL;
  gchar *q;

  q = g_strdup_printf ("SELECT cantidad, date_part ('day', elaboracion), "
                       "date_part ('month', elaboracion), date_part('year', elaboracion) FROM "
                       "documentos_detalle WHERE barcode='%s' ORDER BY fecha ASC", barcode);
  res = EjecutarSQL (q);
  g_free (q);

  if (res == NULL)
    return "";

  tuples = PQntuples (res);

  if (tuples == 0)
    return "";

  if (strcmp (PQgetvalue (res, 0, 1), "") != 0)
    date = g_strdup_printf ("%.2d/%.2d/%.4d", atoi (PQgetvalue (res, 0, 1)),
                            atoi (PQgetvalue (res, 0, 2)), atoi (PQgetvalue (res, 0, 3)));
  return
    "";


  return date;
}

gchar *
GetVencDate (gchar *barcode, gint current_stock)
{
  PGresult *res;
  gint tuples;
  //  gint stock = 0;
  gchar *date = NULL;

  res = EjecutarSQL (g_strdup_printf
                     ("SELECT cantidad, date_part ('day', vencimiento), "
                      "date_part ('month', vencimiento), date_part('year', vencimiento) FROM "
                      "documentos_detalle WHERE barcode='%s' ORDER BY fecha DESC", barcode));

  if (res == NULL)
    return "";

  tuples = PQntuples (res);

  if (tuples == 0)
    return "";

  if (strcmp (PQgetvalue (res, 0, 1), "") != 0)
    date = g_strdup_printf ("%.2d/%.2d/%.4d", atoi (PQgetvalue (res, 0, 1)),
                            atoi (PQgetvalue (res, 0, 2)), atoi (PQgetvalue (res, 0, 3)));
  else
    return "";

  return date;
}

gint
InversionAgregada (gchar *barcode)
{
  PGresult *res;
  gdouble stock = 0.0;
  gint i;
  gdouble vendidos = strtod (PUT(GetDataByOne (g_strdup_printf ("SELECT vendidos FROM producto WHERE barcode='%s'",
								barcode))), (char **)NULL);
  gint suma = 0;

  res = EjecutarSQL (g_strdup_printf ("SELECT count(id) FROM compra_detalle WHERE barcode_product='%s'", barcode));

  /*Evita Cuelgue al no recibir resultados (en PUT)*/
  if (PQgetvalue (res, 0, 1) == 0)
    {
      printf("No hay compras ingresadas\n");
      return 0;
    }

  res = EjecutarSQL (g_strdup_printf ("SELECT precio, cantidad_ingresada, (precio * cantidad_ingresada)::integer FROM compra_detalle "
                                      "WHERE barcode_product='%s'", barcode));

  if (res == NULL)
    return 0;

  for (i = 0; stock < vendidos; i++)
    {
      if ((stock + strtod (PUT(PQgetvalue (res, i, 1)), (char **)NULL)) < vendidos)
        {
          stock += strtod (PUT(PQgetvalue (res, i, 1)), (char **)NULL);

          suma += atoi (PQgetvalue (res, i, 2));
        }
      else
        {
          suma += (vendidos - stock) * atoi (PQgetvalue (res, i, 0));
          break;
        }

    }

  return suma;
}

gchar *
InversionTotalStock (void)
{
  PGresult *res;
  gchar *corriente, *materia_prima;

  corriente = g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE'"), 0, "id"));
  materia_prima = g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA'"), 0, "id"));
  res = EjecutarSQL (g_strdup_printf ("SELECT COALESCE(round (SUM (costo_promedio * stock)),0) FROM producto WHERE tipo = %s OR tipo = %s", corriente, materia_prima));

  if (res != NULL)
    return PQgetvalue (res, 0, 0);
  else
    return 0;
}

gchar *
ValorTotalStock (void)
{
  PGresult *res;
  gchar *corriente, *materia_prima;

  corriente = g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE'"), 0, "id"));
  materia_prima = g_strdup (PQvaluebycol (EjecutarSQL ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA'"), 0, "id"));
  res = EjecutarSQL (g_strdup_printf ("SELECT COALESCE(round (SUM (precio * stock)),0) FROM producto WHERE tipo = %s OR tipo = %s", corriente, materia_prima));

  if (res != NULL)
    return PQgetvalue (res, 0, 0);
  else
    return 0;
}


/**
 * Returns the total contribution of stock
 * rounded.
 *
 * @param: void
 * @return: the total contribution of stock rounded
 */
gchar *
ContriTotalStock (void)
{
  PGresult *res;

  res = EjecutarSQL ("SELECT round(monto_contribucion) FROM contribucion_total_stock()");

  if (res != NULL)
    return PQgetvalue (res, 0, 0);
  else
    return 0;
}

void
SetModificacionesProducto (gchar *barcode, gchar *dias_stock, gchar *margen, gchar *new_venta,
                           gboolean canjeable, gint tasa, gboolean mayorista, gchar *precio_mayorista,
                           gchar *cantidad_mayorista)
{
  gchar *q;

  q = g_strdup_printf ("UPDATE producto SET dias_stock=%s, margen_promedio=%s, precio=%s, canje='%d', tasa_canje=%d, "
                       "precio_mayor=%s, cantidad_mayor=%s, mayorista='%d' WHERE barcode='%s'",
                       dias_stock, CUT(margen), CUT (new_venta), (gint)canjeable, tasa, CUT (precio_mayorista), 
		       CUT (cantidad_mayorista), (gint)mayorista, barcode);
  EjecutarSQL (q);
  g_free (q);
}

gboolean
Egresar (gint monto, gint motivo, gint usuario)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("select insert_egreso (%d, %d, %d)",
                       monto, motivo, usuario);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gboolean
SaveVentaTarjeta (gint id_venta, gchar *insti, gchar *numero, gchar *fecha_venc)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("INSERT INTO tarjetas VALUES (DEFAULT, %d, '%s', '%s', '%s')",
                       id_venta, insti, numero, fecha_venc);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gboolean
Ingreso (gint monto, gint motivo, gint usuario)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("select insert_ingreso (%d, %d, %d)",
                       monto, motivo, usuario);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gboolean
PagarFactura (gint id_invoice)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("UPDATE factura_compra SET pagada='t' WHERE id=%d", id_invoice );
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}


/**
 * Registra merma y ajusta el nuevo stock del producto
 *
 * @nuevo_stock: Es el nuevo stock del producto
 * @motivo: La razón por la cual se ha disminuído su stock
 * @barcode: El barcode del producto modificado
 */
void
AjusteStock (gdouble nuevo_stock, gint motivo, gchar *barcode)
{
  gdouble stock = GetCurrentStock (barcode);
  gchar *q;
  gdouble cantidad_merma;

  if (nuevo_stock >= 0 && nuevo_stock <= stock)
    {
      cantidad_merma = stock - nuevo_stock;
      q = g_strdup_printf ("SELECT * FROM registrar_merma (%s, %s, %d)",
			   barcode, CUT (g_strdup_printf ("%.3f", cantidad_merma)), motivo);
      EjecutarSQL (q);
      g_free (q);
    }
  else
    g_printerr ("No se ha registrado la merma, puesto que es mayor al stock actual");
}


gboolean
Asistencia (gint user_id, gboolean entrada)
{
  PGresult *res;
  gchar *q;

  if (entrada == TRUE)
    {
      q = g_strdup_printf ("INSERT INTO asistencia (id_user, entrada) VALUES (%d, NOW())", user_id);
      res = EjecutarSQL (q);
    }
  else
    {
      q = g_strdup_printf ("UPDATE asistencia SET salida=NOW() WHERE id="
                           "(SELECT id FROM asistencia WHERE salida=to_timestamp('0','0') "
                           "AND id_user=%d ORDER BY entrada DESC LIMIT 1)", user_id);
      res = EjecutarSQL (q);
    }
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gboolean
VentaFraccion (gchar *barcode)
{
  PGresult *res;
  gchar *res_char;
  gchar *q;

  q = g_strdup_printf ("SELECT fraccion FROM producto WHERE barcode='%s'", barcode);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL && PQntuples (res) != 0)
    res_char = PQgetvalue (res, 0, 0);
  else
    res_char = "";

  if (strcmp (res_char, "f") == 0)
    return FALSE;
  else if (strcmp (res_char, "t") == 0)
    return TRUE;
  else
    return FALSE;
}

int
AddFormaPagoDB (gchar *forma_name, gchar *forma_days)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("INSERT INTO formas_pago VALUES (DEFAULT, '%s', %s)", forma_name, forma_days);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return 0;
  else
    return -1;
}

gint
AnularCompraDB (gint id_compra)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("UPDATE compra SET anulada='t' WHERE id=%d", id_compra);
  res = EjecutarSQL (q);
  g_free (q);

  // Se igualan la cantidad ingresada y la cantidad solicitada de aquellas que han tenido ingresos
  q = g_strdup_printf ("UPDATE compra_detalle "
		       "SET cantidad = cantidad_ingresada "
		       "WHERE id_compra=%d "
		       "AND candidad_ingresada > 0", id_compra);
  res = EjecutarSQL (q);
  g_free (q);
  
  q = g_strdup_printf ("UPDATE compra_detalle SET anulado='t' WHERE id_compra=%d", id_compra);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return 1;
  else
    return -1;
}

gint
CanjearProduct (gchar *barcode, gdouble cantidad)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf
                     ("UPDATE producto SET stock=stock-%s, stock_pro=stock_pro+%s WHERE barcode=%s",
                      CUT (g_strdup_printf ("%.2f", cantidad)), CUT (g_strdup_printf ("%2.f", cantidad)), barcode));

  res = EjecutarSQL (g_strdup_printf
                     ("INSERT INTO canje VALUES (DEFAULT, NOW (), '%s', %s)",
                      barcode, CUT (g_strdup_printf ("%.2f", cantidad))));

  if (res != NULL)
    return 0;
  else
    return -1;
}

gboolean
Devolver (gchar *barcode, gchar *cantidad)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("UPDATE producto SET stock=stock-%s WHERE barcode='%s'",
                       CUT (cantidad), barcode);
  res = EjecutarSQL (q);
  g_free (q);

  q = g_strdup_printf ("INSERT INTO devolucion (barcode_product, cantidad) VALUES ('%s', %s)",
                       barcode, CUT (cantidad));
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gboolean
Recibir (gchar *barcode, gchar *cantidad)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("UPDATE producto SET stock=stock+%s WHERE barcode=%s",
                       CUT (cantidad), barcode);
  res = EjecutarSQL (q);
  g_free (q);

  q = g_strdup_printf ("UPDATE devolucion SET cantidad_recibida=%s, devuelto='t' "
                       "WHERE id=(SELECT id FROM devolucion WHERE barcode_product=%s AND devuelto='f')",
                       CUT (cantidad), barcode);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gint
SetModificacionesProveedor (gchar *rut, gchar *nombre, gchar *direccion, gchar *comuna,
                            gchar *ciudad, gchar *fono, gchar *web, gchar *contacto,
                            gchar *email, gchar *giro, gchar *lap_rep)
{
  gchar *q;
  gchar **aux;
  gint lrep; // Lapso de reposicion

  lrep = atoi (lap_rep);
  aux = g_strsplit(rut, "-", 0);

  if (lrep == 0)
    return -1;

  q = g_strdup_printf ("UPDATE proveedor SET nombre='%s', direccion='%s', ciudad='%s', "
                       "comuna='%s', telefono='%s', email='%s', web='%s', contacto='%s', giro='%s', lapso_reposicion=%d "
                       "WHERE rut=%s", nombre, direccion, ciudad,
                       comuna, fono, email, web, contacto, giro, lrep, aux[0]);
  EjecutarSQL (q);
  g_free (q);
  g_strfreev(aux);

  return 0;
}

/**
 * Esta funciom  reviza si existe  el rut de un proveedor, a traves de una
 * consulta a la BD
 *
 * @param rut es el rut del proveedor
 * @return 1 si se realizo correctamente la operacion 0 si hay error
 *
 */

gboolean
provider_exist (const gchar *rut)
{
  PGresult *res;
  gchar *q;
  gchar *rut2 = g_strdup(rut);
  q = g_strdup_printf ("SELECT count(*) FROM proveedor WHERE rut=%s", strtok(rut2,"-"));
  res = EjecutarSQL (q);
  g_free (q);
  g_free (rut2);

  if (res == NULL)
    return FALSE;
  else
    if (g_str_equal(PQgetvalue(res, 0, 0), "0"))
      return FALSE;
    else
      return TRUE;
}

gint
users_working (void)
{
  PGresult *res;
  PGresult *res2;
  gint tuples;
  gchar *q;
  gint i;
  gint users_working = 0;

  res = EjecutarSQL("select id from users");

  tuples = PQntuples(res);

  for (i=0 ; i < tuples ; i++)
    {
      q = g_strdup_printf("select salida_year from select_asistencia(%s)",
                          PQvaluebycol(res, i, "id"));
      res2 = EjecutarSQL (q);

      if (PQntuples(res2) == 0)
        {
          g_free (q);
          continue;
        }

      if (g_str_equal (PQvaluebycol(res2, 0, "salida_year"), "-1") || (g_str_equal(PQvaluebycol(res2, 0, "salida_year"), "")))
        users_working++;


      g_free (q);
    }

  return users_working;
}

/**
 * Nullify the given sale.
 *
 * Note: this is dummy function, because the real functionality is
 * implemented inside of a plpgsql function.
 *
 * @param sale_id the sale id
 *
 * @return 0 when was executed without error.
 */
gint
nullify_sale (gint sale_id)
{
  PGresult *res;
  gchar *q;
  gboolean con_egreso;

  /*De estar habilitada caja, se asegura que ésta se encuentre
    abierta al momento de vender*/

  if (rizoma_get_value_boolean ("CAJA"))
    {
      if (check_caja()) // Se abre la caja en caso de que esté cerrada
	open_caja (TRUE);
      con_egreso = TRUE;
    }
  else
    con_egreso = FALSE;

  q = g_strdup_printf("select * from nullify_sale(%d, %d, %s)", 
		      user_data->user_id, sale_id, con_egreso ? "true" : "false");

  res = EjecutarSQL(q);
  g_free (q);

  if ((res != NULL) && (PQntuples(res) == 1) && (g_str_equal(PQgetvalue(res, 0, 0), "t")))
    return 0;

  g_printerr("\n%s: could not be nullified the sale (%d)\n", G_STRFUNC, sale_id);
  return -1;
}

/**
 * Es llamada por la funcion on_btn_devolucion_clicked [ventas.c]
 * Esta funciom  registra los datos en la tabla devolucion
 *
 * @param total entero que contiene el precio total de los productos
 * devueltos al proveedor
 * @param rut es el rut del proveedor
 * @return 1 si se realizo correctamente la operacion 0 si hay error
 */

gboolean
SaveDevolucion (gint total, gint rut)
{
  gint devolucion_id;
  gchar *q;

  q = g_strdup_printf( "SELECT inserted_id FROM registrar_devolucion( %d, %d) ", total, rut);
  devolucion_id = atoi (GetDataByOne (q));
  g_free (q);

  SaveProductsDevolucion (venta->header, devolucion_id);

  return TRUE;
}

/**
 * Es llamada por la funcion SaveDevolucion
 * Esta funciom descuenta del stock los productos devueltos al proveedor y
 * luego registra los productos devueltos al proveedor, en la tabla devolucion_detalle
 *
 * @param products puntero que apunta a los valores del producto que se va vender
 * @param id_devolucion  es el id de la devolucion que se esta realizando
 * @return 1 si se realizo correctamente la operacion 0 si hay error
 */

gboolean
SaveProductsDevolucion (Productos *products, gint id_devolucion)
{
  PGresult *res;
  Productos *header = products;
  gchar *cantidad;
  gint precio;
  gdouble precioCompra;
  gchar *q;
  gdouble pre;
  do
    {
      cantidad = CUT (g_strdup_printf ("%.3f", products->product->cantidad));
      res = EjecutarSQL
        (g_strdup_printf
         ("UPDATE producto SET stock=%s WHERE barcode='%s'",
          CUT (g_strdup_printf ("%.3f", (gdouble)GetCurrentStock (products->product->barcode) - products->product->cantidad)), products->product->barcode));
      /*      res = EjecutarSQL (g_strdup_printf
              ("UPDATE productos SET stock=stock-%s WHERE barcode='%s'",
              cantidad, products->product->barcode));
      */

      precioCompra = products->product->precio_compra;
      precio = products->product->precio;

      if (lround(precioCompra) == -1)
        {
          q = g_strdup_printf ("select * from informacion_producto (%s, '')", products->product->barcode);
          res = EjecutarSQL (q);
          pre = strtod (PUT(PQvaluebycol(res, 0, "costo_promedio")), (char **)NULL);

          q = g_strdup_printf ("select registrar_devolucion_detalle(%d, %s, %s, %d, %s)",
                               id_devolucion, products->product->barcode, cantidad, precio,
			       CUT (g_strdup_printf ("%.3f", pre)));
        }
      else
         q = g_strdup_printf ("select registrar_devolucion_detalle(%d, %s, %s, %d, %s)",
                              id_devolucion, products->product->barcode, cantidad, precio,
			      CUT (g_strdup_printf ("%.3f", precioCompra)));

      res = EjecutarSQL (q);
      g_free (q);

      products = products->next;
    }
  while (products != header);

  return TRUE;
}

/**
 * Es llamada por la funcion on_enviar_button_clicked() [ventas.c]
 *
 * Esta funciom  registra los datos en la tabla traspaso y llama a la funcion SaveProductsTraspaso()
 *
 * @param total entero que contiene el precio total de los productos
 * devueltos al proveedor
 * @param rut es el rut del proveedor
 * @return retorna el id de traspaso
 */
gint
SaveTraspaso (gdouble total, gint origen, gint vendedor, gint destino, gboolean tipo_traspaso)
{
  gint traspaso_id;
  gchar *q;

  q = g_strdup_printf( "SELECT inserted_id FROM registrar_traspaso( %s, %d, %d, %d) ",
                       CUT (g_strdup_printf ("%.3f", total)), origen, destino, vendedor);
  traspaso_id = atoi (GetDataByOne (q));
  g_free (q);

  SaveProductsTraspaso (venta->header, traspaso_id, tipo_traspaso);

  return traspaso_id;
}

/**
 * Es llamada por la funcion on_enviar_button_clicked() y
 * on_recibir_button_clicked() [compras.c]
 *
 * Esta funciom  registra los datos en la tabla traspaso y llama a la funcion SaveProductsTraspaso()
 *
 * @param total entero que contiene el precio total de los productos a traspasar
 * @param origen id del destino del traspaso
 * @param vendedor id del destino del traspaso
 * @param destino id del destino del traspaso
 * @param tipo_traspaso: 1 si es un traspaso de enviar 0  si es un trapaso de recibir
 * @return Retorna el id de traspaso
 */

gint
SaveTraspasoCompras (gdouble total, gint origen, gint vendedor, gint destino, gboolean tipo_traspaso)
{
  gint traspaso_id;
  gchar *q;

  q = g_strdup_printf ("SELECT inserted_id FROM registrar_traspaso( %s, %d, %d, %d)",
                       CUT (g_strdup_printf ("%.3f", total)), origen, destino, vendedor);
  traspaso_id = atoi (GetDataByOne (q));
  g_free (q);

  SaveProductsTraspaso (compra->header_compra, traspaso_id, tipo_traspaso);
  return traspaso_id;
}

/**
 * Es llamada por las funciones SaveTraspaso() y SaveTraspasoCompras()
 *
 * Esta funcion descuenta  o agrega del stock los productos traspasados y
 * luego registra los productos traspasados, en la tabla devolucion_detalle
 *
 * @param products puntero que apunta a los valores del producto que se va vender
 * @param id_traspaso es el id del traspaso  que se esta realizando
 * @param traspaso_envio: 1 si es un traspaso de enviar 0  si es un trapaso de recibir
 * @return 1 si se realizo correctamente la operacion 0 si hay error
 */

gboolean
SaveProductsTraspaso (Productos *products, gint id_traspaso, gboolean traspaso_envio)
{
  PGresult *res;
  Productos *header = products;
  gchar *cantidad, *precio_venta;
  gchar *q;
  gdouble costo;

  do
    {
      cantidad = CUT (g_strdup_printf ("%.3f", products->product->cantidad));
      precio_venta = CUT (g_strdup_printf ("%.2f", products->product->precio));

      q = g_strdup_printf ("select * from informacion_producto (%s, '')", products->product->barcode);
      res = EjecutarSQL (q);
      costo = strtod (PUT (PQvaluebycol (res, 0, "costo_promedio")), (char **)NULL);
      
      if (costo == (gdouble)0)
	{
	  costo = products->product->precio_compra;
	  EjecutarSQL (q);
	  
	  q = g_strdup_printf ("SELECT registrar_traspaso_detalle (%d, %s, %s, %s, %s, %s, 't')",
			       id_traspaso, products->product->barcode, cantidad,
			       CUT (g_strdup_printf ("%.2f", costo)), precio_venta,
			       (traspaso_envio)?"TRUE":"FALSE");
	}
      else
	{
	  q = g_strdup_printf ("SELECT registrar_traspaso_detalle (%d, %s, %s, %s, %s, %s, 'f')",
			       id_traspaso, products->product->barcode, cantidad,
			       CUT (g_strdup_printf ("%.2f", costo)), precio_venta,
			       (traspaso_envio)?"TRUE":"FALSE");
	}

      res = EjecutarSQL (q);
      g_free (q);

      products = products->next;
    }
  while (products != header);

  return TRUE;
}

/**
 * Es llamada por las funciones  DatosEnviar() de  [ventas.c] y
 * DatosEnviar(), DatosRecibir()  de [compras.c].
 *
 * Consulta por el ultimo id de traspaso y lo retorna
 *
 * @return  el resultado de la consulta id del ultimo traspaso
 */

gint
InsertIdTraspaso ()
{
  PGresult *res;
  res = EjecutarSQL (g_strdup_printf ("SELECT MAX(id) FROM traspaso"));
  return atoi (PQgetvalue (res, 0, 0));
}


/**
 * Es llamada por las funciones on_enviar_button_clicked() de [ventas.c]
 * y on_enviar_button_clicked(),on_recibir_button_clicked() de  [compras.c].
 *
 * Esta funcion hace una consulta por el nombre del negocio y le retorna el
 * ide de este
 *
 * @return  el resultado de la consulta: el id del negocio
 */
gint
ReturnBodegaID (gchar *destino)
{

  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT id FROM bodega WHERE nombre='%s'",destino);
  res = EjecutarSQL (q);
  g_free (q);

  return atoi (PQgetvalue (res, 0, 0));
}


/**
 * Es llamada por las funciones  DatosEnviar(), on_enviar_button_clicked()de
 * [ventas.c] y  DatosEnviar(), DatosRecibir(),on_enviar_button_clicked(),
 * on_recibir_button_clicked() de [compras.c]
 *
 * Esta funcion realiza una consulta por el nombre del negocio y lo retorna
 *
 * @return  el resultado de la consulta el nombre del negocio
 */

gchar *
ReturnNegocio ()
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT nombre FROM negocio"));

  return ((gchar *)(PQgetvalue (res, 0, 0)));
}


/**
 * Es llamada por las funciones on_enviar_button_clicked() [ventas.c] y
 * DatosEnviar() [ventas.c].
 *
 * Esta funcion recibe el puntero del productos, luego apunta al barcode de
 * cada uno, y a  traves de una consulta a la BD obtiene los precio de
 * compras, para luego sumarlos y obtener el total.
 *
 * @param products puntero que apunta a los valores del producto que se va
 * vender
 * @return total suma de los precios de compra de los productos
 */

gdouble
TotalPrecioCompra (Productos *products)
{
  Productos *header = products;
  gdouble total=0;
  gdouble pre;
  PGresult *res;
  gchar *q;
  do
    {
      q = g_strdup_printf ("select * from informacion_producto (%s, '')", products->product->barcode);
      res = EjecutarSQL (q);
      pre = strtod (PUT (PQvaluebycol (res, 0, "costo_promedio")), (char **)NULL) * products->product->cantidad;
      total = total + pre;
      products = products->next;
    }
  while (products != header);

  return total;
}


/**
 * Es llamada por AskProductProvider de [compras.c].
 *
 * @return PGresult: La respuesta a a la consulta
 * (todos los productos comprados a X proveedor)
 */

PGresult *getProductsByProvider (gchar *rut)
{
  PGresult *res;
  gchar *q;

  //TODO: Ojo con el MAX (id_compra), evaluar el uso de last_value  
  /*q = g_strdup_printf("SELECT DISTINCT p.barcode, p.codigo_corto, p.descripcion, p.marca, p.contenido, "
		      "                p.unidad, p.precio, p.stock, p.dias_stock, p.fraccion, "
		      "       (select_ventas_dia(p.barcode, FALSE)::float) AS ventas_dia, "
		      "       (stock::float / select_ventas_dia(p.barcode, TRUE)::float) AS stock_day, "
		      "	      (SELECT precio "
		      "	       FROM compra_detalle "
		      "	       WHERE id_compra = (SELECT MAX (id_compra) FROM compra_detalle WHERE barcode_product=p.barcode) "
		      "	             AND barcode_product = p.barcode) AS precio_compra, "
		      "	      (SELECT SUM (cantidad-cantidad_ingresada) "
		      "	       FROM compra_detalle "
		      "	            INNER JOIN compra ON compra.id = compra_detalle.id_compra "
		      "	       WHERE barcode_product = p.barcode "
		      "		     AND compra.anulada = 'f' "
		      "		     AND compra.anulada_pi = 'f' "
		      "		     AND compra.ingresada = 'f') AS cantidad_pedido "
		      "FROM producto p "
		      "     INNER JOIN compra_detalle cd "
		      "           ON p.barcode = cd.barcode_product "
		      "     INNER JOIN compra c "
		      "           ON c.id = cd.id_compra "
		      "     INNER JOIN proveedor pr "
		      "           ON pr.rut = c.rut_proveedor "
		      "WHERE pr.rut = %s "
		      "      AND c.anulada = false "
		      "      AND c.anulada_pi = false "
		      "      AND estado = true "
		      "      AND ingresada = true "
		      "      AND (c.fecha BETWEEN now() - '2 month'::interval AND now())", rut);*/

  q = g_strdup_printf ("SELECT p.barcode, p.codigo_corto, p.descripcion, p.marca, p.contenido, p.unidad, "
		       "       p.precio, p.stock, p.dias_stock, p.fraccion, pp.costo_compra AS precio_compra, "
		       "       (SELECT_ventas_dia(p.barcode, FALSE)::float) AS ventas_dia, "
		       "       (stock::float / select_ventas_dia(p.barcode, TRUE)::float) AS stock_day, "
		       "       (SELECT SUM (cantidad-cantidad_ingresada) "
		       "	       FROM compra_detalle "
		       "	            INNER JOIN compra ON compra.id = compra_detalle.id_compra "
		       "	       WHERE barcode_product = p.barcode "
		       "		     AND compra.anulada = 'f' "
		       "		     AND compra.anulada_pi = 'f' "
		       "		     AND compra.ingresada = 'f') AS cantidad_pedido "
		       "FROM producto p INNER JOIN producto_proveedor pp "
		       "                ON p.barcode = pp.barcode_producto "
		       "                AND pp.rut_proveedor = %s", rut);
  
  
  res = EjecutarSQL (q);
  g_free (q);
  return res;
}


/**
 * Return the last cash id
 *
 * @param: void
 * @return: gint (the cash id)
 */

gint
get_last_cash_box_id (void)
{
  PGresult *res;
  res = EjecutarSQL("SELECT last_value FROM caja_id_seq");

  if ((res != NULL) && (PQntuples (res) > 0))
    return atoi (PQgetvalue (res, 0, 0));
  else
    return -1;
}

/**
 * Return the PGresult with the information
 * from query
 *
 * @param: (gchar *) The product's 'barcode'
 * @param: (gchar *) The product's 'codigo_corto'
 * @param: (gchar *) The product's 'columns' to get (void string is replaced by *)
 * @return: (PGresult *) The result from query
 */

PGresult *
get_product_information (gchar *barcode, gchar *codigo_corto, gchar *columnas)
{
  PGresult *res;

  if (g_str_equal (columnas, ""))
    columnas = g_strdup_printf ("*");

  if (!g_str_equal (barcode, ""))
    res = EjecutarSQL (g_strdup_printf ("SELECT %s FROM informacion_producto (%s, '')", columnas, barcode));
  else if (!g_str_equal (codigo_corto, ""))
    res = EjecutarSQL (g_strdup_printf ("SELECT %s FROM informacion_producto (0, '%s')", columnas, codigo_corto));

  if (res == NULL)
    {
      printf ("No se pudo obtener la informacion del producto \n"
	      "columnas = %s \n"
	      "barcode = %s \n"
	      "codigo_corto = %s \n", columnas, barcode, codigo_corto);
      return res;
    }
  else
    return res;
}


/**
 * If the code doesn't was registered previously,
 * this function save the clothes code
 * and all its components into 'clothes_code' table
 *
 * @param gchar *codigo: The code to save into clothes_code table
 */
void
registrar_nuevo_codigo (gchar *codigo)
{
  GArray *fragmentos;
  gchar *q;

  fragmentos = decode_clothes_code (codigo);

  if (!DataExist (g_strdup_printf ("SELECT * FROM clothes_code WHERE codigo_corto = '%s'", codigo)))
    {
      q = g_strdup_printf ("INSERT INTO clothes_code (codigo_corto, depto, temp, ano, sub_depto, id_ropa, talla, color) "
			   "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')",
			   codigo,
			   g_array_index (fragmentos, gchar*, 0), g_array_index (fragmentos, gchar*, 1),
			   g_array_index (fragmentos, gchar*, 2), g_array_index (fragmentos, gchar*, 3),
			   g_array_index (fragmentos, gchar*, 4), g_array_index (fragmentos, gchar*, 5),
			   g_array_index (fragmentos, gchar*, 6));
      printf ("%s", q);
      EjecutarSQL (q);
    }
  else
    printf ("No se registró el código %s, puesto que ya existe", codigo);

  g_array_free (fragmentos, TRUE);
}

/**
 * If the code doesn't was registered previously,
 * this function save the color code and color name
 * into 'color' table
 *
 * @param gchar *codigo: The color code
 * @param gchar *color: The color name
 */
void
registrar_nuevo_color (gchar *codigo, gchar *color)
{
  gchar *q;

  if (!DataExist (g_strdup_printf ("SELECT * FROM color WHERE codigo = '%s'", codigo)))
    {
      q = g_strdup_printf ("INSERT INTO color (codigo, nombre) "
			   "VALUES ('%s', '%s')", codigo, color);
      printf ("%s", q);
      EjecutarSQL (q);
    }
  else
    printf ("No se registró el color %s, puesto que ya existe", color);
}


/**
 * If the code doesn't was registered previously,
 * this function save the color code and color name
 * into 'color' table
 *
 * @param gchar *codigo: The color code
 * @param gchar *talla: The size name
 */
void
registrar_nueva_talla (gchar *codigo, gchar *talla)
{
  gchar *q;

  if (!DataExist (g_strdup_printf ("SELECT * FROM talla WHERE codigo = '%s'", codigo)))
    {
      q = g_strdup_printf ("INSERT INTO talla (codigo, nombre) "
			   "VALUES ('%s', '%s')", codigo, talla);
      printf ("%s", q);
      EjecutarSQL (q);
    }
  else
    printf ("No se registró la talla %s, puesto que ya existe", talla);
}


/**
 * If the code doesn't was registered previously,
 * this function save the sub_depto code and sub_depto name
 * into 'sub_depto' table
 *
 * @param gchar *codigo: The sub_depto code
 * @param gchar *sub_depto: The sub_depto name
 */
void
registrar_nuevo_sub_depto (gchar *codigo, gchar *sub_depto)
{
  gchar *q;

  if (!DataExist (g_strdup_printf ("SELECT * FROM sub_depto WHERE codigo = '%s'", codigo)))
    {
      q = g_strdup_printf ("INSERT INTO sub_depto (codigo, nombre) "
			   "VALUES ('%s', '%s')", codigo, sub_depto);
      printf ("%s", q);
      EjecutarSQL (q);
    }
  else
    printf ("No se registró el sub_depto %s, puesto que ya existe", sub_depto);
}


/**
 * This function save the association
 * between the mother and (derived or components) merchandise
 *
 * @param : gchar *barcode_complejo
 * @param : gint tipo_complejo
 * @param : gchar *barcode_componente
 * @param : gint tipo_componente
 * @param : gdouble cant_mud : Cantidad de madre que usa el derivado
 * @return: gboolean: TRUE si realizó algún cambio, FALSE si no realizó cambios
 */
gboolean
asociar_componente_o_derivado (gchar *barcode_complejo, gint tipo_complejo,
			       gchar *barcode_componente, gint tipo_componente,
			       gdouble cant_mud)
{
  PGresult *res;
  gchar *q;
  gdouble cant_mud_l;

  // Se comprueba si ya existe la asociación
  q = g_strdup_printf ("SELECT cant_mud FROM componente_mc "
		       "WHERE barcode_complejo = %s AND barcode_componente = %s", barcode_complejo, barcode_componente);
  res = EjecutarSQL(q);
  g_free(q);

  /*Si no existe la asociación*/
  if (res == NULL || PQntuples (res) == 0)
    {
      q = g_strdup_printf ("INSERT INTO componente_mc (id, barcode_complejo, tipo_complejo, barcode_componente, tipo_componente, cant_mud) "
			   "                   VALUES (DEFAULT,   %s,             %d,              %s,                  %d,         %s   ) ", 
			   barcode_complejo, tipo_complejo, barcode_componente, tipo_componente, CUT (g_strdup_printf ("%.3f", cant_mud)));
      res = EjecutarSQL(q);
      g_free(q);

      if (res == NULL)
	return FALSE;
      else
	printf ("Se asoció %s con %s\n", barcode_complejo, barcode_componente);
    }
  else
    {
      cant_mud_l = strtod (PUT (g_strdup (PQgetvalue (res, 0, 0))), (char **)NULL);
      if (cant_mud_l != cant_mud && cant_mud > 0)
	{
	  q = g_strdup_printf ("UPDATE componente_mc SET cant_mud = %s "
			       "WHERE barcode_complejo = %s AND barcode_componente = %s",
			       CUT (g_strdup_printf ("%.3f", cant_mud)), barcode_complejo, barcode_componente);
	  res = EjecutarSQL(q);
	  g_free(q);

	  if (res == NULL)
	    return FALSE;
	  else
	    printf ("Se actualizó la cant_mud de %s con %s en %s\n", 
		    barcode_complejo, barcode_componente, CUT (g_strdup_printf ("%.3f", cant_mud)));
	}
      else
	return FALSE;
    }
  return TRUE;
}


/**
 * This function detaches the specified goods 
 * 
 * @param: gchar * barcode_madre
 * @param: gchar * barcode_hijo
 */
gboolean
desasociar_componente_compuesto (gchar *barcode_complejo, gchar * barcode_componente)
{
  PGresult *res;
  gchar *q;
  gint id_derivado;
  
  /*Se obtiene el id de la mercadería derivada*/
  id_derivado = atoi (PQvaluebycol (EjecutarSQL 
				    (g_strdup ("SELECT id FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA'")), 
				    0, "id"));

  q = g_strdup_printf ("DELETE FROM componente_mc WHERE barcode_complejo = %s AND barcode_componente = %s RETURNING tipo_complejo",
		       barcode_complejo, barcode_componente);

  res = EjecutarSQL(q);
  g_free(q);

  if (res == NULL)
    return FALSE;

  printf ("Se desasoció %s con %s\n", barcode_complejo, barcode_componente);

  //Si la mercadería es derivada, se debe "deshabilitar"
  if (atoi (g_strdup (PQvaluebycol (res, 0, "tipo_complejo"))) == id_derivado)
    {
      res = EjecutarSQL (g_strdup_printf ("UPDATE producto SET estado = false WHERE barcode = %s", barcode_complejo));
      if (res == NULL) return FALSE;
      printf ("Se des-habilitó el producto %s\n", barcode_complejo);
    }

  return TRUE;
}


/**
 * This function return the available code
 * with the max lenght specified.
 *
 * @param : gchar *codigo   : The code to look at
 * @param : gint min_lenght : The minimum code lenght
 * @param : gint max_lenght : The maximum code lenght
 *
 * @return : gchar *: The available code  
 */
gchar *
sugerir_codigo (gchar *codigo, guint min_lenght, guint max_lenght)
{
  gboolean maximo = FALSE;
  gchar *code = g_strdup (codigo);
  long double code_num;

  /*Comprobaciones iniciales*/
  //longitud de los tamaños maximos y minimos
  if (max_lenght > 18 || min_lenght < 1 ||
      max_lenght < 1  || max_lenght < min_lenght)
    return g_strdup ("");
  if (g_str_equal (code, "") || strlen (code) > 18)
    return g_strdup ("");

  /*Para codigos solamente numéricos*/
  if (!HaveCharacters (code))
    {      
      //Se prepara el relleno (agregan 0 hasta quedar del largo mínimo)
      while (strlen (code) < min_lenght)
	code = g_strdup_printf ("%s%d", code, 0);
      
      //Se parsea a dato numérico
      code_num = strtod (code, (char **)NULL);
      
      //Revisa la existencia del barcode o codigo_corto, de ser así va aumentando el número del último dígito hasta ser único
      while (DataExist (g_strdup_printf ("SELECT barcode FROM producto "
					 "WHERE barcode = %ld OR codigo_corto = '%ld'", lroundl(code_num), lroundl(code_num))))
	{
	  if (strlen (g_strdup_printf ("%ld", lroundl (code_num))) > max_lenght)
	    maximo = FALSE;

	  if (maximo == FALSE)
	    code_num++;
	  else
	    code_num--;
	}

      code = g_strdup_printf ("%ld", lroundl (code_num));
      return code;
    }
  else
    {
      gboolean first = TRUE;
      //Se prepara el relleno (agregan 0 hasta quedar del largo mínimo)
      while (strlen (code) < min_lenght)
	code = g_strdup_printf ("%s%d", code, 0);      
      
      gint largo_inicial = strlen (code);
      gchar *aux;
      //Revisa la existencia del barcode o codigo_corto, de ser así va aumentando el número del último dígito hasta ser único
      while (DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE codigo_corto = '%s'", code)))
	{
	  if (strlen (code) > max_lenght)
	    maximo = FALSE;

	  if (maximo == FALSE)
	    {
	      if (!HaveCharacters (g_strdup_printf ("%c", code [largo_inicial-1])))
		{
		  //Parte desde el numero 0
		  aux = (first == TRUE) ? g_strdup ("0") : invested_strndup (code, largo_inicial-1);
		  first = FALSE;
		  code = g_strdup_printf ("%s%d", g_strndup (code, largo_inicial-1), atoi (aux)+1);
		}
	      else
		{
		  //Parte desde la a
		  aux = (first == TRUE) ? g_strdup ("a") : invested_strndup (code, largo_inicial-1);
		  first = FALSE;

		  if (!g_str_equal (aux, "z"))
		    code = g_strdup_printf ("%s%c", g_strndup (code, largo_inicial-1), atoi (aux)+1);
		  else
		    return "";
		}
	    }
	  
	  return code;
	}
    }

  return "";
}

/**
 * This function returns the code availability
 * 
 * @param: gchar *code: The code to looking at
 * @return: gboolean: return TRUE if the code is available, else return FALSE;
 */
gboolean
codigo_disponible (gchar *code)
{
  if (strlen (code) > 18)
    return FALSE;

  if (!HaveCharacters (code))
    {
      if (!DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE barcode = %s "
				       "OR codigo_corto = '%s'", code, code)))
	return TRUE;
    }
  else
    {
      if (!DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE codigo_corto = '%s'", code)))
	return TRUE;
    }

  return FALSE;
}

/**
 * 
 * 
 * 
 */
gdouble
get_last_buy_price (gchar *barcode)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT cd.precio AS costo "
		       "FROM compra c INNER JOIN compra_detalle cd "
		       "ON c.id = cd.id_compra "
		       "INNER JOIN producto p "
		       "ON cd.barcode_product = p.barcode "
		       "WHERE c.anulada = false "
		       "AND c.anulada_pi = false "
		       "AND p.barcode = '%s' "
		       "AND cd.anulado = false "
		       "ORDER BY c.fecha DESC", barcode);

  res = EjecutarSQL (q);

  if ((res != NULL) && (PQntuples (res) > 0))
    return strtod (PUT(PQvaluebycol (res, 0, "costo")), (char **)NULL);
  else
    return 0;
}


/**
 *
 *
 *
 */
gdouble
get_last_buy_price_to_invoice (gchar *barcode, gint last_invoice_id)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT precio AS costo "
		       "FROM compra c "
		       "INNER JOIN factura_compra fc "
		       "ON c.id = fc.id_compra "
		       "INNER JOIN factura_compra_detalle fcd "
		       "ON fc.id = fcd.id_factura_compra "
		       "WHERE c.anulada_pi = false "
		       "AND fcd.barcode = %s "
		       "AND fc.id <= %d "
		       "ORDER BY fc.fecha DESC",
		       barcode, last_invoice_id);

  res = EjecutarSQL (q);

  if ((res != NULL) && (PQntuples (res) > 0))
    return strtod (PUT(PQvaluebycol (res, 0, "costo")), (char **)NULL);
  else
    return 0;
}

/**
 * Indica si la cantidad del producto de la factura que se pretende 
 * modificar es editable, de lo contrario retorna la cantidad minima para
 * que sea modificable (como número negativo).
 * 
 * @param: gchar *barcode = Codigo de barras del producto a revisar
 * @param: gdouble cantidad_nueva = La nueva cantidad que se desea setear
 * @param: gint id_factura_compra = El id de la factura compra cuyo detalle se desea modificar
 *
 * @return: gdouble: 1 si es posible, (-N) si no lo es. (N <= 0)
 * (-N indica la cantidad minima necesaria para que sea modificable)
 */
gdouble
cantidad_compra_es_modificable (gchar *barcode, gdouble cantidad_nueva, gint id_factura_compra)
{
  gint tuplas, i;
  gdouble cantidad_original, cantidad_pre_ingreso, cantidad_min, resto;
  gchar *query;
  PGresult *res;

  /*Si se esta verificando una compra a modificar*/
  query = g_strdup_printf ("SELECT cantidad "
			   "FROM factura_compra_detalle fcd "
			   "WHERE id_factura_compra = %d "
			   "AND barcode = %s", id_factura_compra, barcode);
  
  res = EjecutarSQL (query);
  g_free (query);

  if (PQntuples (res) > 0)
    cantidad_original = strtod (PUT (PQvaluebycol (res, 0, "cantidad")), (char **)NULL);
  else /*Si no existe en la factura, quiere decir que es un producto que se esta agregando a ella*/
    return 1;

  /* Si se busca disminuir la cantidad comprada
     se debe comprobar que la cantidad logre justificar las transacciones futuras */
  if (cantidad_original > 0 && (cantidad_original > cantidad_nueva))
    {
      if (cantidad_nueva <= 0)
	return 0;

      //Se obtiene la cantidad que se prentende disminuir de la cantidad original
      resto = cantidad_original - cantidad_nueva;

      /*Se obtiene la informacion de las compras posteriores a esta*/
      query = g_strdup_printf ("SELECT cantidad_pre_ingreso AS cantidad "
			       "FROM product_on_ingress (%s) "
			       "WHERE id_fc_out > %d "
			       "OR (id_fc_out = 0 AND id_t_out = 0)", barcode, id_factura_compra);

      //Cantidad - (cantidad restada) debe ser >= a 0 en todas las compras siguientes
      res = EjecutarSQL (query);
      g_free (query);
      tuplas = PQntuples (res);
  
      //Recorre todas las compras revisando el stock del producto entes de realizada cada una de ellas
      for (i = 0; i < tuplas; i++)
	{
	  cantidad_pre_ingreso = strtod (PUT (PQvaluebycol (res, i, "cantidad")), (char **)NULL);

	  /*Nota: cantidad_min es el máximo de unidades que se puede disminuir la compra
	          del producto especificado
	  */

	  //Se obtiene el minimo de las cantidades
	  if (i == 0 || cantidad_min > cantidad_pre_ingreso)
	    cantidad_min = cantidad_pre_ingreso;
	}
      
      // Si por X motivo cantidad_min es negativo, se retorna 0
      if (cantidad_min <= 0)
	return 0;

      /* Si la cantidad a disminuir (resto) es mayor al stock minimo anterior a 
	 una compra, se retorna el valor minimo (como un valor negativo) */      
      else if (cantidad_min < resto)
	return (cantidad_min * -1);

      else //Se retorna 1 si es modificable
	return 1;
    }
  
  else if (cantidad_original > 0 
	   && (cantidad_original < cantidad_nueva))
    return 1;

  else if (cantidad_original == cantidad_nueva)
    return 1;

  else // if (cantidad_original <= 0)
    return 0;    
}

/**
 * Indica si la cantidad del producto del traspaso que se pretende 
 * modificar es editable.
 * 
 * @param: gchar *barcode = Codigo de barras del producto a revisar
 * @param: gdouble cantidad_nueva = La nueva cantidad que se desea setear
 * @param: gint id_factura_compra = El id de la factura compra cuyo detalle se desea modificar
 *
 * @return: gdouble: 1 si es posible, (-N) si no lo es. (N <= 0)
 * (-N indica la cantidad minima necesaria para que sea modificable)
 */
gdouble
cantidad_traspaso_es_modificable (gchar *barcode, gdouble cantidad_original, 
				  gdouble cantidad_nueva, gint id_traspaso, int origen)
{
  gint tuplas, i;
  gdouble cantidad_a_disminuir, cantidad_pre_ingreso, 
          cantidad_min;
  gchar *query;
  PGresult *res;

  /*SI se recibió más cantidad o se envió menos cantidad es modificable*/
  if ( (origen != 1 && cantidad_nueva > cantidad_original) ||
       (origen == 1 && cantidad_nueva < cantidad_original)  )
    return 1;

  /*cantidad a disminuir*/

  /*Si es una recepcion*/
  if (origen != 1)
    cantidad_a_disminuir = cantidad_original - cantidad_nueva;
  else /*si es un envio*/
    cantidad_a_disminuir = cantidad_nueva - cantidad_original;

  /*Si se esta verificando una compra a modificar*/
  query = g_strdup_printf ("SELECT fecha "
			   "FROM traspaso "
			   "WHERE id = %d ", id_traspaso);

  res = EjecutarSQL (query);
  g_free (query);

  if (PQntuples (res) <= 0)
    return 0;

  /*Se obtiene la informacion de las compras posteriores a esta*/
  query = g_strdup_printf ("SELECT cantidad_pre_ingreso AS cantidad "
			   "FROM product_on_ingress (%s) "
			   "WHERE id_fc_out > %d "
			   "OR (id_fc_out = 0 AND id_t_out = 0)", barcode, id_traspaso);

  //Cantidad - (cantidad restada) debe ser >= a 0 en todas las compras siguientes
  res = EjecutarSQL (query);
  g_free (query);
  tuplas = PQntuples (res);
  
  //Recorre todas las compras revisando el stock del producto entes de realizada cada una de ellas
  for (i = 0; i < tuplas; i++)
    {
      cantidad_pre_ingreso = strtod (PUT (PQvaluebycol (res, i, "cantidad")), (char **)NULL);

      /*Nota: cantidad_min es el máximo de unidades que se puede disminuir la compra
	del producto especificado
      */

      //Se obtiene el minimo de las cantidades
      if (i == 0 || cantidad_min > cantidad_pre_ingreso)
	cantidad_min = cantidad_pre_ingreso;
    }
      
  // Si por X motivo cantidad_min es negativo, se retorna 0
  if (cantidad_min <= 0)
    return 0;

  /* Si la cantidad a disminuir (resto) es mayor al stock minimo anterior a 
     una compra, se retorna el valor minimo (como un valor negativo) */      
  else if (cantidad_min < cantidad_a_disminuir)
    return ((cantidad_original - cantidad_min) * -1);

  else //Se retorna 1 si es modificable
    return 1;
}


/**
 *
 *
 *
 *
 */
gboolean
mod_to_mod_on_buy (Prod *producto)
{
  gchar *q;
  PGresult *res;
  gboolean entro;
  
  if (producto->accion != MOD)
    return FALSE;

  /*Se actualiza la cantidad en la tabla factura_compra_detalle*/
  if ((producto->cantidad_original != producto->cantidad_nueva) &&
      (producto->cantidad_original > 0 && producto->cantidad_nueva > 0))
    {
      q = g_strdup_printf ("UPDATE factura_compra_detalle "
			   "SET cantidad = %s "
			   "WHERE id_factura_compra = %d "
			   "AND barcode = %s",
			   CUT (g_strdup_printf ("%.3f", producto->cantidad_nueva)),
			   producto->id_factura_compra,
			   producto->barcode);
      res = EjecutarSQL (q);
      g_free (q);
      
      if (res == NULL)
	return FALSE;
      entro = TRUE;
    }

  /*Se actualiza costo en la tabla factura_compra_detalle*/
  if ((producto->costo_original != producto->costo_nuevo) &&
      (producto->costo_original > 0 && producto->costo_nuevo > 0))
    {
      q = g_strdup_printf ("UPDATE factura_compra_detalle "
			   "SET precio = %s "
			   "WHERE id_factura_compra = %d "
			   "AND barcode = %s",
			   CUT (g_strdup_printf ("%.3f", producto->costo_nuevo)),
			   producto->id_factura_compra,
			   producto->barcode);
      res = EjecutarSQL (q);
      g_free (q);
      
      if (res == NULL)
	return FALSE;
      entro = TRUE;
    }

  /*Se actualizan los promedios en las tablas correspondientes*/
  if (entro == TRUE)
    {
      q = g_strdup_printf ("SELECT * FROM update_avg_cost (%s, %d)",
			   producto->barcode,
			   producto->id_factura_compra);
      res = EjecutarSQL (q);
      g_free (q);

      if (res == NULL)
	return FALSE;
    }
  
  return TRUE;
}


/**
 *
 *
 *
 *
 */
gboolean
mod_to_add_on_buy (Prod *producto)
{
  gchar *q;
  PGresult *res;
  
  if (producto->accion != ADD)
    return FALSE;

  /*Se ingresa un producto en factura_compra_detalle*/
  if (producto->cantidad_nueva > 0 && producto->costo_nuevo > 0)
    {
      /*Los 3 últimos rellenará update_avg_cost*/
      q = g_strdup_printf ("INSERT INTO factura_compra_detalle "
			   "VALUES (DEFAULT, %d, %s, %s, %s, 0, 0, 0)",
			   producto->id_factura_compra,
			   producto->barcode,
			   CUT (g_strdup_printf ("%.3f",producto->cantidad_nueva)),
			   CUT (g_strdup_printf ("%.3f",producto->costo_nuevo)));
      res = EjecutarSQL (q);
      g_free (q);
      
      if (res == NULL)
	return FALSE;

      /*Se actualizan los promedios en las tablas correspondientes*/
      q = g_strdup_printf ("SELECT * FROM update_avg_cost (%s, %d)",
			   producto->barcode,
			   producto->id_factura_compra);
      res = EjecutarSQL (q);
      g_free (q);

      if (res == NULL)
	return FALSE;
    }

  return TRUE;
}


/**
 *
 *
 *
 *
 */
gboolean
mod_to_del_on_buy (Prod *producto)
{
  gchar *q;
  PGresult *res;
  gint cantidad_productos;

  if (producto->accion != DEL)
    return FALSE;

  /*Se elimina el producto de factura_compra_detalle*/
  q = g_strdup_printf ("DELETE FROM factura_compra_detalle "
		       "WHERE id_factura_compra = %d "
		       "AND barcode = %s",
		       producto->id_factura_compra,
		       producto->barcode);
  res = EjecutarSQL (q);
  g_free (q);
  
  if (res == NULL)
    return FALSE;

  /*cantidad ingresada en compra_detalle debe ser de 0*/
  q = g_strdup_printf ("UPDATE compra_detalle "
		       "SET cantidad_ingresada = 0 "
		       "WHERE id_compra IN (SELECT id_compra FROM factura_compra WHERE id = %d) "
		       "AND barcode_product = %s",
		       producto->id_factura_compra,
		       producto->barcode);
  res = EjecutarSQL (q);
  g_free (q);
  
  if (res == NULL)
    return FALSE;

  /*Se obtiene la cantidad de productos que tiene la factura*/
  q = g_strdup_printf ("SELECT COUNT (fcd.barcode) AS cantidad "
		       "       FROM factura_compra_detalle fcd "
		       "       INNER JOIN factura_compra fc "
		       "       ON fcd.id_factura_compra = fc.id "
		       "WHERE fc.id = %d",
		       producto->id_factura_compra);
  res = EjecutarSQL (q);
  g_free (q);

  cantidad_productos = atoi (PQvaluebycol (res, 0, "cantidad"));

  /*Si ya no quedan más productos en esa factura, ésta se elimina*/
  if (cantidad_productos == 0)
    {
      q = g_strdup_printf ("UPDATE compra SET anulada = true "
			   "WHERE id IN (SELECT id_compra FROM factura_compra WHERE id = %d)",
			   producto->id_factura_compra);
      res = EjecutarSQL (q);
      g_free (q);

      q = g_strdup_printf ("DELETE FROM factura_compra "
			   "WHERE id = %d",
			   producto->id_factura_compra);
      res = EjecutarSQL (q);
      g_free (q);
    }

  //TODO: PEEE si el producto que elimine no tiene más compras, su costo_promedio es 0
  /*Se actualizan los promedios en las tablas correspondientes*/
  q = g_strdup_printf ("SELECT * FROM update_avg_cost (%s, %d)",
		       producto->barcode,
		       producto->id_factura_compra);
  res = EjecutarSQL (q);
  g_free (q);

  if (res == NULL)
    return FALSE;

  return TRUE;
}


/**
 *
 * 
 *
 *
 */
gboolean
mod_to_mod_on_transfer (Prod *producto)
{
  gchar *q;
  PGresult *res;
  gboolean entro;
  gint id_factura_compra;
  
  if (producto->accion != MOD)
    return FALSE;

  /*Se actualiza la cantidad en la tabla factura_compra_detalle*/
  if ((producto->cantidad_original != producto->cantidad_nueva) &&
      (producto->cantidad_original > 0 && producto->cantidad_nueva > 0))
    {
      q = g_strdup_printf ("UPDATE traspaso_detalle "
			   "SET cantidad = %s "
			   "WHERE id_traspaso = %d "
			   "AND barcode = %s",
			   CUT (g_strdup_printf ("%.3f", producto->cantidad_nueva)),
			   producto->id_factura_compra,
			   producto->barcode);
      res = EjecutarSQL (q);
      g_free (q);
      
      if (res == NULL)
	return FALSE;
      entro = TRUE;
    }

  /*Se actualiza costo en la tabla factura_compra_detalle*/
  if ((producto->costo_original != producto->costo_nuevo) &&
      (producto->costo_original > 0 && producto->costo_nuevo > 0))
    {
      q = g_strdup_printf ("UPDATE traspaso_detalle "
			   "SET precio = %s "
			   "WHERE id_traspaso = %d "
			   "AND barcode = %s",
			   CUT (g_strdup_printf ("%.3f", producto->costo_nuevo)),
			   producto->id_factura_compra,
			   producto->barcode);
      res = EjecutarSQL (q);
      g_free (q);
      
      if (res == NULL)
	return FALSE;
      entro = TRUE;
    }

  /*Se actualizan los promedios en las tablas correspondientes*/
  if (entro == TRUE)
    {
      q = g_strdup_printf ("SELECT COALESCE (MAX (fc.id ), 0) AS id "
			   "FROM factura_compra fc "
			   "INNER JOIN factura_compra_detalle fcd "
			   "      ON fc.id = fcd.id_factura_compra "
			   "WHERE fecha < (SELECT fecha "
			   "	           FROM traspaso "
			   "	           WHERE id = %d) "
			   "AND fcd.barcode = %s", producto->id_traspaso, producto->barcode);
      res = EjecutarSQL (q);
      if (PQntuples (res) == 1)
	id_factura_compra = atoi (PQvaluebycol (res, 0, "id"));
      else
	id_factura_compra = 0;

      q = g_strdup_printf ("SELECT * FROM update_avg_cost (%s, %d)",
			   producto->barcode,
			   id_factura_compra);
      res = EjecutarSQL (q);
      g_free (q);

      if (res == NULL)
	return FALSE;
    }
  
  return TRUE;
}



/**
 * Devuelve el barcode del producto del codigo_corto correpondiente
 *
 * @param: codigo_corto (gchar *): Codigo corto del producto.
 *
 */
gchar *
codigo_corto_to_barcode (gchar *codigo_corto)
{
  gchar *barcode;
  gchar *q;
  PGresult *res;

  q = g_strdup_printf ("SELECT barcode "
		       "FROM codigo_corto_to_barcode ('%s')", codigo_corto);

  res = EjecutarSQL (q);
  barcode = g_strdup (PQvaluebycol (res, 0, "barcode"));
  return barcode;
}


/**
 * Obtiene todos los componentes del producto
 * compuesto con el barcode especificado
 *
 * @param: barcode producto compuesto
 * @return: PGresult resultado de la busqueda
 */
PGresult *
get_componentes_compuesto (gchar *barcode)
{
  PGresult *res;
  gchar *q;

  // barcode_madre = (Mercadería compuesta), barcode_comp_der = (Mercadería que compone a la madre)
  q = g_strdup_printf ("SELECT cmc.cant_mud AS cantidad_mud, cmc.barcode_componente AS barcode, cmc.tipo_componente AS tipo, "
		       "       prd.codigo_corto AS codigo, prd.marca, prd.descripcion, prd.precio, "
		       "       (SELECT costo FROM obtener_costo_promedio_desde_barcode (cmc.barcode_componente)) AS costo_promedio "
		       "FROM componente_mc cmc "
		       "INNER JOIN producto prd "
		       "      ON prd.barcode = cmc.barcode_componente "
		       "WHERE cmc.barcode_complejo = '%s' "
		       "      AND prd.estado = true",
		       barcode);

  res = EjecutarSQL (q);
  return res;
}


/**
 * Obtiene el stock, las ventas, y el valorizado de ambos, día a día dentro del 
 * rango de fecha especificado, tanto de un producto específico, como toda la mercadería
 *
 * @param fecha_inicio
 * @param fecha_final
 * @param barcode: Si es 0 muestra el valorizado de todas las mercaderías
 */
PGresult *
movimiento_en_rango (GDate *fecha_inicio, GDate *fecha_final, gchar *barcode)
{
  gchar *q;
  PGresult *res;

  q = g_strdup_printf ("SELECT fecha_out AS fecha, SUM (stock_fecha_out) AS stock, ROUND (SUM (valor_stock_fecha_out)) AS valor_stock, "
		       "       SUM (cantidad_vendida_fecha_out) AS unidades_vendidas, ROUND (SUM (monto_venta_fecha_out)) AS valor_vendido "
		       "FROM movimiento_en_periodo (to_timestamp('%.2d %.2d %.4d', 'DD MM YYYY')::timestamp, to_timestamp('%.2d %.2d %.4d', 'DD MM YYYY')::timestamp, %s::bigint) "
		       "GROUP BY fecha_out "
		       "ORDER BY fecha_out ASC",
		       g_date_get_day(fecha_inicio), g_date_get_month(fecha_inicio), g_date_get_year(fecha_inicio),
		       g_date_get_day(fecha_final), g_date_get_month(fecha_final), g_date_get_year(fecha_final),
		       barcode);
  
  res = EjecutarSQL (q);
  
  if (res != NULL)
    return res;
  else
    return NULL;

}


/**
 *
 */
gboolean
agregar_producto_mesa (gint num_mesa, gchar *barcode, gdouble precio, gdouble cantidad)
{
  PGresult *res;
  res = EjecutarSQL (g_strdup_printf
                     ("INSERT INTO mesa (num_mesa, barcode, precio, cantidad) "
                      "VALUES (%d, %s, %s, %s)", num_mesa, barcode, CUT (g_strdup_printf ("%.3f", precio)), CUT (g_strdup_printf ("%.3f", cantidad)) ));

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}


/**
 *
 */
gboolean
aumentar_producto_mesa (gint num_mesa, gchar *barcode, gdouble cantidad)
{
  PGresult *res;
  res = EjecutarSQL (g_strdup_printf
                     ("UPDATE mesa SET cantidad = cantidad + %s "
                      "WHERE num_mesa=%d AND barcode=%s", CUT (g_strdup_printf ("%.3f", cantidad)), num_mesa, barcode));

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}
