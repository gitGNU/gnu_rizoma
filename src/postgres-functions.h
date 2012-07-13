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

#define DD(string) DropDelimiter (string)

#define PQvaluebycol( res, fila, col ) PQgetvalue( res, fila, PQfnumber( res, col ) )

gchar * CutComa (gchar *number);

gchar * PutComa (gchar *number);

PGresult * EjecutarSQL (gchar *sentencia);

PGresult * EjecutarSQL2 (gchar *sentencia);

gchar * SQLgetvalue( PGresult *res, gint fila, const gchar *col );

gint EjecutarSQLInsert (gchar *sentencia);

gboolean DataExist (gchar *sentencia);

gchar * GetDataByOne (gchar *setencia);

gboolean DeleteProduct (gchar *codigo);

gint SaveSell (gint total, gint machine, gint seller, gint tipo_venta, gchar *rut, gchar *discount,
               gint boleta, gint tipo_documento, gchar *cheque_date,  gboolean cheques, gboolean canceled);

PGresult * SearchTuplesByDate (gint from_year, gint from_month, gint from_day,
                               gint to_year, gint to_month, gint to_day,
                               gchar *date_column, gchar *fields, gchar *grupo);

PGresult * exempt_sells_on_date (gint from_year, gint from_month, gint from_day,
                                 gint to_year, gint to_month, gint to_day);

gint GetTotalCashSell (guint from_year, guint from_month, guint from_day,
                       guint to_year, guint to_month, guint to_day, gint *total);

gint GetTotalCreditSell (guint from_year, guint from_month, guint from_day,
                         guint to_year, guint to_month, guint to_day, gint *total);

void total_taxes_on_time_interval (guint from_year, guint from_month, guint from_day,
                                   guint to_year, guint to_month, guint to_day, 
                                   gint *total_iva, gint *total_otros);

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

gint ReturnClientCredit (gint rut);

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
                        gchar *contenido, gchar *precio, gboolean iva, int otros, gchar *barcode,
                        gboolean perecible, gboolean fraccion, gint familia);

gboolean AddNewProductToDB (gchar *codigo, gchar *barcode, gchar *description, gchar *marca, char *contenido,
                            gchar *unidad, gboolean iva, gint otros, gint familia, gboolean perecible,
                            gboolean fraccion, gint tipo);

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

gdouble FiFo (gchar *barcode, gint compra);

gboolean SaveProductsSell (Productos *products, gint id_venta, gint tipo_venta);

PGresult * ReturnTransferRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gboolean traspaso_envio);

PGresult * ReturnMpTransferRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gboolean traspaso_envio);

PGresult * ReturnMcTransferRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gboolean traspaso_envio);

PGresult * ReturnDerivTransferRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gchar *barcode_madre);

PGresult * ReturnCompTransferRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gchar *barcode_madre);

PGresult * ReturnProductsRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gint family);

PGresult * ReturnMpProductsRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gint family);

PGresult * ReturnMcProductsRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gint family);

PGresult * ReturnDerivProductsRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gchar *barcode_madre);

PGresult * ReturnCompProductsRank (gint from_year, gint from_month, gint from_day, gint to_year, gint to_month, gint to_day, gchar *barcode_madre);

gboolean AddProveedorToDB (gchar *rut, gchar *nombre, gchar *direccion, gchar *ciudad, gchar *comuna,
                           gchar *telefono, gchar *email, gchar *web, gchar *contacto, gchar *giro);

gboolean SetProductosIngresados (void);

gdouble GetDayToSell (gchar *barcode);

gint GetMinStock (gchar *barcode);

gboolean SaveDataCheque (gint id_venta, gchar *serie, gint number, gchar *banco, gchar *plaza,
                         gint monto, gint day, gint month, gint year);

gint ReturnIncompletProducts (gint id_venta);

Proveedor * ReturnProveedor (gint id_compra);

gint IngresarFactura (gint n_doc, gint id_compra, gchar *rut_proveedor, gint total_productos, gint d_emision, gint m_emision, gint y_emision, gint guia, gint costo_bruto_transporte);

gint IngresarGuia (gint n_doc, gint id_compra, gint d_emision, gint m_emision, gint y_emision);

gboolean AsignarFactAGuia (gint id_guia, gint id_factura);

gdouble GetIVA (gchar *barcode);

gdouble GetOtros (gchar *barcode);

gint GetOtrosIndex (gchar *barcode);

gchar * GetOtrosName (gchar *barcode);

gdouble GetNeto (gchar *barcode);

gdouble GetFiFo (gchar *barcode);

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

void SetModificacionesProducto (gchar *barcode, gchar *dias_stock, gchar *margen, gchar *new_venta,
                                gboolean canjeable, gint tasa, gboolean mayorista, gint precio_mayorista,
                                gint cantidad_mayorista);

gboolean Egresar (gint monto, gint motivo, gint usuario);

gboolean SaveVentaTarjeta (gint id_venta, gchar *insti, gchar *numero, gchar *fecha_venc);

gboolean Ingreso (gint monto, gint motivo, gint usuario);

gboolean PagarFactura (gint id_invoice);

void AjusteStock (gdouble cantidad, gint motivo, gchar *barcode);

gboolean Asistencia (gint user_id, gboolean entrada);

gboolean VentaFraccion (gchar *barcode);

gint AnularCompraDB (gint id_compra);

gint CanjearProduct (gchar *barcode, gdouble cantidad);

gboolean Devolver (gchar *barcode, gchar *cantidad);

gboolean Recibir (gchar *barcode, gchar *cantidad);

gint SetModificacionesProveedor (gchar *rut, gchar *razon, gchar *direccion, gchar *comuna,
                                 gchar *ciudad, gchar *fono, gchar *web, gchar *contacto,
                                 gchar *email, gchar *giro, gchar *lap_rep);

gboolean provider_exist (const gchar *rut);

gint users_working (void);

gint nullify_sale (gint sale_id);

gint get_last_cash_box_id ();

gdouble TotalPrecioCompra (Productos *products);

gint SaveTraspasoCompras (gdouble total, gint origen, gint vendedor, gint destino, gboolean tipo_traspaso);

gboolean emisor_delete (gint id);

gboolean insert_emisores (gchar *rut, gchar *dv, gchar *razon_social, gchar *telefono, gchar *direccion,
                          gchar *comuna, gchar *ciudad, gchar *giro);

gboolean fact_cheque_rest (gint id);

gint InsertNewDocument (gint sell_id, gint document_type, gint sell_type);

gboolean InsertNewDocumentDetail (gint document_id, gchar *barcode, gint precio, gdouble cantidad);

gboolean ingresar_cheques (gint id_emisor, gint id_venta, ChequesRestaurant *header);

gboolean SaveProductsDevolucion (Productos *products, gint id_devolucion);

gboolean SaveProductsTraspaso (Productos *products, gint id_traspaso, gboolean tipo_traspaso);

gboolean asociar_derivada_a_madre (gchar *barcode_madre, gint tipo_madre, gchar *barcode_comp_der, 
                                   gint tipo_comp_der, gdouble cant_mud);

gchar * sugerir_codigo (gchar *codigo, guint min_lenght, guint max_lenght);

gboolean codigo_disponible (gchar *code);

gint get_last_sell_id ();

gboolean SaveDevolucion (gint total, gint rut);

gchar * ReturnNegocio ();

gint InsertIdTraspaso ();

gint ReturnBodegaID (gchar *destino);

gint SaveTraspaso (gdouble total, gint origen, gint vendedor, gint destino, gboolean tipo_traspaso);

void registrar_nuevo_codigo (gchar *codigo);

void registrar_nuevo_color (gchar *codigo, gchar *color);

void registrar_nueva_talla (gchar *codigo, gchar *talla);

void registrar_nuevo_sub_depto (gchar *codigo, gchar *sub_depto);

PGresult * getProductsByProvider (gchar *rut);

PGresult * get_product_information (gchar *barcode, gchar *codigo_corto, gchar *columnas);

gdouble get_last_buy_price (gchar *barcode);

gdouble get_last_buy_price_to_invoice (gchar *barcode, gint last_invoice_id);

gdouble cantidad_compra_es_modificable (gchar *barcode, gdouble cantidad_nueva, gint id_factura_compra);

gdouble cantidad_traspaso_es_modificable (gchar *barcode, gdouble cantidad_original, gdouble cantidad_nueva, gint id_traspaso, int origen);

gboolean mod_to_mod_on_buy (Prod *producto);

gboolean mod_to_add_on_buy (Prod *producto);

gboolean mod_to_del_on_buy (Prod *producto);

gchar * codigo_corto_to_barcode (gchar *codigo_corto);

PGresult * get_componentes_compuesto (gchar *barcode);

gboolean desasociar_madre_hijo (gchar *barcode_madre, gchar * barcode_comp_der);

gboolean mod_to_mod_on_transfer (Prod *producto);

#endif
