/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
/*caja.c
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
#include<stdlib.h>
#include<string.h>
#include<tipos.h>

#include"postgres-functions.h"

#include"errors.h"
#include"caja.h"
#include"utils.h"

GtkWidget *calendar_win;
guint day, month, year;

GtkWidget *inicio_caja;
GtkWidget *ventas_efect;
GtkWidget *ventas_doc;
GtkWidget *pago_ventas;
GtkWidget *otros_ingresos;
GtkWidget *total_haberes;

GtkWidget *pagos;
GtkWidget *retiros;
GtkWidget *gastos_corrientes;
GtkWidget *otros_egresos;
GtkWidget *total_debitos;

GtkWidget *total_caja;

GtkWidget *combo_egreso;
GtkWidget *combo_ingreso;


/**
 * Es llamada por la funcion EgresarDinero.
 *
 * Esta funcion retorna el valor de inicio de la caja
 *
 * @return caja: entero que contiene el resultado de la consulta a la caja
 *
 */

gint
ReturnSaldoCaja (void)
{
  PGresult *res;
  gint caja;

  res = EjecutarSQL ("select * from get_arqueo_caja(-1)");

  if (res != NULL && PQntuples (res) != 0)
    caja = atoi (PQgetvalue(res, 0, 0));
  else
    caja = 0;

  return caja;
}

/**
 * Es llamada por la funcion IngresarDinero.
 *
 * Esta funcion cierra la ventana "wnd_caja_ingreso" y limpia la caja de
 * texto "entry_caja_in_amount"
 *
 */

void
CloseVentanaIngreso(void)
{
  GtkWidget *wid;

  wid = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_in_amount"));
  gtk_entry_set_text(GTK_ENTRY(wid), "");

  wid = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_caja_ingreso"));
  gtk_widget_hide(wid);
}

/**
 * Es llamada cuando el boton "btn_ingresarDinero" es presionado (signal click).
 *
 * Esta funcion carga el valor desde la caja de texto "entry_caja_in_amount"
 * en monto  y luego llama a la funcion Ingreso, que realiza el ingreso de
 * dinero y su motivo, finalemente llama a la funcion CloseVentanaIngreso
 * para cerrar la ventana.
 *
 * @param button the button
 * @param user_data the user data
 *
 */

void
IngresarDinero (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint monto;
  gint motivo;

  
  /*De estar habilitada caja, se asegura que Ã©sta se encuentre 
    abierta al momento de vender*/
  
  if (rizoma_get_value_boolean ("CAJA"))
    if (check_caja()) // Se abre la caja en caso de que estÃ© cerrada
      open_caja (TRUE);

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_caja_in_motiv"));
  model = gtk_combo_box_get_model(GTK_COMBO_BOX(aux_widget));
  if (!(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(aux_widget), &iter)))
    {
      ErrorMSG(aux_widget, "Debe seleccionar un tipo de ingreso");
      return;
    }

  gtk_tree_model_get (model, &iter,
                      0, &motivo,
                      -1);

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_in_amount"));
  monto = atoi(gtk_entry_get_text(GTK_ENTRY(aux_widget)));

  if (monto == 0)
    {
      ErrorMSG (data, "No pueden haber ingresos de $0");
      return;
    }

  if (Ingreso (monto, motivo, user_data->user_id))
    CloseVentanaIngreso();
  else
    {
      ErrorMSG(aux_widget, "No fue posible registrar el ingreso de dinero en la caja");
      return;
    }
}

/**
 * Es llamada por la funcion "IniciarLaCaja".
 *
 * Esta funcion visualiza la ventana "wnd_caja_ingreso" y  carga en la lista
 * desplegable(combobox) los motivos del ingreso de dinero a la caja, 
 *
 * @param monto: entero que contiene el monto de inicio de la caja
 *
 */

void
VentanaIngreso (gint monto)
{
  GtkWidget *aux_widget;
  PGresult *res;
  gint tuples, i;
  GtkListStore *store;
  GtkTreeIter iter;

  res = EjecutarSQL ("SELECT id, descrip FROM tipo_ingreso");
  tuples = PQntuples (res);

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_caja_in_motiv"));
  store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(aux_widget)));

  if (store == NULL)
    {
      GtkCellRenderer *cell;
      store = gtk_list_store_new (2,
                                  G_TYPE_INT,
                                  G_TYPE_STRING);

      gtk_combo_box_set_model(GTK_COMBO_BOX(aux_widget), GTK_TREE_MODEL(store));

      cell = gtk_cell_renderer_text_new();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(aux_widget), cell, TRUE);
      gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(aux_widget), cell,
                                     "text", 1,
                                     NULL);
    }

  gtk_list_store_clear(store);

  for (i=0 ; i < tuples ; i++)
    {
      gtk_list_store_append(store, &iter);
      gtk_list_store_set(store, &iter,
                         0, atoi(PQvaluebycol(res, i, "id")),
                         1, PQvaluebycol(res, i, "descrip"),
                         -1);
    }

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_in_amount"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), g_strdup_printf("%d", monto));
  gtk_widget_grab_focus(aux_widget);

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_caja_ingreso"));
  gtk_widget_show_all(aux_widget);
}

/**
 * Es llamada cuando el boton "btn_egresarDinero" es presionado (signal
 * click).
 *
 * Esta funcion carga el monto que se egreso y luego a llama a la funcion
 * Egreso que realiza el egreso y el motivo
 *
 * @param button the button
 * @param user_data the user data
 *
 */

void
EgresarDinero (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  gint active;
  gint monto;
  gint motivo;

  GtkTreeModel *model;
  GtkTreeIter iter;
  
  
  /*De estar habilitada caja, se asegura que Ã©sta se encuentre 
    abierta al momento de vender*/
  
  if (rizoma_get_value_boolean ("CAJA"))
    if (check_caja()) // Se abre la caja en caso de que estÃ© cerrada
      open_caja (TRUE);

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_out_amount"));
  monto = atoi (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_caja_out_motiv"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (aux_widget));

  if (monto == 0)
    ErrorMSG (aux_widget, "No pueden haber egresos de $0");
  else if (active == -1)
    ErrorMSG (aux_widget, "Debe Seleccionar un tipo de egreso");
  

  else if (monto > ReturnSaldoCaja ())
    ErrorMSG (aux_widget, "No se puede retirar más dinero del que hay en caja" );

  else
    {
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (aux_widget));
      gtk_combo_box_get_active_iter (GTK_COMBO_BOX (aux_widget), &iter);

      gtk_tree_model_get (model, &iter,
                          0, &motivo,
                          -1);

      if (Egresar (monto, motivo, user_data->user_id))
        CloseVentanaEgreso();
      else
        ErrorMSG(aux_widget, "No fue posible ingresar el egreso de dinero de la caja");
    }
}

/**
 * Es llamada por la funcion EgresarDinero.
 *
 * Esta funcion cierra la ventana "wnd_caja_egreso" y limpia la caja de
 * texto "entry_caja_out_amount"
 *
 */

void
CloseVentanaEgreso (void)
{
  GtkWidget *wid;

  wid = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_out_amount"));
  gtk_entry_set_text(GTK_ENTRY(wid), "");

  wid = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_caja_egreso"));
  gtk_widget_hide(wid);
}


/**
 * Es llamada por la funcion "IniciarLaCaja".
 *
 * Esta funcion visualiza la ventana "wnd_caja_egreso" y  carga en la lista
 * desplegable(combobox) los motivos del ingreso de dinero a la caja, 
 *
 * @param monto: entero que contiene el monto de inicio de la caja
 *
 */
void
VentanaEgreso (gint monto)
{
  GtkWidget *combo;
  GtkWidget *aux_widget;
  GtkListStore *store;
  GtkTreeIter iter;
  PGresult *res, *resId;
  gint tuples, i;
  gint nulVenId; // El Id de "Nulidad de venta" 

  res = EjecutarSQL ("SELECT id, descrip FROM tipo_egreso");
  tuples = PQntuples (res);

  combo = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_caja_out_motiv"));
  store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo)));

  if (store == NULL)
    {
      GtkCellRenderer *cell;
      store = gtk_list_store_new(2,
                                 G_TYPE_INT,
                                 G_TYPE_STRING);

      gtk_combo_box_set_model(GTK_COMBO_BOX(combo),GTK_TREE_MODEL(store));

      cell = gtk_cell_renderer_text_new();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), cell, TRUE);
      gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), cell,
                                     "text", 1,
                                     NULL);
    }

  gtk_list_store_clear(store);
  
  /*ObtenciÃ³n del Id de 'nulidad de venta' */
  resId = EjecutarSQL ("SELECT id FROM tipo_egreso WHERE descrip='Nulidad de Venta'");
  nulVenId = atoi (PUT(PQvaluebycol(resId, 0, "id")));

  /*Poblamiento del combobox*/
  for (i = 0; i < tuples; i++)
    {
      if (atoi (PUT(PQvaluebycol(res, i, "id"))) != nulVenId) /*No queremos que el motivo "Nulidad de Venta" aparezca como opción*/ 
	{
	  gtk_list_store_append(store, &iter);
	  gtk_list_store_set(store, &iter,
			     0, atoi(PQvaluebycol(res, i, "id")),
			     1, PQvaluebycol(res, i, "descrip"),
			     -1);
	}
    }

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_out_amount"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), g_strdup_printf("%d", monto));
  gtk_widget_grab_focus(aux_widget);

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_caja_egreso"));
  gtk_widget_show_all(aux_widget);
}

/**
 * Es llamada por las funciones "IniciarLaCaja" y "open_caja"
 *
 * Esta funcion insertar los valores iniciales de la caja
 * (id_vendedor,fecha_inicio, inicio(monto))
 *
 * @return TRUE si se realizo correctamented, FALSE si fallo.
 *
 */

gboolean
InicializarCaja (gint monto)
{
  PGresult *res;
  gchar *q;

  q = g_strdup_printf ("INSERT INTO caja (id_vendedor, fecha_inicio, inicio) "
                       "VALUES(%d, NOW(), %d)", user_data->user_id, monto);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

/**
 * Es llamada por la funcion "CerrarCajaWin"
 *
 * Esta funcion retorna el valor de inicio de la caja
 *
 * @return monto de la consulta, -1 si fallo
 *
 */

gint
ArqueoCaja (void)
{
  PGresult *res;

  res = EjecutarSQL("select * from get_arqueo_caja (-1)");

  if ((res != NULL) && (PQntuples(res)>0))
    return atoi (PQgetvalue (res, 0, 0));
  else
    return -1;
}

/**
 * Es llamada por la funcion "CerrarLaCaja"
 *
 * Esta funcion actualiza la caja para insertar los valores de cierre de la caja
 * (fecha_termino, termino(monto))
 *
 * @param monto entero que contiene el dinero de cierre la caja
 * @return TRUE si se realizo correctamented, FALSE si fallo.
 *
 */

gboolean
CerrarCaja (gint monto)
{
  PGresult *res;
  gchar *q;

  if (monto == -1)
    monto = ArqueoCaja ();

  q = g_strdup_printf ("UPDATE caja SET fecha_termino=NOW(), termino=%d "
                       "WHERE id=(SELECT last_value FROM caja_id_seq)",
                       monto);
  res = EjecutarSQL (q);
  g_free (q);

  if (res != NULL || PQntuples (res) == 0)
    return TRUE;
  else
    return FALSE;
}

/**
 * Es llamada por la funcion "CerrarLaCaja"
 *
 * Esta funcion carga los valores a traves de consultas a la BD, de
 * cash_sell(dinero en la caja), el dinero de cierre de la caja, Calculando
 * la perdida y luego registrandola en la caja con update.
 *
 * @return TRUE si se realizo correctamented, FALSE si fallo.
 *
 */

gint
CalcularPerdida (void)
{
  PGresult *res;
  gint perdida, cash_sell, cierre_caja;

  cash_sell = atoi (GetDataByOne ("select * from get_arqueo_caja(-1)"));

  cierre_caja = atoi (GetDataByOne ("SELECT termino FROM caja WHERE id=(SELECT last_value FROM caja_id_seq)"));

  perdida = cash_sell - cierre_caja;

  res = EjecutarSQL (g_strdup_printf ("update caja set perdida=%d WHERE id="
                                      "(SELECT last_value FROM caja_id_seq)", perdida));

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

/**
 * Esta funcion es llamda por "prepare_caja"
 *
 * Retornamos TRUE si la caja fue cerrada anteriormente y debemos
 * inicilizarla otra vez de lo contrario FALSE lo cual significa que
 * la caja no se a cerrado
 *
 * @return TRUE when the caja was closed previously
 */
gboolean
check_caja (void)
{
  PGresult *res;

  res = EjecutarSQL ("select is_caja_abierta()");

  if (PQntuples (res) == 0)
    {
      g_printerr("%s: could not retrieve the result of the sql query\n",
                 G_STRFUNC);
      return FALSE;
    }

  if (g_str_equal(PQgetvalue(res, 0, 0), "t"))
    return FALSE;
  else
    return TRUE;
}

/**
 * Closes the dialog that close the caja
 *
 */
void
CloseCajaWin (void)
{
  GtkWidget *widget;

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_caja_close"));
  gtk_widget_hide (widget);
}

/**
 * Esta funcion es llamada por "open_caja 
 * Raise the initialization caja dialog
 *
 * @param proposed_amount the amount that must be entered in the entry
 */
void
InicializarCajaWin (gint proposed_amount)
{
  GtkWidget *widget;

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_init_amount"));
  gtk_entry_set_text(GTK_ENTRY(widget), g_strdup_printf("%d", proposed_amount));
  gtk_widget_grab_focus(widget);

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_caja_init"));
  gtk_widget_show_all (widget);
}

/**
 * Esta Funcion es llamda por "open_caja"
 *
 * Callback connected to accept button of the initialize caja dialog
 *
 * @param widget the widget that emited the signal
 * @param data the user data
 */
void
IniciarLaCaja (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  gint inicio;
  gint monto;

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_init_amount"));
  monto = atoi (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  inicio = caja_get_last_amount();

  CloseCajaWin ();

  InicializarCaja (inicio);

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_caja_init"));
  gtk_widget_hide (aux_widget);

  if (inicio < monto)
    VentanaIngreso (monto - inicio);
  else if (inicio > monto)
    VentanaEgreso (inicio - monto);
}


/**
 * Es llamada por la funcion "on_btn_cash_box_close_clicked"
 *
 * Raise the close caja dialog
 *
 */
void
CerrarCajaWin (void)
{
  GtkWidget *widget;
  gint amount_must_have;

  amount_must_have = ArqueoCaja();

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "lbl_caja_close_must_have"));
  gtk_label_set_text(GTK_LABEL(widget), g_strdup_printf("%d", amount_must_have));
  g_object_set_data(G_OBJECT(widget), "must-have", (gpointer)amount_must_have);

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "lbl_caja_close_lost"));
  gtk_label_set_text(GTK_LABEL(widget), "");

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_close_have"));
  gtk_entry_set_text(GTK_ENTRY(widget), g_strdup_printf("%d", amount_must_have));
  gtk_editable_select_region (GTK_EDITABLE(widget), 0, -1);
  gtk_widget_grab_focus(widget);

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_close_amount"));
  gtk_entry_set_text(GTK_ENTRY(widget), g_strdup_printf("%d", amount_must_have));

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_caja_close"));
  gtk_widget_show_all(widget);
}

/**
 * Callback associated to the accept button of close caja dialog
 *
 * @param widget the widget that emited the signal
 * @param data the user data
 */
void
CerrarLaCaja (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  gint monto_must_have;
  gint monto_real_que_tiene;
  gint monto_de_cierre;
  gboolean res;

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "lbl_caja_close_must_have"));
  monto_must_have = (gint)g_object_get_data(G_OBJECT(aux_widget), "must-have");

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_close_have"));
  monto_real_que_tiene = atoi(gtk_entry_get_text(GTK_ENTRY(aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_close_amount"));
  monto_de_cierre = atoi(gtk_entry_get_text(GTK_ENTRY(aux_widget)));

  if (monto_de_cierre < monto_real_que_tiene)
    {
      Egresar (monto_real_que_tiene - monto_de_cierre, 0, user_data->user_id);
      res = CerrarCaja (monto_de_cierre);
    }
  else
    {
    if (monto_de_cierre > monto_real_que_tiene)
      {
        Ingreso (monto_de_cierre - monto_real_que_tiene, 0, user_data->user_id);
        res = CerrarCaja (monto_de_cierre);
      }
    else
      {
        res = CerrarCaja(monto_de_cierre);
      }
    }

  CalcularPerdida();

  if (res)
    {
      CloseCajaWin ();
      aux_widget = GTK_WIDGET (gtk_builder_get_object (builder, "quit_message"));
      gtk_dialog_response (GTK_DIALOG(aux_widget), GTK_RESPONSE_YES);
    }
  else
    ErrorMSG (aux_widget, "No se pudo cerrar la caja apropiadamente\nPor favor intente nuevamente");
}

/**
 * prepares the software to open the caja based in the in the user
 * that is running the software.
 *
 */
void
prepare_caja (void)
{
  if (check_caja())
    open_caja (FALSE);
}

/**
 * Es llamada por la funcion "prepare_caja"
 *
 * initializes a new caja
 *
 * @param automatic_mode TRUE if does NOT must prompt a dialog
 * interaction with the user
 */
void
open_caja (gboolean automatic_mode)
{
  if (automatic_mode)
    InicializarCaja (caja_get_last_amount ());
  else
    InicializarCajaWin (caja_get_last_amount ());
}


/**
 * Es llamada por las funciones "open_caja" y "IniciarLaCaja"
 *
 * Retrieves the last amount of money that has the current caja
 * (opened or closed)
 *
 * @return the amount of money
 */
gint
caja_get_last_amount (void)
{
  PGresult *res;
  gchar *q;
  gint last_amount = 0;
  gint last_caja;

  res = EjecutarSQL("select max(id) from caja");
  last_caja = atoi (PQgetvalue (res, 0, 0));

  q = g_strdup_printf ("select termino from caja where id=%d", last_caja);
  res = EjecutarSQL (q);
  g_free (q);

  if (PQntuples (res) != 0)
    {
      last_amount = atoi (PQgetvalue (res, 0, 0));
    }

  return last_amount;
}

/**
 * Es llamada cuando se preiona enter en la caja de texto "entry_caja_close_have"
 * (signal actived)
 *
 * Uodates the entry of amount to close and the label of lost money in
 * the close 'caja' dialog.
 *
 * @param editable the entry that emits the signal
 * @param data the user data
 */
void
on_entry_caja_close_have_changed (GtkEditable *editable, gpointer data)
{
  GtkWidget *widget;
  gint monto;
  gint must_have;

  monto = atoi(gtk_entry_get_text(GTK_ENTRY(editable)));

  if (monto < 0)
    AlertMSG(GTK_WIDGET(editable), "no puede ingresar un monto de perdida menor que 0");
  else
    {
      widget = GTK_WIDGET (gtk_builder_get_object(builder, "lbl_caja_close_must_have"));
      must_have = (gint)g_object_get_data(G_OBJECT(widget), "must-have");

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "lbl_caja_close_lost"));
      gtk_label_set_text(GTK_LABEL(widget), g_strdup_printf("%d", must_have - monto));

      if (must_have > monto)
        gtk_label_set_markup (GTK_LABEL(widget), g_strdup_printf("<span color=\"red\">%d</span>", must_have - monto));
      else
        gtk_label_set_markup (GTK_LABEL(widget), "0");

      widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_close_amount"));
      gtk_entry_set_text (GTK_ENTRY(widget), g_strdup_printf("%d", monto));

    }
}

/**
 * Es llamada cuando el boton "btn_cash_box_close" es presionado
 * (signal clicked)
 *
 * Llama a la funcion "CerrarCajaWin"
 *
 * @param editable the entry that emits the signal
 * @param data the user data
 */
void
on_btn_cash_box_close_clicked (void)
{
  CerrarCajaWin ();
}
