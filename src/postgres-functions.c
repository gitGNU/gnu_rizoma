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
InsertNewDocument (gint document_type, gint sell_type)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf
                     ("INSERT INTO documentos_emitidos (tipo_documento, forma_pago, num_documento, fecha_emision)"
                      "VALUES (%d, %d, %d, NOW())", document_type, sell_type, get_ticket_number (document_type) + 1));

  res = EjecutarSQL ("SELECT last_value FROM documentos_emitidos_id_seq");

  if (res != NULL)
    return atoi (PQgetvalue (res, 0, 0));
  else
    return -1;
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
          gint tipo_documento, gchar *cheque_date, gboolean cheques, gboolean canceled)
{
  gint venta_id, monto, id_documento;
  gint day, month, year;
  gchar *serie, *numero, *banco, *plaza;
  gchar *inst, *fecha;
  gchar *q;
  gchar *vale_dir = rizoma_get_value ("VALE_DIR");

  id_documento = InsertNewDocument (tipo_documento, tipo_venta);

  if (id_documento == -1)
    return FALSE;

  q = g_strdup_printf( "SELECT inserted_id FROM registrar_venta( %d, %d, %d, "
                       "%d::smallint, %d::smallint, %s::smallint, %d, '%d' )",
                       total, machine, seller, tipo_documento, tipo_venta,
                       CUT(discount), id_documento, canceled);
  venta_id = atoi (GetDataByOne (q));
  g_free (q);

  if (boleta != -1)
    set_ticket_number (boleta, tipo_documento);

  SaveProductsSell (venta->header, venta_id);

  if (vale_dir != NULL && !g_str_equal(vale_dir, ""))
    PrintVale (venta->header, venta_id, total);

  switch (tipo_venta)
    {
    case CHEQUE:
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
      break;

    case TARJETA:
      inst = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->tarjeta_inst)));
      numero = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->tarjeta_numero)));
      fecha = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->tarjeta_fecha)));

      SaveVentaTarjeta (venta_id, inst, numero, fecha);
      break;

    case CREDITO:
      InsertDeuda (venta_id, atoi (rut), seller);

      if (GetResto (atoi (rut)) != 0)
        CancelarDeudas (0, atoi (rut));
      break;
    case CASH:
      break;
    default:
      g_printerr("%s: Trying to sale without a proper sell type", G_STRFUNC);
      return FALSE;
    }

  switch (tipo_documento)
    {
    case FACTURA: //specific operations for invoice
      if (rizoma_get_value_boolean ("PRINT_FACTURA")) //print the invoice
        PrintDocument(tipo_documento, rut, total, id_documento,venta->header);
      break;

    case SIMPLE: //specific operations for cash
      break;

    case GUIA: //specific operations for guide
      break;

    case VENTA:
      break;
    default:
      g_printerr("%s: Trying to sale without the proper document type", G_STRFUNC);
      return FALSE;
    }
  return TRUE;
}

PGresult *
SearchTuplesByDate (gint from_year, gint from_month, gint from_day,
                    gint to_year, gint to_month, gint to_day,
                    gchar *date_column, gchar *fields)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf
                       ("SELECT %s FROM venta WHERE "
                        "%s>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND "
                        "%s<=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') and id not in  (select id_sale from venta_anulada) ORDER BY fecha DESC",
                        fields, date_column, from_day, from_month, from_year,
                        date_column, to_day+1, to_month, to_year));

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
     ("SELECT trunc(sum(vd.precio * vd.cantidad -descuento)),count(Distinct(v.id)) "
      "FROM venta v,venta_detalle vd "
      "WHERE fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') "
      "AND fecha<to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') and v.id = id_venta "
      "AND (SELECT forma_pago FROM documentos_emitidos WHERE id=id_documento)=%d "
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
     ("SELECT SUM((SELECT SUM(cantidad * precio) FROM venta_detalle WHERE id_venta=venta.id)), "
      "count (*) FROM venta WHERE fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND "
      "fecha<to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND ((SELECT forma_pago FROM documentos_emitidos "
      "WHERE id=id_documento)=%d OR (SELECT forma_pago FROM documentos_emitidos WHERE id=id_documento)=%d) and venta.id not in ( select id_sale from venta_anulada)",
      from_day, from_month, from_year, to_day+1, to_month, to_year, CREDITO, TARJETA));

  if (res == NULL)
    return 0;

  *total = atoi (PQgetvalue (res, 0, 1));

  return atoi (PQgetvalue (res, 0, 0));
}

gint
GetTotalSell (guint from_year, guint from_month, guint from_day,
              guint to_year, guint to_month, guint to_day, gint *total)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf
                     ("SELECT trunc(sum(vd.precio * vd.cantidad -descuento)),count(Distinct(v.id)) "
                      "FROM venta v,venta_detalle vd "
                      "WHERE fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') "
                      "AND fecha<to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') and v.id = id_venta "
                      "AND  v.id NOT IN (select id_sale from venta_anulada)",
                      from_day, from_month, from_year, to_day+1, to_month, to_year));

  if (res == NULL)
    return 0;

  *total = atoi (PQgetvalue (res, 0, 1));

  return atoi (PQgetvalue (res, 0, 0));
}

gboolean
InsertClient (gchar *nombres, gchar *paterno, gchar *materno, gchar *rut, gchar *ver,
              gchar *direccion, gchar *fono, gint credito, gchar *giro)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("INSERT INTO cliente (rut, dv, nombre, apell_p, apell_m, giro, abonado, direccion, telefono, credito) "
                       "VALUES (%s, '%s', '%s', '%s', '%s', '%s', 0, '%s', '%s', %d)",
                       rut, ver, nombres, paterno, materno, giro, direccion, fono, credito);
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
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("INSERT INTO deuda (id_venta, rut_cliente, vendedor) "
                                      "VALUES (%d, %d, %d)",
                                      id_venta, rut, vendedor));

  return 0;
}

gint
DeudaTotalCliente (gint rut)
{
  PGresult *res;
  gint deuda;

  res = EjecutarSQL (g_strdup_printf ("SELECT SUM (monto) as monto FROM venta WHERE id IN "
                                      "(SELECT id_venta FROM deuda WHERE rut_cliente=%d AND pagada='f')",
                                      rut));

  deuda = atoi (PQgetvalue (res, 0, 0));
  deuda -= GetResto(rut);

  return deuda;
}

PGresult *
SearchDeudasCliente (gint rut)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT id, monto, maquina, vendedor, date_part('day', fecha), date_part('month', fecha), "
                       "date_part('year', fecha), date_part('hour', fecha), date_part('minute', fecha), "
                       "date_part ('second', fecha) FROM venta WHERE id IN (SELECT id_venta FROM deuda WHERE "
                       "rut_cliente=%d AND pagada='f')", rut);
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

  q = g_strdup_printf ("SELECT * FROM venta WHERE id IN "
                       "(SELECT id_venta FROM deuda WHERE rut_cliente=%d AND pagada='f') ORDER BY fecha asc", rut);
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
      monto_venta = atoi (PQgetvalue (res, i, 1));

      if (monto_venta <= abonar)
        {
          abonar -= monto_venta;

          PagarDeuda (PQgetvalue (res, i, 0));
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
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("UPDATE cliente SET credito_enable='%d' WHERE rut=%d",
                       (gint) status, rut);
  res = EjecutarSQL (q);
  g_free (q);
}

gboolean
ClientDelete (gint rut)
{
  PGresult *res = NULL;

  if (DeudaTotalCliente (rut) == 0)
    res = EjecutarSQL (g_strdup_printf ("DELETE FROM cliente WHERE rut=%d", rut));

  if (res != NULL)
    return TRUE;
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
                   gchar *familia, gboolean perecible, gboolean fraccion)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT costo_promedio FROM producto WHERE barcode ='%s'", barcode);
  res = EjecutarSQL(q);
  g_free(q);

  // Recalcular el porcentaje de ganancia a partir del precio final del producto
  // TODO: Unificar este cálculo, pues se realiza una y otra vez en distintas partes
  gdouble porcentaje;
  gint costo_promedio = atoi (PQvaluebycol (res, 0, "costo_promedio"));
  gint precio_local = atoi (g_strdup (precio));

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

      // Calculo
      iva_local = (iva_local / 100) + 1;
      porcentaje = (gdouble) ((precio_local / (gdouble)(iva_local * costo_promedio)) -1) * 100;
      g_free(q);
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
      iva_local = (gdouble) (iva_local / 100);
      otros_local = (gdouble) (otros_local / 100);

      porcentaje = (gdouble) precio_local / (gdouble)(iva_local + otros_local + 1);
      porcentaje = (gdouble) porcentaje - costo_promedio;
      porcentaje = lround ((gdouble)(porcentaje / costo_promedio) * 100);
    }

  /*No tiene IVA y Otros es la opcion "Sin Impuestos"*/
  else if (iva == FALSE && (otros == 0 || otros == -1))
    {
      porcentaje = (gdouble) ((precio_local / costo_promedio) - 1) * 100;
    }

  q = g_strdup_printf ("UPDATE producto SET codigo_corto='%s', descripcion='%s',"
                       "marca='%s', unidad='%s', contenido='%s', precio=%d, "
                       "impuestos='%d', otros=%d, "
                       "perecibles='%d', fraccion='%d', margen_promedio=%ld WHERE barcode='%s'",
                       codigo, SPE(description), SPE(marca), unidad, contenido, atoi (precio),
                       iva, otros, (gint)perecible, (gint)fraccion, lround(porcentaje), barcode);
  res = EjecutarSQL(q);
  g_free(q);
}

gboolean
AddNewProductToDB (gchar *codigo, gchar *barcode, gchar *description, gchar *marca,
                   gchar *contenido, gchar *unidad, gboolean iva, gint otros, gchar *familia,
                   gboolean perecible, gboolean fraccion)
{
  gint insertado;
  gchar *q;

  q = g_strdup_printf ("SELECT insertar_producto::integer FROM insertar_producto(%s::bigint, '%s'::varchar,"
                       "upper('%s')::varchar, upper('%s')::varchar,'%s'::varchar, upper('%s')::varchar, "
                       "%d::boolean, %d,0::smallint, %d::boolean,"
                       "%d::boolean)",barcode, codigo, SPE(marca), SPE(description), contenido, unidad, iva,
                       otros, perecible, fraccion);
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
                      rut, nota, dias_pago == 0 ? dias_pago - 1 : dias_pago);
  id_compra = atoi(GetDataByOne(q));

  g_free(q);

  SaveBuyProducts (compra->header_compra, id_compra);
}

void
SaveBuyProducts (Productos *header, gint id_compra)
{
  Productos *products = header;
  PGresult *res;
  gdouble iva, otros = 0;
  gchar *cantidad;
  gchar *precio_compra;
  gchar *q;

  do
    {

      if (products->product->iva != -1)
        iva = (gdouble) (products->product->precio_compra *
                         products->product->cantidad) *
          (gdouble)products->product->iva / 100;

      if (products->product->otros != -1)
        otros = (gdouble) (products->product->precio_compra *
                           products->product->cantidad) *
          (gdouble)products->product->otros / 100;

      cantidad = g_strdup_printf ("%.2f", products->product->cantidad);
      precio_compra = g_strdup_printf ("%.2f", products->product->precio_compra);

      q = g_strdup_printf("SELECT * FROM insertar_detalle_compra(%d, "
                          "%s::double precision, %s::double precision, %d, "
                          "0::double precision, 0::smallint, %s, %d, %ld, %ld)",
                          id_compra, CUT(cantidad), CUT (precio_compra),
                          products->product->precio, products->product->barcode,
                          products->product->margen, lround (iva),
                          lround (otros));
      res = EjecutarSQL(q);

      g_free(precio_compra);
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

  iva = (gdouble) (product->precio_compra * product->cantidad) *
    (gdouble)product->iva / 100;

  if (product->otros != -1)
    otros = (gdouble) (product->precio_compra * product->cantidad) *
      (gdouble)product->otros / 100;


  cantidad = CUT (g_strdup_printf ("%.2f", product->cantidad));

  if (factura)
    {
      q = g_strdup_printf
        ("INSERT INTO factura_compra_detalle (id, id_factura_compra, barcode, cantidad, precio, iva, otros) "
         "VALUES (DEFAULT, %d, %s, %s, '%s', %ld, %ld)",
         doc, product->barcode, cantidad, CUT (g_strdup_printf ("%.2f", product->precio_neto)), lround (iva), lround (otros));
    }
  else
    {
      q = g_strdup_printf
        ("INSERT INTO guias_compra_detalle (id, id_guias_compra, barcode, cantidad, precio, iva, otros) "
         "VALUES (DEFAULT, %d, %s, %s, '%s', %ld, %ld)",
         doc, product->barcode, cantidad, CUT (g_strdup_printf ("%.2f", product->precio_neto)),lround (iva), lround (otros));
    }

  res = EjecutarSQL (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

gboolean
IngresarProducto (Producto *product, gint compra)
{
  PGresult *res;
  //  gint old_stock, stock_final;
  gint fifo;
  gdouble stock_pro, canjeado;
  gdouble imps, ganancia, margen_promedio;
  gchar *q;
  gchar *cantidad;

  cantidad = CUT (g_strdup_printf ("%.2f", product->cantidad));

  if (product->stock_pro != 0 && product->tasa_canje != 0)
    {
      canjeado = product->stock_pro * ((double)1 / product->tasa_canje);
    }
  else
    {
      canjeado = 0;
    }

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

  if (product->otros != -1)
    imps = (gdouble) product->iva / 100 + (gdouble)product->otros / 100;
  else
    imps = (gdouble) product->iva / 100;

  ganancia = lround ((gdouble)(product->precio / (imps + 1)));

  margen_promedio = (gdouble)((ganancia - fifo) / fifo) * 100;

  if (product->canjear == TRUE)
    {
      q = g_strdup_printf ("UPDATE producto SET margen_promedio=%ld, costo_promedio=%d, stock=stock+%s, stock_pro=%s WHERE barcode=%s",
                           lround (margen_promedio), fifo, cantidad, CUT (g_strdup_printf ("%.2f", stock_pro)), product->barcode);
      res = EjecutarSQL (q);
      g_free (q);
    }
  else
    {
      q = g_strdup_printf ("UPDATE producto SET margen_promedio=%ld, costo_promedio=%d, stock=stock+%s WHERE barcode=%s",
                           lround (margen_promedio), fifo, cantidad, product->barcode);
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
  gint stock;
  PGresult *res;
  gchar *cantidad;

  do
    {
      cantidad = CUT (g_strdup_printf ("%.2f", products->product->cantidad));

      stock = atoi (GetDataByOne (g_strdup_printf ("SELECT stock FROM select_producto(%s)",
                                                   products->product->barcode)));

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

  res = EjecutarSQL (g_strdup_printf ("SELECT stock FROM select_producto(%s)", barcode));

  stock = g_ascii_strtod(PQgetvalue (res, 0, 0), NULL);

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

gint
FiFo (gchar *barcode, gint compra)
{
  PGresult *res;
  gchar *q;
  gint fifo;

  q = g_strdup_printf ("select calculate_fifo (%s,%d)", barcode, compra);
  res = EjecutarSQL (q);
  g_free (q);

  fifo = atoi (PQgetvalue (res, 0, 0));

  return fifo;
}

/**
 * Es llamada por la funcion SaveSell
 * Esta funciom calcula el iva, otros, ademas de descontar los productos
 * vendidos del stock y luego registra la venta en la tabla venta_detalle
 *
 * @param products puntero que apunta a los valores del producto que se va vender
 * @param id_venta es el id de la venta que se esta realizando
 * @return 1 si se realizo correctamente la operacion 0 si hay error
 */

gboolean
SaveProductsSell (Productos *products, gint id_venta)
{
  PGresult *res;
  Productos *header = products;
  gdouble iva,precioPro, otros = 0;
  gint margen;
  gchar *cantidad;
  gint precio;
  gchar *q;
  gint pre;
  do
    {
      cantidad = CUT (g_strdup_printf ("%.3f", products->product->cantidad));

      iva = GetIVA (products->product->barcode);
      otros = GetOtros (products->product->barcode);
      iva = (gdouble) iva / 100;

      if (otros != -1)
        otros = (gdouble) otros / 100;

      if (products->product->cantidad_mayorista > 0 && products->product->precio_mayor > 0 &&
          products->product->cantidad >= products->product->cantidad_mayorista &&
          products->product->mayorista == TRUE)
        {
          if (otros == -1)
            margen = (gdouble) ((products->product->precio_mayor / (gdouble)((iva + 1) * products->product->fifo)) - 1) * 100;
          else
            {
              margen = (gdouble) products->product->precio_mayor / (gdouble)(iva + otros + 1);
              margen = (gdouble) margen - products->product->fifo;
              margen = (gdouble) (margen / products->product->fifo) * 100;
            }
        }
      else
        margen = products->product->margen;

      precioPro = products->product->precio_compra;
      /*
        Si el costo promedio es -1, vuelve a preguntar el valor de costo
        promedio, a traves de la funcion de postgres "informacion_producto"

       */
      if (lround(precioPro) == -1)
	    {
	      q = g_strdup_printf ("select * from informacion_producto (%s, '')", products->product->barcode);
	      res = EjecutarSQL (q);
          pre=atoi(PQvaluebycol(res, 0, "costo_promedio"));
	      iva = (gdouble) ((pre *((gdouble)margen /100 + 1))*
		  products->product->cantidad) * (gdouble)products->product->iva / 100;
	    }
      /*

       */
      else
	    {
	      iva = (gdouble) ((products->product->precio_compra * ((gdouble)margen /100 + 1))*
          products->product->cantidad) * (gdouble)products->product->iva / 100;
	    }

      if (products->product->otros != -1)
        otros = (gdouble)((products->product->precio_compra * ((gdouble)margen /
                                                               100 + 1)) *
                          products->product->cantidad) * (gdouble)products->product->otros / 100;

      res = EjecutarSQL
        (g_strdup_printf
         ("UPDATE producto SET vendidos = vendidos+%s, stock=%s WHERE barcode='%s'",
          cantidad, CUT (g_strdup_printf ("%.3f", (gdouble)GetCurrentStock (products->product->barcode) - products->product->cantidad)), products->product->barcode));

      /*      res = EjecutarSQL (g_strdup_printf
              ("UPDATE productos SET stock=stock-%s WHERE barcode='%s'",
              cantidad, products->product->barcode));
      */

      if (products->product->cantidad_mayorista > 0 && products->product->precio_mayor > 0 &&
          products->product->cantidad >= products->product->cantidad_mayorista &&
          products->product->mayorista == TRUE)
        precio = products->product->precio_mayor;
      else
        precio = products->product->precio;

      /*
        Registra los productos con sus respectivos datos(barcode,cantidad,
        precio,fifo,iva,otros) en la tabla venta_detalle
       */
      q = g_strdup_printf ("select registrar_venta_detalle(%d, %s, %s, %d, %d, %ld, %ld)",
                           id_venta, products->product->barcode, cantidad, precio,
                           products->product->fifo, lround (iva), lround (otros));
      res = EjecutarSQL (q);
      g_free (q);

      products = products->next;
    }
  while (products != header);

  return TRUE;
}

PGresult *
ReturnProductsRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf
    ("SELECT * FROM ranking_ventas (to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date, to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')::date)",
     from_day, from_month, from_year, to_day+1, to_month, to_year);

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
  PGresult *res;

  res = EjecutarSQL ("UPDATE compra_detalle SET ingresado='t' WHERE cantidad_ingresada=cantidad");

  return TRUE;
}

gint
GetMinStock (gchar *barcode)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("SELECT stock_min FROM producto WHERE barcode='%s'",
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
IngresarFactura (gint n_doc, gint id_compra, gchar *rut_proveedor, gint total, gint d_emision, gint m_emision, gint y_emision, gint guia)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("INSERT INTO factura_compra (id, id_compra, rut_proveedor, num_factura, fecha, valor_neto,"
                       " valor_iva, descuento, pagada, monto) VALUES (DEFAULT, %d, '%s', %d, "
                       "to_timestamp('%.2d %.2d %.4d', 'DD MM YYYY'), 0, 0, 0,'f', %d)",
                       id_compra, rut_proveedor, n_doc, d_emision, m_emision, y_emision,
                       total);
  res = EjecutarSQL (q);
  g_free (q);

  if (id_compra != 0)
    {
      q = g_strdup_printf ("UPDATE factura_compra SET forma_pago=(SELECT compra.forma_pago FROM compra WHERE id=%d)"
                           " WHERE id_compra=%d", id_compra, id_compra);
      res = EjecutarSQL (q);
      g_free (q);

      q = g_strdup_printf ("UPDATE factura_compra SET fecha_pago=DATE(fecha)+(forma_pago) WHERE id_compra=%d", id_compra);
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

      q = g_strdup_printf ("UPDATE factura_compra SET fecha_pago=DATE(fecha)+(forma_pago) WHERE num_factura=%d", n_doc);
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
IngresarGuia (gint n_doc, gint id_compra, gint total, gint d_emision, gint m_emision, gint y_emision)
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
    return -1;
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
    return -1;
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
    return -1;
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

gint
GetNeto (gchar *barcode)
{
  PGresult *res;
  gint tuples;

  res = EjecutarSQL (g_strdup_printf ("SELECT precio FROM compra_detalle WHERE "
                                      "barcode_product='%s' AND id_compra IN (SELECT id FROM "
                                      "compra ORDER BY fecha DESC)", barcode));

  tuples = PQntuples (res);

  if (tuples != 0)
    return atoi (PQgetvalue (res, 0, 0));
  else
    return -1;
}

gint
GetFiFo (gchar *barcode)
{
  PGresult *res;
  gint tuples;

  res = EjecutarSQL (g_strdup_printf ("SELECT get_fifo(%s)", barcode));

  tuples = PQntuples (res);

  if (tuples != 0)
    return atoi (PQgetvalue (res, 0, 0));
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
  gint i, tuples;
  gdouble vendidos = strtod ((GetDataByOne (g_strdup_printf ("SELECT vendidos FROM producto WHERE barcode='%s'",
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

  tuples = PQntuples (res);

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

  res = EjecutarSQL ("SELECT SUM (costo_promedio * stock)::integer FROM producto");

  if (res != NULL)
    return PQgetvalue (res, 0, 0);
  else
    return 0;
}

gchar *
ValorTotalStock (void)
{
  PGresult *res;

  res = EjecutarSQL ("SELECT SUM (precio * stock)::integer FROM producto");

  if (res != NULL)
    return PQgetvalue (res, 0, 0);
  else
    return 0;
}

gchar *
ContriTotalStock (void)
{
  PGresult *res;

  res = EjecutarSQL ("SELECT SUM (round (costo_promedio * (margen_promedio / 100))  * stock)::integer FROM producto");

  if (res != NULL)
    return PQgetvalue (res, 0, 0);
  else
    return 0;
}

void
SetModificacionesProducto (gchar *barcode, gchar *stock_minimo, gchar *margen, gchar *new_venta,
                           gboolean canjeable, gint tasa, gboolean mayorista, gint precio_mayorista,
                           gint cantidad_mayorista)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("UPDATE producto SET stock_min=%s, margen_promedio=%s, precio=%s, canje='%d', tasa_canje=%d, "
                       "precio_mayor=%d, cantidad_mayor=%d, mayorista='%d' WHERE barcode='%s'",
                       stock_minimo, margen, new_venta, (gint)canjeable, tasa, precio_mayorista, cantidad_mayorista,
                       (gint)mayorista, barcode);
  res = EjecutarSQL (q);
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

void
AjusteStock (gdouble cantidad, gint motivo, gchar *barcode)
{
  PGresult *res;
  gdouble stock = GetCurrentStock (barcode);
  gchar *q;

  q = g_strdup_printf ("INSERT INTO merma VALUES (DEFAULT, '%s', %s, %d)",
                       barcode, CUT (g_strdup_printf ("%f", stock - cantidad)), motivo);
  res = EjecutarSQL (q);
  g_free (q);

  if (cantidad == 0)
    {
      q = g_strdup_printf ("UPDATE producto SET stock=0 WHERE barcode='%s'", barcode);
      res = EjecutarSQL (q);
      g_free (q);
    }
  else
    {
      gchar *new = CUT (g_strdup_printf ("%f", stock - cantidad));
      q = g_strdup_printf ("UPDATE producto SET stock=stock-%s WHERE barcode='%s'", new, barcode);
      res = EjecutarSQL (q);
      g_free (q);
    }
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
Recivir (gchar *barcode, gchar *cantidad)
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
SetModificacionesProveedor (gchar *rut, gchar *razon, gchar *direccion, gchar *comuna,
                            gchar *ciudad, gchar *fono, gchar *web, gchar *contacto,
                            gchar *email, gchar *giro)
{
  PGresult *res;
  gchar *q;
  gchar **aux;

  aux = g_strsplit(rut, "-", 0);

  q = g_strdup_printf ("UPDATE proveedor SET nombre='%s', direccion='%s', ciudad='%s', "
                       "comuna='%s', telefono='%s', email='%s', web='%s', contacto='%s', giro='%s'"
                       " WHERE rut=%s", razon, direccion, ciudad,
                       comuna, fono, email, web, contacto, giro, aux[0]);
  res = EjecutarSQL (q);
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

  q = g_strdup_printf("select * from nullify_sale(%d, %d)", user_data->user_id, sale_id);

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
  gint precio,precioCompra;
  gchar *q;
  gint pre;
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
          pre=atoi(PQvaluebycol(res, 0, "costo_promedio"));

          q = g_strdup_printf ("select registrar_devolucion_detalle(%d, %s, %s, %d, %d)",
                               id_devolucion, products->product->barcode, cantidad,precio, pre);
        }
      else
         q = g_strdup_printf ("select registrar_devolucion_detalle(%d, %s, %s, %d, %d)",
                              id_devolucion, products->product->barcode, cantidad, precio,precioCompra);

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
 * @return 1 si se realizo correctamente la operacion 0 si hay error
 */
gboolean
SaveTraspaso (gint total, gint origen, gint vendedor, gint destino, gboolean tipo_traspaso)
{
  gint traspaso_id;
  gchar *q;

  q = g_strdup_printf( "SELECT inserted_id FROM registrar_traspaso( %d, %d, %d, %d) ",
                       total, origen, destino,vendedor);
  traspaso_id = atoi (GetDataByOne (q));
  g_free (q);

  SaveProductsTraspaso (venta->header, traspaso_id, tipo_traspaso);

  return TRUE;
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
 * @return 1 si se realizo correctamente la operacion 0 si hay error
 */

gboolean
SaveTraspasoCompras (gint total, gint origen, gint vendedor, gint destino, gboolean tipo_traspaso)
{
  gint traspaso_id;
  gchar *q;

  q = g_strdup_printf( "SELECT inserted_id FROM registrar_traspaso( %d, %d, %d, %d) ",
                       total, origen, destino,vendedor);
  traspaso_id = atoi (GetDataByOne (q));
  g_free (q);

  SaveProductsTraspaso (compra->header_compra, traspaso_id, tipo_traspaso);
  return TRUE;
}

/**
 * Es llamada por las funciones SaveTraspaso() y SaveTraspasoCompras()
 *
 * Esta funcion descuenta  o agrega del stock los productos traspasados y
 * luego registra los productos traspasados, en la tabla devolucion_detalle
 *
 * @param products puntero que apunta a los valores del producto que se va vender
 * @param id_traspaso es el id del traspaso  que se esta realizando
 * @param tipo_traspaso: 1 si es un traspaso de enviar 0  si es un trapaso de recibir
 * @return 1 si se realizo correctamente la operacion 0 si hay error
 */

gboolean
SaveProductsTraspaso (Productos *products, gint id_traspaso, gboolean tipo_traspaso)
{
  PGresult *res;
  Productos *header = products;
  gdouble iva, otros = 0;
  gint margen;
  gchar *cantidad;
  gint precio;
  gchar *q;
  gint pre;

  do
    {
      cantidad = CUT (g_strdup_printf ("%.3f", products->product->cantidad));
      if (tipo_traspaso == TRUE)
        {
          res = EjecutarSQL
            (g_strdup_printf
             ("UPDATE producto SET stock=%s WHERE barcode='%s'",
              CUT (g_strdup_printf ("%.3f", (gdouble)GetCurrentStock (products->product->barcode) - products->product->cantidad)), products->product->barcode));

          /*      res = EjecutarSQL (g_strdup_printf
                  ("UPDATE productos SET stock=stock-%s WHERE barcode='%s'",
                  cantidad, products->product->barcode));
          */
        }
      else
        {
          res = EjecutarSQL
            (g_strdup_printf
             ("UPDATE producto SET stock=%s WHERE barcode='%s'",
              CUT (g_strdup_printf ("%.3f", (gdouble)GetCurrentStock (products->product->barcode) + products->product->cantidad)), products->product->barcode));
        }

      precio = products->product->precio_compra;

      if (lround(precio) == -1)
        {
          q = g_strdup_printf ("select * from informacion_producto (%s, '')", products->product->barcode);
          res = EjecutarSQL (q);
          pre=atoi(PQvaluebycol(res, 0, "costo_promedio"));

          q = g_strdup_printf ("select registrar_traspaso_detalle(%d, %s, %s, %d)",
                           id_traspaso, products->product->barcode, cantidad, pre);
        }
      else
         q = g_strdup_printf ("select registrar_traspaso_detalle(%d, %s, %s, %d)",
                           id_traspaso, products->product->barcode, cantidad, precio);

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

gint
TotalPrecioCompra (Productos *products)
{
  Productos *header = products;
  gint total=0;
  gint pre;
  PGresult *res;
  gchar *q;
  do
    {
      q = g_strdup_printf ("select * from informacion_producto (%s, '')", products->product->barcode);
      res = EjecutarSQL (q);
      pre = atoi(PQvaluebycol(res, 0, "costo_promedio"));
      total = total + pre;
      products = products->next;
    }
  while (products != header);

  return total;
}


