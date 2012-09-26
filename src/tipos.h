/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*tipos.h
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

#include<gtk/gtk.h>

#ifndef TIPOS_H

#define TIPOS_H

GtkBuilder *builder;

GtkWidget *main_window;

gchar *config_profile;

// Solo vamos a tener un pequeño display de ventas ?
gboolean solo_venta;

enum sell_type {SIMPLE, FACTURA, GUIA, VENTA};

enum pay_way {CASH, CREDITO, CHEQUE_RESTAURANT, MIXTO, CHEQUE, TARJETA}; //CHEQUE_RESTAURANT = No Afecto Impuesto

enum action {ADD, MOD, DEL};

typedef struct _main_box
{
  GtkWidget *main_box;
  GtkWidget *new_box;
}
MainBox;

typedef struct _product
{
  gchar *codigo;
  gchar *producto;
  gchar *marca;
  gint id;
  gdouble fifo;
  gdouble precio;
  gdouble margen;
  gdouble precio_neto;
  gdouble precio_compra;
  gdouble cantidad;
  gdouble stock;
  gchar *unidad;
  gint contenido;
  gchar *barcode;
  gint lugar;
  gint tipo;
  gint familia;

  /* Venta por fraccion */
  gboolean fraccion;

  /* Impuestos */
  gboolean impuestos; //Se usa en venta
  gint iva;
  gint otros;
  gint otros_id;

  /* En ventas Mixtas, su proporción de impuesto 
     (de los productos que sí pagan impuestos)
     con respecto al total de pago afecto a impuesto */
  gdouble proporcion_afecta_imp;
  gdouble proporcion_no_afecta_imp;

  /*
    Solo para usar en compras
  */
  gboolean ingresar;
  gboolean perecible;

  /*
    Elaboracion y Vencimiento
  */
  gint elab_day;
  gint elab_month;
  gint elab_year;
  gint venc_day;
  gint venc_month;
  gint venc_year;

  GtkTreeIter iter;

  /*
    Canjeable ?
   */
  gboolean canjeable;
  gint stock_pro;

  /*
    Solo usar en compras
  */
  gboolean canjear;
  gint cuanto;
  gint tasa_canje;

  /*
     Datos para cantidades mayoristas
  */
  gboolean mayorista;
  gint precio_mayor;
  gint cantidad_mayorista;
}
Producto;

typedef struct _products
{
  struct _products *back;
  Producto *product;
  struct _products *next;
}
Productos;

typedef struct _ventas
{
  /*
    The vars to save the list of products;
   */
  Productos *header;
  Productos *products;

  Productos *product_check;

  /*
    The widget for the entry of
    product code and description product
    and the gchar to save the value
   */
  GtkWidget *codigo_entry;
  GtkWidget *product_entry;
  GtkWidget *cantidad_entry;
  GtkWidget *barcode_entry;

  gchar *codigo;
  gchar *product;

  GtkWidget *boleta;
  GtkWidget *vendedor;
  GtkWidget *stockday;

  /*
    The Widget for the "Precio" label
    and "Total" label
  */

  GtkWidget *precio_label;
  GtkWidget *mayor_label;
  GtkWidget *mayor_cantidad;
  GtkWidget *total_label;
  GtkWidget *sub_total_label;
  GtkWidget *subtotal_label;
  GtkWidget *stock_label;

  /*
    The TreeView for the products
    and other things
   */
  GtkWidget *treeview_products;
  GtkListStore *store;

  /*
    Sell type
   */
  GtkWidget *window;
  GtkWidget *entry_rut;

  GtkWidget *sell_button;

  GtkWidget *entry_paga;
  GtkWidget *label_vuelto;

  gboolean tipo_venta;

  GtkWidget *client_label;
  GtkWidget *name_label;

  GtkWidget *client_venta;
  GtkWidget *client_vender;


  /*
   */
  GtkWidget *products_window;

  GtkWidget *products_tree;
  GtkListStore *products_store;

  /*
    Window Client Selection
   */

  GtkWidget *clients_window;

  GtkWidget *clients_tree;
  GtkListStore *clients_store;

  GtkWidget *find_button;

  /*
    Window Product Search
   */
  GtkListStore *search_store;
  GtkTreeSelection *search_selection;

  GtkWidget *buscar_entry;

  /*
    Discount
   */
  GtkWidget *sencillo;
  GtkWidget *discount_entry;

  /*
    Datos Venta;
  */

  GtkWidget *venta_rut;
  GtkWidget *venta_nombre;
  GtkWidget *venta_direccion;
  GtkWidget *venta_fono;
  GtkWidget *venta_pago;
  GtkWidget *forma_pago;

  /*
    Cheque Window
  */
  gboolean cheque;
  GtkWidget *cheque_window;
  GtkWidget *cheque_rut;
  GtkWidget *cheque_nombre;
  GtkWidget *cheque_direccion;
  GtkWidget *cheque_fono;
  GtkWidget *cheque_serie;
  GtkWidget *cheque_numero;
  GtkWidget *cheque_banco;
  GtkWidget *cheque_plaza;
  GtkWidget *cheque_monto;

  /*
    Datos Tarjeta
  */
  GtkWidget *tarjeta_inst;
  GtkWidget *tarjeta_numero;
  GtkWidget *tarjeta_fecha;

  /*
    Datos Factura
  */
  GtkWidget *factura_cliente;
  GtkWidget *factura_address;
  GtkWidget *factura_giro;

  /*
    Cancelar venta
   */
  GtkWidget *cancel_window;
  GtkListStore *cancel_store;
  GtkListStore *cancel_store_details;
  GtkWidget *cancel_tree;
  GtkWidget *cancel_tree_details;
}
Venta;

Venta *venta;

//Información cheques de restaurant
typedef struct _cheque_restaurant
{
  gchar *codigo;
  gchar *fecha_vencimiento;
  gint monto;
  gint lugar;

  GtkTreeIter iter;
} 
ChequeRest;

ChequeRest *cheque_rest;

typedef struct _cheques_restaurant
{
  struct _cheques_restaurant *back;
  ChequeRest *cheque;
  struct _cheques_restaurant *next;
}
ChequesRestaurant;

ChequesRestaurant *cheques_restaurant;

//Información cheques de restaurant
typedef struct _pago_cheques_restaurant
{
  //Lista cheques restaurant
  ChequesRestaurant *header;
  ChequesRestaurant *cheques;

  ChequesRestaurant *cheques_check;
    
  //Emisor del cheque
  gchar *rut_emisor;
  gchar *nombre_emisor;
  gint id_emisor;

  //El treeview correspondiente
  GtkTreeView *treeview;
  GtkListStore *store;
  GtkTreeSelection *selection;
}
PagoChequesRest;

PagoChequesRest *pago_chk_rest;


typedef struct _pago_mixto
{
  /*Primer Pago*/
  gint tipo_pago1;
  /*Datos cheque restaurant*/
  PagoChequesRest *check_rest1;
  /*Datos credito*/
  gchar *rut_credito1;
  gint monto_pago1;
    
  /*Segundo Pago*/
  gint tipo_pago2;
  /*Datos cheque restaurant*/
  PagoChequesRest *check_rest2;
  /*Datos credito*/
  gchar *rut_credito2;
  gint monto_pago2;

  gint total_a_pagar;
}
PagoMixto;

PagoMixto *pago_mixto;


typedef struct _ingreso_producto
{
  GtkWidget *codigo_entry;
  GtkWidget *product_entry;
  GtkWidget *precio_entry;
  GtkWidget *barcode_entry;
  GtkWidget *stock_entry;
  GtkWidget *marca_entry;
  GtkWidget *cantidad_entry;
  GtkWidget *unidad_entry;

  /*
    The widget for the window to
    list the products
  */
  GtkWidget *products_window;

  GtkWidget *treeview_products;
  GtkListStore *store;

  GtkWidget *buscar_entry;

  /*
    The entrys to edit the data
   */
  GtkWidget *codigo_entry_edit;
  GtkWidget *product_entry_edit;
  GtkWidget *precio_entry_edit;

  GtkTreeSelection *selection;
}
IngresoProducto;

IngresoProducto *ingreso;

/*Para almacenar los cambios de una factura
  o traspaso ya ingresados*/
typedef struct _prod
{
  gchar *barcode;
  gchar *descripcion;

  /*Datos de la compra o traspaso al que pertenece*/
  //gint id_compra;
  gchar *fecha_compra;
  gint id_factura_compra;
  //gint id_factura_detalle;
  gint id_traspaso;

  /*Costos y cantidades se inicializan en 0*/
  gdouble costo_original;
  gdouble costo_nuevo;
  gdouble cantidad_original;
  gdouble cantidad_nueva;

  gint accion;
  gint lugar;
}
Prod;

/*Lista de productos a modificar*/
typedef struct _prods
{
  struct _prods *back;
  Prod *prod;
  struct _prods *next;
}
Prods;

typedef struct _lista_mod_prod
{
  Prods *header;
  Prods *check;
  Prods *prods;
}
ListaModProd;

ListaModProd *lista_mod_prod;


typedef struct _ventas_stats
{
  Productos *header;
  Productos *products;

  GtkWidget *calendar;

  GtkTreeStore *store_ventas_stats;
  GtkWidget *tree_ventas_stats;

  guint selected_from_year;
  guint selected_from_month;
  guint selected_from_day;

  guint selected_to_year;
  guint selected_to_month;
  guint selected_to_day;

  GtkWidget *total_sell_label;
  GtkWidget *total_cash_sell_label;
  GtkWidget *total_credit_sell_label;

  GtkWidget *from;
  GtkWidget *to;

  GtkTreeStore *store_venta;
  GtkWidget *tree_venta;

  GtkTreeSelection *selection;

  GtkWidget *rank_tree;
  GtkListStore *rank_store;

  GtkListStore *store_caja;
  GtkWidget *tree_caja;

  /*
    Proveedores
   */
  GtkListStore *proveedores_store;
  GtkWidget *proveedores_tree;

  GtkWidget *proveedor_razon;
  GtkWidget *proveedor_direccion;
  GtkWidget *proveedor_fono;
  GtkWidget *proveedor_web;
  GtkWidget *proveedor_giro;
  GtkWidget *proveedor_comuna;
  GtkWidget *proveedor_email;
  GtkWidget *proveedor_observaciones;

  GtkTreeStore *facturas_store;
  GtkWidget *facturas_tree;


  GtkListStore *store_morosos;
  GtkWidget *tree_morosos;

  GtkTreeStore *pagar_store;
  GtkWidget *pagar_tree;

  GtkTreeStore *pagadas_store;
  GtkWidget *pagadas_tree;
}
VentasStats;

VentasStats *ventastats;

typedef struct _creditos
{
  GtkListStore *store;
  GtkWidget *tree;

  GtkTreeSelection *selection;

  GtkWidget *search_entry;

  /*
    Entrys for the new client
    And the window
   */

  GtkWidget *entry_nombres;
  GtkWidget *materno;
  GtkWidget *paterno;
  GtkWidget *rut;
  GtkWidget *rut_ver;
  GtkWidget *direccion;
  GtkWidget *fono;
  GtkWidget *giro;
  GtkWidget *credito;

  GtkWidget *window;

  GtkWidget *deuda;
  GtkWidget *abono;
  GtkWidget *deuda_total;

  GtkListStore *detalle;
  GtkWidget *tree_detalle;

  GtkListStore *ventas;
  GtkWidget *tree_ventas;
  GtkTreeSelection *selection_ventas;

  GtkWidget *abonar_window;
  GtkWidget *entry_plata;

  /*
    Client Status
   */
  GtkWidget *status_window;

}
Creditos;

Creditos *creditos;

typedef struct _user
{
  gint user_id;
  gint level;
  gchar *user;
}
User;

User *user_data;

typedef struct _compra
{
  GtkWidget *proveedor_entry;
  GtkWidget *barcode_entry;
  GtkWidget *codigo_entry;
  GtkWidget *producto_entry;
  GtkWidget *marca_entry;
  GtkWidget *unidad_entry;
  GtkWidget *contenido_entry;
  GtkWidget *precio_unitario_entry;
  GtkWidget *total_compra_entry;
  GtkWidget *n_pedido_entry;
  GtkWidget *cantidad_entry;

  GtkWidget *total_compra;

  /*
    History
   */
  GtkWidget *barcode_history_entry;

  GtkWidget *tree_history;
  GtkListStore *store_history;

  /*
    Product Window
   */
  GtkWidget *see_button;

  GtkWidget *see_window;
  GtkWidget *see_codigo;
  GtkWidget *see_barcode;
  GtkWidget *see_description;
  GtkWidget *see_marca;
  GtkWidget *see_precio;
  GtkWidget *see_contenido;
  GtkWidget *see_unidad;

  GtkWidget *product;
  GtkWidget *marca;
  GtkWidget *unidad;
  GtkWidget *stock;
  GtkWidget *stockday;
  GtkWidget *current_price;
  GtkWidget *fifo;

  /*
    Products List
   */
  GtkWidget *tree_list;
  GtkListStore *store_list;

  Productos *header;
  Producto *current;
  Productos *products_list;

  /*
    Circular productos a comprar
   */
  Productos *header_compra;
  Productos *products_compra;

  /*
    Add New Product
  */
  GtkWidget *new_window;

  GtkWidget *new_codigo;
  GtkWidget *new_barcode;
  GtkWidget *new_description;
  GtkWidget *new_marca;
  GtkWidget *new_contenido;
  GtkWidget *new_unidad;

  /*
    Buy Window
   */
  GtkWidget *buy_window;

  GtkWidget *rut_label;
  GtkWidget *nombre_label;
  GtkWidget *nota_entry;

  /*
    Search Window
   */
  GtkWidget *find_win;
  GtkListStore *find_store;
  GtkWidget *find_tree;

  /*
    Ingreso de Compras
  */
  GtkWidget *ingreso_tree;
  GtkListStore *ingreso_store;

  GtkWidget *compra_tree;
  GtkListStore *compra_store;

  GtkWidget *total_neto;
  GtkWidget *total_iva;
  GtkWidget *total_otros;
  GtkWidget *total;

  /*
    Proveedores
   */
  GtkWidget *tree_prov;
  GtkListStore *store_prov;

  GtkWidget *rut;
  GtkWidget *proveedor;

  /*
    Add Proveedores
   */

  GtkWidget *rut_add;
  GtkWidget *rut_ver;
  GtkWidget *nombre_add;
  GtkWidget *direccion_add;
  GtkWidget *ciudad_add;
  GtkWidget *comuna_add;
  GtkWidget *telefono_add;
  GtkWidget *email_add;
  GtkWidget *web_add;
  GtkWidget *contacto_add;
  GtkWidget *giro_add;

  /* Tipo de Documento de Ingreso */
  GtkWidget *documentos;
  GtkWidget *n_documento;
  GtkWidget *fecha_emision_d;
  GtkWidget *fecha_emision_m;
  GtkWidget *fecha_emision_y;
  GtkWidget *fecha_d;
  GtkWidget *fecha_m;
  GtkWidget *fecha_y;
  GtkWidget *monto_documento;
  GtkWidget *error_documento;

  GtkWidget *tree_document;
  GtkListStore *store_document;

  /*
    Es o no factura?
  */

  gboolean factura;
  gboolean n_doc;

  /*
    Facturas
  */

  GtkWidget *fact_proveedor;
  GtkWidget *fact_rut;
  GtkWidget *fact_contacto;
  GtkWidget *fact_direccion;
  GtkWidget *fact_comuna;
  GtkWidget *fact_fono;
  GtkWidget *fact_email;
  GtkWidget *fact_web;
  GtkWidget *win_proveedor;
  GtkWidget *n_factura;

  GtkWidget *tree_guias;
  GtkTreeStore *store_guias;

  GtkWidget *tree_det_guias;
  GtkTreeStore *store_det_guias;

  GtkWidget *fact_neto;
  GtkWidget *fact_iva;
  GtkWidget *fact_otros;
  GtkWidget *fact_total;

  GtkWidget *fact_monto;

  /* Temp */
  GtkWidget *tree_new_guias;
  GtkTreeStore *store_new_guias;
  /*
    Pagos
  */

  GtkWidget *tree_facturas;
  GtkTreeStore *store_facturas;

  /*
   Mensajes de Error
  */

  GtkWidget *guias_error;


  /*
    Vars para informe compras;
  */
  GtkWidget *informe_tree;
  GtkTreeStore *informe_store;

}
Compra;

Compra *compra;

typedef struct _proveedor {
  gchar *nombre;
  gchar *rut;
  gchar *direccion;
  gchar *comuna;
  gchar *telefono;
  gchar *email;
  gchar *web;
  gchar *contacto;
  gchar *ciudad;
  gchar *giro;
}
Proveedor;

typedef struct _caja
{
  gint current; /* We got the current status of the "caja" */

  GtkWidget *win;
  GtkWidget *entry_inicio;
}
Caja;

Caja *caja;

/*
  All the dimensions need two values x and y, this it's because
  we localize the place to write a value in the paper with these values
*/
typedef struct _Position
{
  float x;
  float y;
} Position;

typedef struct _dimensions
{
  Position *factura_size;
  Position *factura_cliente;
  Position *factura_address;
  Position *factura_giro;
  Position *factura_rut;
  Position *factura_comuna;
  Position *factura_fono;

  Position *factura_detail_begin;
  Position *factura_detail_end;
  Position *factura_codigo;
  Position *factura_descripcion;
  Position *factura_cantidad;
  Position *factura_precio_uni;
  Position *factura_precio_total;

  Position *factura_subtotal;
  Position *factura_iva;
  Position *factura_total;

}
Dimensions;

Dimensions *dimension;

#endif

