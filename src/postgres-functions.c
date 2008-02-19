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

#include<glib.h>

#include<stdlib.h>
#include<string.h>

#include"tipos.h"
#include"config_file.h"
#include"postgres-functions.h"
#include"boleta.h"
#include"vale.h"
#include"rizoma_errors.h"

PGconn *connection;

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
  gchar *num = g_strdup (number);
  gint len = (int)strlen (num);

  while (len != -1)
    {
	  if (num[len] == '.')
		{
		  num[len]=',';
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

  if( status == CONNECTION_OK ) {
	  res = PQexec (connection, sentencia);
	  if( res == NULL ) {
		  rizoma_errors_set (PQerrorMessage (connection), "EjcutarSQL ()", ERROR);
	  } else {
		  return res;
	  }
  } else {
	  gchar *strconn = g_strdup_printf ("host=%s port=%s dbname=%s user=%s password=%s sslmode=%s",
										host, port, name, user, pass,sslmode);

	  connection = PQconnectdb (strconn);
	  g_free( strconn );

	  status = PQstatus(connection);

	  switch (status)
	  {
		  case CONNECTION_OK:
			  res = PQexec (connection, sentencia);
			  if( res == NULL ) {
				  rizoma_errors_set (PQerrorMessage (connection), "EjcutarSQL ()", ERROR);
			  } else {
				  return res;
			  }
			  break;
		  case CONNECTION_BAD:
 			  rizoma_errors_set (PQerrorMessage (connection), "EjcutarSQL ()", ERROR);
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

gboolean
DeleteProduct (gchar *codigo)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("DELETE FROM producto WHERE codigo='%s'", codigo));

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

  if (vale_dir != "" || vale_dir != NULL)
    {
	  PrintVale (venta->header, venta_id, total);
    }

  if (tipo_venta == CHEQUE)
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
  if (tipo_venta == TARJETA)
    {
	  inst = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->tarjeta_inst)));
	  numero = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->tarjeta_numero)));
	  fecha = g_strdup (gtk_entry_get_text (GTK_ENTRY (venta->tarjeta_fecha)));

	  SaveVentaTarjeta (venta_id, inst, numero, fecha);
    }
  if (tipo_venta == CREDITO)
    {
	  InsertDeuda (venta_id, atoi (rut), seller);

	  if (GetResto (atoi (rut)) != 0)
		CancelarDeudas (0, atoi (rut));
    }

  return TRUE;
}

PGresult *
SearchTuplesByDate (gint from_year, gint from_month, gint from_day,
					gint to_year, gint to_month, gint to_day,
					gchar *date_column, gchar *fields)
{
  PGresult *res;

  if (from_year == to_year && from_month == to_month && from_day == to_day)
	res = EjecutarSQL (g_strdup_printf
					   ("SELECT %s FROM venta WHERE "
						"date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND "
						"date_part('day', fecha)=%d ORDER BY fecha DESC",
						fields, from_year, from_month, from_day));
  else
	res = EjecutarSQL (g_strdup_printf
					   ("SELECT %s FROM venta WHERE "
						"%s>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND "
						"%s<=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') ORDER BY fecha DESC",
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
  //TODO: arreglar este select para ajustarse al nuevo esquema, hace
  //que se vaya de segfault
  res = EjecutarSQL
	(g_strdup_printf
	 ("SELECT SUM ((SELECT SUM (cantidad * precio) FROM venta_detalle WHERE id_venta=ventas.id)), "
	  "count (*) FROM venta WHERE fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND "
	  "fecha<to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND (SELECT forma_pago FROM documentos_emitidos "
	  "WHERE id=id_documento)=%d", from_day, from_month, from_year, to_day+1, to_month, to_year, CASH));

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
	 ("SELECT SUM((SELECT SUM(cantidad * precio) FROM venta_detalle WHERE id_venta=ventas.id)), "
	  "count (*) FROM venta WHERE fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND "
	  "fecha<to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND ((SELECT forma_pago FROM documentos_emitidos "
	  "WHERE id=id_documento)=%d OR (SELECT forma_pago FROM documentos_emitidos WHERE id=id_documento)=%d)",
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
					 ("SELECT SUM((SELECT SUM(cantidad * precio) FROM venta_detalle WHERE "
					  "id_venta=ventas.id)), count (*) FROM venta WHERE "
					  "fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND "
					  "fecha<to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')",
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

  res = EjecutarSQL
	(g_strdup_printf ("INSERT INTO clientes VALUES(DEFAULT, '%s', '%s', '%s', '%s', '%s', '%s', "
					  "'%s', 0, %d, DEFAULT, '%s')", nombres, paterno, materno, rut, ver,
					  direccion, fono, credito, giro));

  if (res != NULL)
	return TRUE;
  else
	return FALSE;
}

gboolean
RutExist (gchar *rut)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT * FROM cliente WHERE rut=%s", strtok (rut, "-")));

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

  res = EjecutarSQL (g_strdup_printf ("INSERT INTO deudas VALUES (DEFAULT, %d, %d, %d, DEFAULT, DEFAULT)",
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

  return deuda;
}

PGresult *
SearchDeudasCliente (gint rut)
{
  PGresult *res;

  res = EjecutarSQL
	(g_strdup_printf
	 ("SELECT id, monto, maquina, vendedor, date_part('day', fecha), date_part('month', fecha), "
	  "date_part('year', fecha), date_part('hour', fecha), date_part('minute', fecha), "
	  "date_part ('second', fecha) FROM venta WHERE id IN (SELECT id_venta FROM deuda WHERE "
	  "rut_cliente=%d AND pagada='f')", rut));

  return res;
}

gint
PagarDeuda (gchar *id_venta)
{
  EjecutarSQL (g_strdup_printf ("UPDATE deudas SET pagada='t' WHERE id_venta=%s", id_venta));

  return 0;
}

gint
CancelarDeudas (gint abonar, gint rut)
{
  PGresult *res;
  //  gint deuda = DeudaTotalCliente (rut);
  gint monto_venta;
  gint i, tuples;

  res = EjecutarSQL (g_strdup_printf ("INSERT INTO abonos VALUES (DEFAULT, %d, %d, NOW())", rut, abonar));

  res = EjecutarSQL (g_strdup_printf ("SELECT * FROM venta WHERE id IN "
									  "(SELECT id_venta FROM deuda WHERE rut_cliente=%d AND pagada='f') ORDER BY fecha asc", rut));

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

  res = EjecutarSQL (g_strdup_printf ("UPDATE clientes SET abonado=%d WHERE rut=%d", resto, rut));

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

gchar *
ReturnClientCredit (gint rut)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT credito FROM cliente WHERE rut=%d", rut));

  return PQgetvalue (res, 0, 0);
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
gboolean
SaveNewPassword (gchar *passwd, gchar *user)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("UPDATE users SET passwd=md5('%s')WHERE usuario='%s'",
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

  if (strcmp (id, "") != 0)
    {
	  res = EjecutarSQL (g_strdup_printf
						 ("SELECT * FROM users WHERE id='%s'", id));

	  if (PQntuples (res) != 0)
		{
		  rizoma_errors_set (g_strdup_printf ("Ya existe un vendedor con el id: %s", id), "AddNewSeller()", ERROR);

		  return FALSE;
		}
    }

  res = EjecutarSQL
	(g_strdup_printf
	 ("INSERT INTO users VALUES (%s, '%s', md5('%s'), %s, '%s', '%s', '%s', NOW(), 1)",
	  strcmp (id, "") != 0 ? id : "DEFAULT", username, passwd, rut, nombre, apell_p, apell_m));


  if (res != NULL)
    {
	  rizoma_errors_set ("El vendedor se ha agregado con exito", "AddNewSeller()", APPLY);
	  return TRUE;
    }
  else
    {
	  rizoma_errors_set ("Error al intentar agregar un nuevo vendedor", "AddNewSeller()", ERROR);
	  return FALSE;
    }
}

gboolean
ReturnUserExist (gchar *user)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT usuario FROM users WHERE usuario='%s'", user));

  if (PQntuples (res) == 0)
	return FALSE;
  else
	return TRUE;
}

void
ChangeEnableCredit (gboolean status, gint rut)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("UPDATE clientes SET credito_enable='%d' WHERE rut=%d",
									  (gint) status, rut));
}

gboolean
ClientDelete (gint rut)
{
  PGresult *res;

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

  res = EjecutarSQL (g_strdup_printf ("UPDATE productos SET codigo='%s', descripcion='%s', precio=%d WHERE barcode='%s'",
									  codigo, SPE(description), precio, barcode));

  if (res != NULL)
	return TRUE;
  else
	return FALSE;
}

void
SaveModifications (gchar *codigo, gchar *description, gchar *marca, gchar *unidad,
				   gchar *contenido, gchar *precio, gboolean iva, gchar *otros, gchar *barcode,
				   gchar *familia, gboolean perecible, gboolean fraccion)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("UPDATE producto SET codigo_corto='%s', descripcion='%s',"
		       "marca='%s', unidad='%s', contenido='%s', precio=%d, "
		       "impuestos='%d', otros=(SELECT id FROM impuesto WHERE descripcion='%s'), "
		       "perecibles='%d', fraccion='%d' WHERE barcode='%s'",
		       codigo, SPE(description), SPE(marca), unidad, contenido, atoi (precio),
		       iva, otros, (gint)perecible, (gint)fraccion, barcode);
  printf("%s\n", q);
  res = EjecutarSQL(q);
  g_free(q);
}

gboolean
AddNewProductToDB (gchar *codigo, gchar *barcode, gchar *description, gchar *marca,
				   gchar *contenido, gchar *unidad, gboolean iva, gchar *otros, gchar *familia,
				   gboolean perecible, gboolean fraccion)
{
  gint *insertado;

  insertado = atoi( GetDataByOne (
								  g_strdup_printf
								  ("SELECT insertar_producto::integer FROM insertar_producto(%s::bigint, %s::varchar,"
								   "upper('%s')::varchar, upper('%s')::varchar,%s::varchar, upper('%s')::varchar, "
								   "%d::boolean, (SELECT id FROM impuesto WHERE descripcion='%s'),0::smallint, 0::boolean,"
								   "%d::boolean)",barcode, codigo, SPE(marca), SPE(description), contenido, unidad, iva,
								   otros, perecible, fraccion)));

  if( insertado == 1 ) {
	return TRUE;
  } else {
	return FALSE;
  }
}

void
AgregarCompra (gchar *rut, gchar *nota, gint dias_pago)
{
  PGresult *res;
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
      precio_compra=g_strdup_printf ("%.2f", products->product->precio_compra);

      q = g_strdup_printf("SELECT * FROM insertar_detalle_compra(%d, "
			  "%s::double precision, %s::double precision, %d, "
			  "0::double precision, 0::smallint, %s, %d, %d, %d)",
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

  //  res = EjecutarSQL (g_strdup_printf ("UPDATE compras SET ingresada='t' WHERE id=%d", id));
  res = EjecutarSQL (g_strdup_printf ("UPDATE compras SET ingresada='t' WHERE id IN (SELECT id_compra FROM compra_detalle WHERE cantidad=cantidad_ingresada) AND ingresada='f'"));

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
  gdouble iva = 0, otros = 0;

  iva = (gdouble) (product->precio_compra * product->cantidad) *
	(gdouble)product->iva / 100;

  if (product->otros != -1)
	otros = (gdouble) (product->precio_compra * product->cantidad) *
	  (gdouble)product->otros / 100;


  cantidad = CUT (g_strdup_printf ("%.2f", product->cantidad));

  if (product->perecible == TRUE)
	res = EjecutarSQL
	  (g_strdup_printf
	   ("INSERT INTO documentos_detalle (id, numero, id_compra, barcode, cantidad, precio, fecha, factura, elaboracion, vencimiento, iva, otros) "
		"VALUES (DEFAULT, %d, %d, '%s', %s, %s, NOW(), '%d', NULL, to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY'), %d, %d)",
		doc, compra, product->barcode, cantidad, CUT (g_strdup_printf ("%.2f", product->precio_neto)),
		(gint)factura, product->venc_day, product->venc_month, product->venc_year, lround (iva), lround (otros)));
  else
	res = EjecutarSQL
	  (g_strdup_printf
	   ("INSERT INTO documentos_detalle (id, numero, id_compra, barcode, cantidad, precio, fecha, factura, elaboracion, vencimiento, iva, otros) "
		"VALUES (DEFAULT, %d, %d, '%s', %s, %s, NOW(), '%d', NULL, NULL, %d, %d)",
		doc, compra, product->barcode, cantidad, CUT (g_strdup_printf ("%.2f", product->precio_neto)),(gint)factura, lround (iva), lround (otros)));

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

  gchar *cantidad;

  cantidad = CUT (g_strdup_printf ("%.2f", product->cantidad));

  canjeado = product->stock_pro * ((double)1 / product->tasa_canje);

  stock_pro = product->stock_pro - (product->cuanto - canjeado);

  /*
	Calculamos el margen Promedio
  */
  res = EjecutarSQL
	(g_strdup_printf ("UPDATE products_buy_history SET cantidad_ingresada=cantidad_ingresada+%s, canjeado=%s "
					  "WHERE barcode_product IN (SELECT barcode  FROM producto WHERE barcode='%s'"
					  " AND id_compra=%d)", cantidad, CUT (g_strdup_printf ("%.2f", canjeado)),product->barcode, compra));

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
	res = EjecutarSQL
	  (g_strdup_printf
	   ("UPDATE productos SET margen_promedio=%d, fifo=%d, stock=stock+%s, stock_pro=%s WHERE barcode='%s'",
		lround (margen_promedio), fifo, cantidad, CUT (g_strdup_printf ("%.2f", stock_pro)), product->barcode));
  else
	res = EjecutarSQL
	  (g_strdup_printf
	   ("UPDATE productos SET margen_promedio=%d, fifo=%d, stock=stock+%s WHERE barcode='%s'",
		lround (margen_promedio), fifo, cantidad, product->barcode));

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

	  stock = atoi (GetDataByOne (g_strdup_printf ("SELECT stock FROM producto WHERE barcode='%s'",
												   products->product->barcode)));

	  res = EjecutarSQL (g_strdup_printf ("UPDATE productos SET stock=stock-%s WHERE barcode='%s'",
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

  res = EjecutarSQL (g_strdup_printf ("SELECT contenido, unidad FROM producto WHERE barcode='%s'", barcode));

  unit = g_strdup_printf ("%s %s", PQgetvalue (res, 0, 0),
						  PQgetvalue (res, 0, 1));

  return unit;
}

gdouble
GetCurrentStock (gchar *barcode)
{
  PGresult *res;
  gdouble stock;

  res = EjecutarSQL (g_strdup_printf ("SELECT stock FROM producto WHERE barcode='%s'", barcode));

  stock = strtod (PUT (PQgetvalue (res, 0, 0)), (char **)NULL);

  return stock;
}

char *
GetCurrentPrice (gchar *barcode)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT precio FROM producto WHERE barcode='%s'", barcode));

  if (res != NULL && PQntuples (res) != 0)
	return PQgetvalue (res, 0, 0);
  else
	return "0";
}

gint
FiFo (gchar *barcode, gint compra)
{

  PGresult *res;
  gint suma = 0;
  gint fifo = GetFiFo (barcode);

  gdouble current_stock = GetCurrentStock (barcode);

  res = EjecutarSQL
	(g_strdup_printf
	 ("SELECT cantidad, precio, cantidad_ingresada FROM compra_detalle, compras WHERE "
	  "barcode_product='%s' AND compras.id=%d AND products_buy_history.id_compra=%d ORDER BY compras.fecha DESC",
	  barcode, compra, compra));

  suma += current_stock * fifo;

  suma += atoi (PQgetvalue (res, 0, 1)) * atoi (PQgetvalue (res, 0, 0));

  current_stock += atoi (PQgetvalue (res, 0, 2));

  fifo = lround ((double) (suma / current_stock));

  return fifo;
}

gboolean
SaveProductsSell (Productos *products, gint id_venta)
{
  PGresult *res;
  Productos *header = products;
  gdouble iva, otros = 0;
  gint margen;
  gchar *cantidad;
  gint precio;

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


	  iva = (gdouble) ((products->product->precio_compra * ((gdouble)margen /
															100 + 1))*
					   products->product->cantidad) * (gdouble)products->product->iva / 100;

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

	  res = EjecutarSQL
		(g_strdup_printf
		 ("INSERT INTO venta_detalle VALUES(DEFAULT, %d, %s, '%s', '%s', %d, '%s',"
		  "%s, %d, %d, %d, %d)", id_venta,
		  products->product->barcode, SPE(products->product->producto), SPE(products->product->marca),
		  products->product->contenido, products->product->unidad, cantidad, precio,
		  products->product->fifo, lround (iva), lround (otros)));

	  products = products->next;
    }
  while (products != header);

  return TRUE;
}

PGresult *
ReturnProductsRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day)
{
  PGresult *res;

  res = EjecutarSQL
	(g_strdup_printf
	 ("SELECT t1.descripcion, t1.marca, (SELECT contenido FROM producto WHERE "
	  "barcode=t1.barcode),(SELECT unidad FROM producto WHERE barcode=t1.barcode), "
	  "SUM(t1.cantidad), SUM((t1.cantidad*t1.precio)::integer), SUM ((t1.cantidad*t1.fifo)::integer), "
	  "SUM(((precio*cantidad)-((iva+otros)+(fifo*cantidad)))::integer) FROM venta_detalle AS"
	  " t1 WHERE id_venta IN (SELECT id FROM venta WHERE fecha>=to_timestamp ('%.2d %.2d %.4d', "
	  "'DD MM YYYY') AND fecha<to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')) GROUP BY "
	  "t1.descripcion, t1.marca, t1.barcode",
	  from_day, from_month, from_year, to_day+1, to_month, to_year));

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

  res = EjecutarSQL
	(g_strdup_printf ("INSERT INTO proveedores VALUES (DEFAULT, '%s', '%s', '%s', "
					  "'%s', '%s', %d, '%s', '%s', '%s', '%s')", nombre, rut, direccion, ciudad,
					  comuna, atoi (telefono), email, web, contacto, giro));

  if (res != NULL)
	return TRUE;
  else
	return FALSE;
}

gboolean
SetProductosIngresados (void)
{
  PGresult *res;

  res = EjecutarSQL ("UPDATE products_buy_history SET ingresado='t' WHERE cantidad_ingresada=cantidad");

  return TRUE;
}

gdouble
GetDayToSell (gchar *barcode)
{
  PGresult *res;
  gint day;
  gchar *days;

  if (GetCurrentStock (barcode) != 0)
    {
	  days = GetDataByOne
		(g_strdup_printf
		 ("SELECT date_part ('day', (SELECT now() - fecha FROM compra WHERE id=t1.id_compra)) "
		  "FROM compra_detalle AS t1, productos AS t2, compras AS t3 WHERE t2.barcode='%s' AND "
		  "t1.barcode_product='%s' AND t3.id=t1.id_compra ORDER BY t3.fecha ASC", barcode, barcode));

	  if (days == NULL)
		return 0;
	  else
		day = atoi (days);

	  if (day == 0)
		day = 1;

	  res = EjecutarSQL (g_strdup_printf
						 ("SELECT t1.stock / (vendidos / %d) FROM producto AS t1 WHERE t1.barcode='%s'",
						  day, barcode));

    }
  else
	return 0;


  if (res != NULL && PQntuples (res) != 0)
	return strtod (PQgetvalue (res, 0, 0), (char **)NULL);
  else
	return 0;
}

gint
GetMinStock (gchar *barcode)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf
					 ("SELECT stock_min FROM producto WHERE barcode='%s'", barcode));

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

  res = EjecutarSQL (g_strdup_printf
					 ("INSERT INTO cheques VALUES (DEFAULT, %d, '%s', %d, '%s', '%s', %d, "
					  "to_timestamp('%.2d %.2d %.4d', 'DD MM YYYY'))", id_venta, serie, number,
					  banco, plaza, monto, day, month, year));

  if (res != NULL)
	return TRUE;
  else
	return FALSE;
}

gint
ReturnIncompletProducts (gint id_venta)
{
  PGresult *res;


  res = EjecutarSQL (g_strdup_printf ("SELECT count(*) FROM compra_detalle WHERE id_compra=%d AND cantidad_ingresada>0 AND cantidad_ingresada<cantidad", id_venta));

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

  res = EjecutarSQL (g_strdup_printf ("SELECT * FROM proveedor WHERE rut=(SELECT rut_proveedor FROM compra"
									  " WHERE id=%d", id_compra));

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
IngresarFactura (gchar *n_doc, gint id_compra, gchar *rut_proveedor, gint total,
				 gchar *d_emision, gchar *m_emision, gchar *y_emision, gint guia)
{
  PGresult *res;

  res = EjecutarSQL
	(g_strdup_printf
	 ("INSERT INTO facturas_compras (id, id_compra, rut_proveedor, num_factura, fecha, valor_neto,"
	  " valor_iva, descuento, pagada, monto) VALUES (DEFAULT, %d, '%s', '%s', "
	  "to_timestamp('%.2d %.2d %.2d', 'DD MM YY'), 0, 0, 0,'f', %d)",
	  id_compra, rut_proveedor, n_doc, atoi (d_emision), atoi (m_emision), atoi (y_emision),
	  total));

  if (id_compra != 0)
    {
	  res = EjecutarSQL
		(g_strdup_printf
		 ("UPDATE facturas_compras SET forma_pago=(SELECT compras.forma_pago FROM compra WHERE id=%d)"
		  " WHERE id_compra=%d", id_compra, id_compra));

	  res = EjecutarSQL
		(g_strdup_printf
		 ("UPDATE facturas_compras SET fecha_pago=DATE(fecha)+(forma_pago) WHERE id_compra=%d", id_compra));
    }
  else
    {
	  res = EjecutarSQL
		(g_strdup_printf
		 ("UPDATE facturas_compras SET forma_pago=(SELECT compras.forma_pago FROM compra WHERE id=(SELECT "
		  "id_compra FROM guias_compra WHERE numero=%d AND rut_proveedor='%s')) WHERE num_factura='%s' AND "
		  "rut_proveedor='%s'", guia, rut_proveedor, n_doc, rut_proveedor));

	  res = EjecutarSQL
		(g_strdup_printf
		 ("UPDATE facturas_compras SET fecha_pago=DATE(fecha)+(forma_pago) WHERE num_factura='%s'", n_doc));
	}

  res = EjecutarSQL ("SELECT last_value FROM factura_compra_id_seq");

  if (res != NULL)
	return atoi (PQgetvalue (res, 0, 0));
  else
	return -1;
}

gint
IngresarGuia (gchar *n_doc, gint id_compra, gint total,
			  gchar *d_emision, gchar *m_emision, gchar *y_emision)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("INSERT INTO guias_compra VALUES (DEFAULT, %s, %d, 0, (SELECT rut_proveedor FROM compra WHERE id=%d), "
									  "to_timestamp('%.2d %.2d %.2d', 'DD MM YY'))",
									  n_doc, id_compra, id_compra, atoi (d_emision), atoi (m_emision), atoi (y_emision)));

  res = EjecutarSQL ("SELECT last_value FROM guias_compra_id_seq");

  if (res != NULL)
	return atoi (PQgetvalue (res, 0, 0));
  else
	return -1;

}

gboolean
AsignarFactAGuia (gint n_guia, gint id_factura)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("UPDATE guias_compra SET id_factura=%d WHERE numero=%d", id_factura, n_guia));

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

  res = EjecutarSQL (g_strdup_printf ("SELECT descripcion FROM impuestos WHERE id=(SELECT otros FROM producto WHERE barcode='%s')", barcode));

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

  res = EjecutarSQL (g_strdup_printf ("SELECT  precio FROM compra_detalle WHERE "
									  "barcode_product='%s' AND id_compra IN (SELECT id FROM "
									  "compras ORDER BY fecha DESC)", barcode));

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

  res = EjecutarSQL (g_strdup_printf ("SELECT fifo FROM producto WHERE barcode='%s'", barcode));

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

  res = EjecutarSQL (g_strdup_printf ("SELECT * from products_buy_history WHERE id_compra=%s AND cantidad_ingresada<cantidad", compra));

  tuples = PQntuples (res);

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

  res = EjecutarSQL (g_strdup_printf ("SELECT * from products_buy_history WHERE id_compra=%s AND barcode_product='%s' AND cantidad_ingresada<cantidad", compra, barcode));

  tuples = PQntuples (res);

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

  res = EjecutarSQL (g_strdup_printf ("SELECT descripcion FROM impuestos WHERE id=(SELECT otros FROM producto WHERE barcode='%s')", barcode));

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

  res = EjecutarSQL (g_strdup_printf ("SELECT SUM(precio * cantidad) FROM compra_detalle "
									  "WHERE barcode_product='%s'", barcode));

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

  res = EjecutarSQL (g_strdup_printf
					 ("SELECT cantidad, date_part ('day', elaboracion), "
					  "date_part ('month', elaboracion), date_part('year', elaboracion) FROM "
					  "documentos_detalle WHERE barcode='%s' ORDER BY fecha ASC", barcode));

  if (res == NULL)
	return "";

  tuples = PQntuples (res);

  if (tuples == 0)
	return "";

  /*  if (current_stock != 0)
	  {
	  for (i = 0; stock < current_stock; i++)
	  stock += atoi (PQgetvalue (res, i, 0));

	  i--;

	  date = g_strdup_printf ("%.2d/%.2d/%.4d", atoi (PQgetvalue (res, i, 1)),
	  atoi (PQgetvalue (res, i, 2)), atoi (PQgetvalue (res, i, 3)));
	  }*/

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

  /*  if (current_stock != 0)
	  {
	  for (i = 0; stock < current_stock; i++)
	  stock += atoi (PQgetvalue (res, i, 0));

	  i--;

	  date = g_strdup_printf ("%.2d/%.2d/%.4d", atoi (PQgetvalue (res, i, 1)),
	  atoi (PQgetvalue (res, i, 2)), atoi (PQgetvalue (res, i, 3)));
	  } */
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

  res = EjecutarSQL
	(g_strdup_printf ("UPDATE producto SET stock_min=%s, margen_promedio=%s, precio=%s, canje='%d', tasa_canje=%d, "
					  "precio_mayor=%d, cantidad_mayor=%d, mayorista='%d' WHERE barcode='%s'",
					  stock_minimo, margen, new_venta, (gint)canjeable, tasa, precio_mayorista, cantidad_mayorista,
					  (gint)mayorista, barcode));
}

gboolean
Egresar (gint monto, gchar *motivo, gint usuario)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("INSERT INTO egresos VALUES (DEFAULT, %d, (SELECT id FROM tipo_egreso WHERE descrip='%s'), NOW(), %d)",
									  monto, motivo, usuario));

  if (res != NULL)
	return TRUE;
  else
	return FALSE;
}

gboolean
SaveVentaTarjeta (gint id_venta, gchar *insti, gchar *numero, gchar *fecha_venc)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf
					 ("INSERT INTO tarjetas VALUES (DEFAULT, %d, '%s', '%s', '%s')",
					  id_venta, insti, numero, fecha_venc));

  if (res != NULL)
	return TRUE;
  else
	return FALSE;
}

gboolean
Ingreso (gint monto, gchar *motivo, gint usuario)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("INSERT INTO ingresos VALUES (DEFAULT, %d, (SELECT id FROM tipo_ingreso WHERE descrip='%s'), NOW(), %d)",
									  monto, motivo, usuario));

  if (res != NULL)
	return TRUE;
  else
	return FALSE;
}

gboolean
PagarFactura (gchar *num_fact, gchar *rut_proveedor, gchar *descrip)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("UPDATE facturas_compras SET pagada='t' WHERE num_factura=%s AND rut_proveedor='%s'",
									  num_fact, rut_proveedor));

  res = EjecutarSQL
	(g_strdup_printf
	 ("INSERT INTO pagos VALUES ((SELECT id FROM factura_compra WHERE num_factura=%s AND rut_proveedor='%s'), NOW(), 'f', '%s')",
	  num_fact, rut_proveedor,descrip));

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

  res = EjecutarSQL (g_strdup_printf ("INSERT INTO merma VALUES (DEFAULT, '%s', %s, %d)",
									  barcode, CUT (g_strdup_printf ("%f", cantidad)), motivo));

  if (cantidad == 0)
	res = EjecutarSQL
	  (g_strdup_printf
	   ("UPDATE productos SET merma_unid=merma_unid+%s, stock=0 WHERE barcode='%s'",
		CUT (g_strdup_printf ("%f", cantidad)), barcode));
  else
    {
	  gchar *new = CUT (g_strdup_printf ("%f", stock - cantidad));

	  res = EjecutarSQL
		(g_strdup_printf
		 ("UPDATE productos SET merma_unid=merma_unid+%s, stock=stock-%s WHERE barcode='%s'",
		  new, new, barcode));
    }
}

gboolean
Asistencia (gint user_id, gboolean entrada)
{
  PGresult *res;

  if (entrada == TRUE)
	res = EjecutarSQL (g_strdup_printf ("INSERT INTO asistencia VALUES (DEFAULT, %d, NOW(), to_timestamp('0','0'))", user_id));
  else
	res = EjecutarSQL (g_strdup_printf ("UPDATE asistencia SET salida=NOW() WHERE id=(SELECT id FROM asistencia WHERE salida=to_timestamp('0','0') AND id_user=%d ORDER BY entrada DESC LIMIT 1)", user_id));

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

  res = EjecutarSQL (g_strdup_printf
					 ("SELECT fraccion FROM producto WHERE barcode='%s'", barcode));

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

  res = EjecutarSQL (g_strdup_printf
					 ("INSERT INTO formas_pagos VALUES (DEFAULT, '%s', %s)", forma_name, forma_days));

  if (res != NULL)
	return 0;
  else
	return -1;
}

gint
AnularCompraDB (gint id_compra)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf
					 ("UPDATE compras SET anulada='t' WHERE id=%d", id_compra));

  res = EjecutarSQL (g_strdup_printf
					 ("UPDATE products_buy_history SET anulado='t' WHERE id_compra=%d", id_compra));

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
					 ("UPDATE productos SET stock=stock-%s, stock_pro=stock_pro+%s WHERE barcode='%s'",
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

  res = EjecutarSQL (g_strdup_printf
					 ("UPDATE productos SET stock=stock-%s WHERE barcode='%s'",
					  CUT (cantidad), barcode));

  res = EjecutarSQL (g_strdup_printf
					 ("INSERT INTO devoluciones (barcode_product, cantidad) VALUES ('%s', %s)",
					  barcode, CUT (cantidad)));

  if (res != NULL)
	return TRUE;
  else
	return FALSE;
}

gboolean
Recivir (gchar *barcode, gchar *cantidad)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf
					 ("UPDATE productos SET stock=stock+%s WHERE barcode='%s'",
					  CUT (cantidad), barcode));

  res = EjecutarSQL
	(g_strdup_printf
	 ("UPDATE devoluciones SET cantidad_recivida=%s, devuelto='t' WHERE id=(SELECT id FROM devolucion WHERE barocde_product='%s' AND devuelto='f')",
	  CUT (cantidad), barcode));

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

  res = EjecutarSQL
	(g_strdup_printf ("UPDATE proveedores SET nombre='%s', direccion='%s', ciudad='%s', "
					  "comuna='%s', telefono='%s', email='%s', web='%s', contacto='%s', giro='%s'"
					  " WHERE rut='%s'", razon, direccion, ciudad,
					  comuna, fono, email, web, contacto, giro, rut));

  return 0;
}
