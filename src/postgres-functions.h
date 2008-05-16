/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*postgres-functions.h
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
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include<libpq-fe.h>
#ifndef POSTGRES_FUNCTIONS_H

#define POSTGRES_FUNCTIONS_H

#define SPE(string) SpecialChar (string)

#define PQvaluebycol( res, fila, col ) PQgetvalue( res, fila, PQfnumber( res, col ) )

gchar * CutComa (gchar *number);

gchar * PutComa (gchar *number);

PGresult * EjecutarSQL (gchar *sentencia);

gchar * SQLgetvalue( PGresult *res, gint fila, const gchar *col );

gint EjecutarSQLInsert (gchar *sentencia);

gboolean DataExist (gchar *sentencia);

gchar * GetDataByOne (gchar *setencia);

gboolean DeleteProduct (gchar *codigo);

gint SaveSell (gint total, gint machine, gint seller, gint tipo_venta, gchar *rut, gchar *discount,
               gint boleta, gint tipo_documento, gchar *cheque_date,  gboolean cheques, gboolean canceled);

PGresult * SearchTuplesByDate (gint from_year, gint from_month, gint from_day,
                               gint to_year, gint to_month, gint to_day,
                               gchar *date_column, gchar *fields);

gint GetTotalCashSell (guint from_year, guint from_month, guint from_day,
                       guint to_year, guint to_month, guint to_day, gint *total);

gint GetTotalCreditSell (guint from_year, guint from_month, guint from_day,
                         guint to_year, guint to_month, guint to_day, gint *total);

gint GetTotalSell (guint from_year, guint from_month, guint from_day,
                   guint to_year, guint to_month, guint to_day, gint *total);

gboolean InsertClient (gchar *nombres, gchar *paterno, gchar *materno, gchar *rut, gchar *ver,
                       gchar *direccion, gchar *fono, gint credito, gchar *giro);

gboolean RutExist (const gchar *rut);

gint InsertDeuda (gint id_venta, gint rut, gint vendedor);

gint DeudaTotalCliente (gint rut);

PGresult * SearchDeudasCliente (gint rut);

gint CancelarDeudas (gint abonar, gint rut);

gint GetResto (gint rut);

gboolean SaveResto (gint resto, gint rut);

gchar * ReturnClientName (gint rut);

gchar * ReturnClientPhone (gint rut);

gchar * ReturnClientAdress (gint rut);

gchar * ReturnClientCredit (gint rut);

gchar * ReturnClientStatusCredit (gint rut);

gint CreditoDisponible (gint rut);

gchar * ReturnPasswd (gchar *user);

gchar * ReturnLlave (gchar *user);

gint  ReturnUserId (gchar *user);

gint ReturnUserLevel (gchar *user);

gchar * ReturnUsername (gint id);

gboolean SaveNewPassword (gchar *passwd, gchar *user);

gboolean AddNewSeller (gchar *rut, gchar *nombre, gchar *apell_p, gchar *apell_m,
                       gchar *username, gchar *passwd, gchar *id);

gboolean ReturnUserExist (gchar *user);

void ChangeEnableCredit (gboolean status, gint rut);

gboolean ClientDelete (gint rut);

gboolean DataProductUpdate (gchar *barcode, gchar *codigo, gchar *description, gint precio);

gboolean ExistProductHistory (gchar *barcode);

void SaveModifications (gchar *codigo, gchar *description, gchar *marca, gchar *unidad,
                        gchar *contenido, gchar *precio, gboolean iva, gchar *otros, gchar *barcode,
                        gchar *familia, gboolean perecible, gboolean fraccion);

gboolean AddNewProductToDB (gchar *codigo, gchar *barcode, gchar *description, gchar *marca, char *contenido,
                            gchar *unidad, gboolean iva, gchar *otros, gchar *familia, gboolean perecible,
                            gboolean fraccion);

void AgregarCompra (gchar *rut, gchar *nota, gint dias_pago);

void SaveBuyProducts (Productos *header, gint id_compra);

void SaveProductsHistory (Productos *header, gint id_compra);

gboolean CompraIngresada (void);

gboolean IngresarDetalleDocumento (Producto *product, gint compra, gint doc, gboolean factura);

gboolean IngresarProducto (Producto *product, gint compra);

gboolean DiscountStock (Productos *header);

gchar * GetUnit (gchar *barcode);

gdouble GetCurrentStock (gchar *barcode);

char * GetCurrentPrice (gchar *barcode);

gint FiFo (gchar *barcode, gint compra);

gboolean SaveProductsSell (Productos *products, gint id_venta);

PGresult * ReturnProductsRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day);

gboolean AddProveedorToDB (gchar *rut, gchar *nombre, gchar *direccion, gchar *ciudad, gchar *comuna,
                           gchar *telefono, gchar *email, gchar *web, gchar *contacto, gchar *giro);

gboolean SetProductosIngresados (void);

gdouble GetDayToSell (gchar *barcode);

gint GetMinStock (gchar *barcode);

gboolean SaveDataCheque (gint id_venta, gchar *serie, gint number, gchar *banco, gchar *plaza,
                         gint monto, gint day, gint month, gint year);

gint ReturnIncompletProducts (gint id_venta);

Proveedor * ReturnProveedor (gint id_compra);

gint IngresarFactura (gint n_doc, gint id_compra, gchar *rut_proveedor, gint total, gint d_emision, gint m_emision, gint y_emision, gint guia);

gint IngresarGuia (gint n_doc, gint id_compra, gint total, gint d_emision, gint m_emision, gint y_emision);

gboolean AsignarFactAGuia (gint n_guia, gint id_factura);

gdouble GetIVA (gchar *barcode);

gdouble GetOtros (gchar *barcode);

gint GetOtrosIndex (gchar *barcode);

gchar * GetOtrosName (gchar *barcode);

gint GetNeto (gchar *barcode);

gint GetFiFo (gchar *barcode);

gboolean CheckCompraIntegrity (gchar *compra);

gboolean CheckProductIntegrity (gchar *compra, gchar *barcode);

gint GetFami (gchar *barcode);

gchar * GetLabelImpuesto (gchar *barcode);

gchar * GetLabelFamilia (gchar *barcode);

gchar * GetPerecible (gchar *barcode);

gint GetTotalBuys (gchar *barcode);

gchar * GetElabDate (gchar *barcode, gint current_stock);

gchar * GetVencDate (gchar *barcode, gint current_stock);

gint InversionAgregada (gchar *barcode);

gchar * InversionTotalStock (void);

gchar * ValorTotalStock (void);

gchar * ContriTotalStock (void);

void SetModificacionesProducto (gchar *barcode, gchar *stock_minimo, gchar *margen, gchar *new_venta,
                                gboolean canjeable, gint tasa, gboolean mayorista, gint precio_mayorista,
                                gint cantidad_mayorista);

gboolean Egresar (gint monto, gchar *motivo, gint usuario);

gboolean SaveVentaTarjeta (gint id_venta, gchar *insti, gchar *numero, gchar *fecha_venc);

gboolean Ingreso (gint monto, gchar *motivo, gint usuario);

gboolean PagarFactura (gchar *num_fact, gchar *rut_proveedor, gchar *descrip);

void AjusteStock (gdouble cantidad, gint motivo, gchar *barcode);

gboolean Asistencia (gint user_id, gboolean entrada);

gboolean VentaFraccion (gchar *barcode);

gint AnularCompraDB (gint id_compra);

gint CanjearProduct (gchar *barcode, gdouble cantidad);

gboolean Devolver (gchar *barcode, gchar *cantidad);

gboolean Recivir (gchar *barcode, gchar *cantidad);

gint SetModificacionesProveedor (gchar *rut, gchar *razon, gchar *direccion, gchar *comuna,
                                 gchar *ciudad, gchar *fono, gchar *web, gchar *contacto,
                                 gchar *email, gchar *giro);
#endif
