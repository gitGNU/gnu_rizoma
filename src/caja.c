/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
/*caja.c
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

gint
ArqueoCajaLastDay (void)
{
  PGresult *res;
  gint monto;

  res = EjecutarSQL
    (g_strdup_printf
     ("SELECT (SELECT inicio FROM caja WHERE date_trunc ('day', fecha_inicio)=date_trunc('day', t1.fecha_inicio)) AS inicio, (SELECT SUM (monto) FROM venta WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio) AND tipo_venta=%d) AS ventas_efect, (SELECT SUM(t1.monto) FROM cheques AS t1 WHERE id_venta IN (SELECT id FROM ventas WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio))) AS ventas_doc, (SELECT SUM(monto_abonado) FROM abono WHERE date_trunc('day', fecha_abono)=date_trunc('day', fecha_inicio)) AS pago_credit, (SELECT SUM(monto) FROM ingreso WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio)) AS otros, (SELECT SUM (monto) FROM egreso WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio) AND tipo=1) AS retiros, (SELECT SUM (monto) FROM egresos WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio) AND tipo=3) AS gastos, (SELECT SUM (monto) FROM egresos WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio) AND tipo=2) AS otros_egresos, (SELECT SUM(monto) FROM factura_compra WHERE id IN (SELECT id_fact FROM pagos  WHERE caja='t' AND date_trunc('day', fecha)=date_trunc('day', fecha_inicio))) AS facturas, fecha_inicio FROM caja AS t1 WHERE t1.fecha_termino=to_timestamp('DD-MM-YY', '00-00-00')", CASH));

  if (res != NULL && PQntuples (res) != 0)
    monto = ((atoi (PQgetvalue (res, 0, 0)) + atoi (PQgetvalue (res, 0, 1)) +
              atoi (PQgetvalue (res, 0, 2)) + atoi (PQgetvalue (res, 0, 3)) +
              atoi (PQgetvalue (res, 0, 4))) -
             (atoi (PQgetvalue (res, 0, 5)) +
              atoi (PQgetvalue (res, 0, 6)) + atoi (PQgetvalue (res, 0, 7)) +
              atoi (PQgetvalue (res, 0, 8))));
  else
    monto = 0;

  return monto;
}

gint
ReturnSaldoCaja (void)
{
  PGresult *res;
  gint caja;

  res = EjecutarSQL
    (g_strdup_printf
     ("SELECT (SELECT inicio FROM caja WHERE date_trunc ('day', fecha_inicio)=date_trunc('day', localtimestamp)) AS inicio, (SELECT SUM (monto) FROM venta WHERE date_trunc('day', fecha)=date_trunc('day', localtimestamp) AND tipo_venta=%d) AS ventas_efect, (SELECT SUM(t1.monto) FROM cheques AS t1 WHERE id_venta IN (SELECT id FROM ventas WHERE date_trunc('day', fecha)=date_trunc('day', localtimestamp))) AS ventas_doc, (SELECT SUM (monto) FROM egreso WHERE date_trunc('day', fecha)=date_trunc('day', localtimestamp) AND tipo=1) AS retiros, (SELECT SUM(monto_abonado) FROM abono WHERE date_trunc('day', fecha_abono)=date_trunc('day', localtimestamp)) AS pago_credit, (SELECT SUM(monto) FROM ingreso WHERE date_trunc('day', fecha)=date_trunc('day', localtimestamp)) AS otros, (SELECT SUM (monto) FROM egresos WHERE date_trunc('day', fecha)=date_trunc('day', localtimestamp) AND tipo=3) AS gastos, (SELECT SUM (monto) FROM egresos WHERE date_trunc('day', fecha)=date_trunc('day', localtimestamp) AND tipo=2) AS otros_egresos, (SELECT SUM(monto) FROM factura_compra WHERE id IN (SELECT id_fact FROM pagos  WHERE caja='t' AND date_trunc('day', fecha)=date_trunc('day', localtimestamp))) AS facturas", CASH));

  if (res != NULL && PQntuples (res) != 0)
    caja = atoi
      ((g_strdup_printf ("%d",
                         (atoi (PQgetvalue (res, 0, 0)) + atoi (PQgetvalue (res, 0, 1)) +
                          atoi (PQgetvalue (res, 0, 2)) + atoi (PQgetvalue (res, 0, 4)) +
                          atoi (PQgetvalue (res, 0, 5))) -
                         (atoi (PQgetvalue (res, 0, 8)) +
                          atoi (PQgetvalue (res, 0, 3)) + atoi (PQgetvalue (res, 0, 6)) +
                          atoi (PQgetvalue (res, 0, 7))))));
  else
    caja = 0;

  return caja;
}

void
CloseVentanaIngreso(void)
{
  GtkWidget *wid;

  wid = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_in_amount"));
  gtk_entry_set_text(GTK_ENTRY(wid), "");

  wid = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_caja_ingreso"));
  gtk_widget_hide(wid);
}

void
IngresarDinero (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint monto;
  gint motivo;

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_caja_in_motiv"));
  model = gtk_combo_box_get_model(GTK_COMBO_BOX(aux_widget));
  if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(aux_widget), &iter))
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

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_caja_out_motiv"));
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

void
EgresarDinero (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  gint active;
  gint monto;
  gint motivo;

  GtkTreeModel *model;
  GtkTreeIter iter;

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_out_amount"));
  monto = atoi (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "cmb_caja_out_motiv"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (aux_widget));

  if (monto == 0)
    ErrorMSG (aux_widget, "No pueden haber egresos de $0");
  else if (active == -1)
    ErrorMSG (aux_widget, "Debe Seleccionar un tipo de egreso");
  else if (monto > ReturnSaldoCaja ())
    ErrorMSG (aux_widget, "No se puede retirar mas dinero del que ahi en caja");
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

void
CloseVentanaEgreso (void)
{
  GtkWidget *wid;

  wid = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_out_amount"));
  gtk_entry_set_text(GTK_ENTRY(wid), "");

  wid = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_caja_egreso"));
  gtk_widget_hide(wid);
}


void
VentanaEgreso (gint monto)
{
  GtkWidget *combo;
  GtkWidget *aux_widget;
  GtkListStore *store;
  GtkTreeIter iter;
  PGresult *res;
  gint tuples, i;

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

  for (i = 0; i < tuples; i++)
    {
      gtk_list_store_append(store, &iter);
      gtk_list_store_set(store, &iter,
			 0, atoi(PQvaluebycol(res, i, "id")),
			 1, PQvaluebycol(res, i, "descrip"),
			 -1);

    }

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_out_amount"));
  gtk_entry_set_text(GTK_ENTRY(aux_widget), g_strdup_printf("%d", monto));
  gtk_widget_grab_focus(aux_widget);

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_caja_egreso"));
  gtk_widget_show_all(aux_widget);
}

void
CleanCajaData (void)
{
  gtk_label_set_text (GTK_LABEL (inicio_caja), "");
  gtk_label_set_text (GTK_LABEL (ventas_efect), "");
  gtk_label_set_text (GTK_LABEL (ventas_doc), "");
  gtk_label_set_text (GTK_LABEL (pago_ventas), "");
  gtk_label_set_text (GTK_LABEL (otros_ingresos), "");
  gtk_label_set_text (GTK_LABEL (total_haberes), "");

  gtk_label_set_text (GTK_LABEL (pagos), "");
  gtk_label_set_text (GTK_LABEL (retiros), "");
  gtk_label_set_text (GTK_LABEL (gastos_corrientes), "");
  gtk_label_set_text (GTK_LABEL (otros_egresos), "");
  gtk_label_set_text (GTK_LABEL (total_debitos), "");

  gtk_label_set_text (GTK_LABEL (total_caja), "");

}

void
FillCajaData (void)
{
  PGresult *res;

  CleanCajaData ();

  res = EjecutarSQL (g_strdup_printf
                     ("SELECT (SELECT inicio FROM caja WHERE date_part('year', fecha_inicio)=%d AND date_part('month', fecha_inicio)=%d AND date_part('day', fecha_inicio)=%d) AS inicio, (SELECT SUM (monto) FROM venta WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d AND tipo_venta=%d) AS ventas_efect, (SELECT SUM(t1.monto) FROM cheques AS t1 WHERE id_venta IN (SELECT id FROM ventas WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d AND date_part('year', fecha)=date_part('year', t1.fecha) AND date_part('month', fecha)=date_part('month', t1.fecha) AND date_part('day', fecha)=date_part('day', t1.fecha))) AS ventas_doc, (SELECT SUM (monto) FROM egreso WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d AND tipo=1) AS retiros, (SELECT SUM(monto_abonado) FROM abono WHERE date_part('year', fecha_abono)=%d AND date_part('month', fecha_abono)=%d AND date_part('day', fecha_abono)=%d) AS pago_credit, (SELECT SUM(monto) FROM ingreso WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d) AS otros, (SELECT SUM (monto) FROM egresos WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d AND tipo=3) AS gastos, (SELECT SUM (monto) FROM egresos WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d AND tipo=2) AS otros_egresos, (SELECT SUM(monto) FROM factura_compra WHERE id IN (SELECT id_fact FROM pagos  WHERE caja='t' AND date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d)) AS facturas",
                      year, month+1, day, year, month+1, day, CASH, year, month+1, day, year, month+1,
                      day, year, month+1, day, year, month+1, day, year, month+1, day, year,
                      month+1, day, year, month+1, day));

  if (res != NULL && PQntuples (res) != 0 && strcmp (PQgetvalue (res, 0, 0), "") != 0)
    {
      if (strcmp (PQgetvalue (res, 0, 0), "") != 0)
        gtk_label_set_markup (GTK_LABEL (inicio_caja),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 0))));

      if (strcmp (PQgetvalue (res, 0, 1), "") != 0)
        gtk_label_set_markup (GTK_LABEL (ventas_efect),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 1))));

      if (strcmp (PQgetvalue (res, 0, 2), "") != 0)
        gtk_label_set_markup (GTK_LABEL (ventas_doc),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 2))));

      if (strcmp (PQgetvalue (res, 0, 4), "") != 0)
        gtk_label_set_markup (GTK_LABEL (pago_ventas),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 4))));

      if (strcmp (PQgetvalue (res, 0, 5), "") != 0)
        gtk_label_set_markup (GTK_LABEL (otros_ingresos),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 5))));

      gtk_label_set_markup (GTK_LABEL (total_haberes),
                            g_strdup_printf
                            ("<b>$\t%s</b>", PutPoints
                             (g_strdup_printf
                              ("%d", atoi (PQgetvalue (res, 0, 0)) +
                               atoi (PQgetvalue (res, 0, 1)) + atoi (PQgetvalue (res, 0, 2)) +
                               atoi (PQgetvalue (res, 0, 4)) + atoi (PQgetvalue (res, 0, 5))))));

      if (strcmp (PQgetvalue (res, 0, 8), "") != 0)
        gtk_label_set_markup (GTK_LABEL (pagos),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 8))));

      if (strcmp (PQgetvalue (res, 0, 3), "") != 0)
        gtk_label_set_markup (GTK_LABEL (retiros),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 3))));

      if (strcmp (PQgetvalue (res, 0, 6), "") != 0)
        gtk_label_set_markup (GTK_LABEL (gastos_corrientes),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 6))));

      if (strcmp (PQgetvalue (res, 0, 7), "") != 0)
        gtk_label_set_markup (GTK_LABEL (otros_egresos),
                              g_strdup_printf ("<b>%s</b>", PutPoints (PQgetvalue (res, 0, 7))));

      gtk_label_set_markup (GTK_LABEL (total_debitos),
                            g_strdup_printf
                            ("<b>$\t%s</b>", PutPoints
                             (g_strdup_printf
                              ("%d", atoi (PQgetvalue (res, 0, 8)) +
                               atoi (PQgetvalue (res, 0, 3)) + atoi (PQgetvalue (res, 0, 6)) +
                               atoi (PQgetvalue (res, 0, 7))))));

      gtk_label_set_markup
        (GTK_LABEL (total_caja),
         g_strdup_printf
         ("<span size=\"xx-large\"><b>$\t%s</b></span>",
          PutPoints
          (g_strdup_printf
           ("%d",
            (atoi (PQgetvalue (res, 0, 0)) + atoi (PQgetvalue (res, 0, 1)) +
             atoi (PQgetvalue (res, 0, 2)) + atoi (PQgetvalue (res, 0, 4)) +
             atoi (PQgetvalue (res, 0, 5))) -
            (atoi (PQgetvalue (res, 0, 8)) +
             atoi (PQgetvalue (res, 0, 3)) + atoi (PQgetvalue (res, 0, 6)) +
             atoi (PQgetvalue (res, 0, 7)))))));


    }
}

void
SelectedCajaDate (GtkCalendar *calendar, gpointer data)
{
  GtkButton *button = (GtkButton *) data;
  GtkCalendar *pepe = calendar;

  if (calendar == NULL)
    calendar = GTK_CALENDAR (gtk_calendar_new ());
  else
    SetToggleMode (GTK_TOGGLE_BUTTON (data), NULL);

  gtk_calendar_get_date (calendar, &year, &month, &day);

  gtk_button_set_label (button, g_strdup_printf ("%.2u/%.2u/%.4u", day, month+1, year));

  if (pepe != NULL)
    FillCajaData ();

}

void
DisplayCajaDate (GtkToggleButton *widget, gpointer data)
{
  GtkWidget *vbox;
  GtkCalendar *calendar;
  GtkRequisition req;
  gint h, w;
  gint x, y;
  gint button_y, button_x;
  gboolean toggle = gtk_toggle_button_get_active (widget);

  if (toggle == TRUE)
    {
      gdk_window_get_origin (GTK_WIDGET (widget)->window, &x, &y);

      gtk_widget_size_request (GTK_WIDGET (widget), &req);
      h = req.height;
      w = req.width;

      button_y = GTK_WIDGET (widget)->allocation.y;
      button_x = GTK_WIDGET (widget)->allocation.x;

      calendar_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_screen (GTK_WINDOW (calendar_win), gtk_widget_get_screen (GTK_WIDGET (widget)));

      gtk_container_set_border_width (GTK_CONTAINER (calendar_win), 5);
      gtk_window_set_type_hint (GTK_WINDOW (calendar_win), GDK_WINDOW_TYPE_HINT_DOCK);
      gtk_window_set_decorated (GTK_WINDOW (calendar_win), FALSE);
      gtk_window_set_resizable (GTK_WINDOW (calendar_win), FALSE);
      gtk_window_stick (GTK_WINDOW (calendar_win));
      gtk_window_set_title (GTK_WINDOW (calendar_win), "Calendario");

      vbox = gtk_vbox_new (FALSE, 3);
      gtk_container_add (GTK_CONTAINER (calendar_win), vbox);
      gtk_widget_show (vbox);

      calendar = GTK_CALENDAR (gtk_calendar_new ());
      gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (calendar), FALSE, FALSE, 0);
      gtk_widget_show (GTK_WIDGET (calendar));

      g_signal_connect (G_OBJECT (calendar), "day-selected-double-click",
                        G_CALLBACK (SelectedCajaDate), (gpointer) widget);

      if (year != 0 && month != 0 && day != 0)
        {
          gtk_calendar_select_day (calendar, day);
          gtk_calendar_select_month (calendar, month, year);
        }

      gtk_widget_show (calendar_win);

      x = (x + button_x);
      y = (y + button_y) + h;

      gtk_window_move (GTK_WINDOW (calendar_win), x, y);
      gtk_window_present (GTK_WINDOW (calendar_win));
    }
  else if (toggle == FALSE)
    gtk_widget_destroy (calendar_win);
}

void
CajaTab (GtkWidget *main_box)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;

  GtkWidget *table;

  vbox = gtk_vbox_new (FALSE, 10);
  gtk_box_pack_start (GTK_BOX (main_box), vbox, FALSE, FALSE, 10);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Preparado Al: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  caja_button = gtk_toggle_button_new_with_label ("\t\t");
  gtk_box_pack_start (GTK_BOX (hbox), caja_button, FALSE, FALSE, 3);
  gtk_widget_show (caja_button);

  SelectedCajaDate (NULL, (gpointer) caja_button);

  g_signal_connect (G_OBJECT (caja_button), "toggled",
                    G_CALLBACK (DisplayCajaDate), NULL);

  table = gtk_table_new (8, 4, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 5);
  gtk_widget_show (table);

  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 30);
  gtk_table_set_col_spacing (GTK_TABLE (table), 3, 10);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<span size=\"x-large\"><b>INGRESOS</b></span>");
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1,
                             0, 1);
  gtk_widget_show (label);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<b>Inicio en Caja</b>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1,
                             1, 2);
  gtk_widget_show (label);

  inicio_caja = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (inicio_caja), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), inicio_caja,
                             1, 2,
                             1, 2);
  gtk_widget_show (inicio_caja);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<b>Ventas en Efectivo</b>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1,
                             2, 3);
  gtk_widget_show (label);

  ventas_efect = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (ventas_efect), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), ventas_efect,
                             1, 2,
                             2, 3);
  gtk_widget_show (ventas_efect);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<b>Ventas con Documentos</b>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1,
                             3, 4);
  gtk_widget_show (label);

  ventas_doc = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (ventas_doc), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), ventas_doc,
                             1, 2,
                             3, 4);
  gtk_widget_show (ventas_doc);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<b>Pago de Ventas a Credito</b>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1,
                             4, 5);
  gtk_widget_show (label);

  pago_ventas = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (pago_ventas), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), pago_ventas,
                             1, 2,
                             4, 5);
  gtk_widget_show (pago_ventas);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<b>Otros Ingresos</b>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1,
                             5, 6);
  gtk_widget_show (label);

  otros_ingresos = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (otros_ingresos), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), otros_ingresos,
                             1, 2,
                             5, 6);
  gtk_widget_show (otros_ingresos);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<span size=\"x-large\"><b>Total Ingresos</b></span>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             0, 1,
                             8, 9);
  gtk_widget_show (label);

  total_haberes = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (total_haberes), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), total_haberes,
                             1, 2,
                             8, 9);
  gtk_widget_show (total_haberes);


  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<span size=\"x-large\"><b>EGRESOS</b></span>");
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             3, 4,
                             0, 1);
  gtk_widget_show (label);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<b>Pago Facturas</b>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             3, 4,
                             1, 2);
  gtk_widget_show (label);

  pagos = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (pagos), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), pagos,
                             4, 5,
                             1, 2);
  gtk_widget_show (pagos);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<b>Retiros</b>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             3, 4,
                             2, 3);
  gtk_widget_show (label);

  retiros = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (retiros), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), retiros,
                             4, 5,
                             2, 3);
  gtk_widget_show (retiros);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<b>Gastos Corrientes</b>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             3, 4,
                             3, 4);
  gtk_widget_show (label);

  gastos_corrientes = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (gastos_corrientes), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), gastos_corrientes,
                             4, 5,
                             3, 4);
  gtk_widget_show (gastos_corrientes);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<b>Otros Egresos</b>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             3, 4,
                             4, 5);
  gtk_widget_show (label);

  otros_egresos = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (otros_egresos), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), otros_egresos,
                             4, 5,
                             4, 5);
  gtk_widget_show (otros_egresos);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label), "<span size=\"x-large\"><b>Total Egresos</b></span>");
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label,
                             3, 4,
                             8, 9);
  gtk_widget_show (label);

  total_debitos = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (total_debitos), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), total_debitos,
                             4, 5,
                             8, 9);
  gtk_widget_show (total_debitos);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
                        "<span size=\"xx-large\"><b>Total en Caja:\t</b></span>");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  total_caja = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), total_caja, FALSE, FALSE, 3);
  gtk_widget_show (total_caja);

}

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

gint
ArqueoCaja (void)
{
  PGresult *res;

  res = EjecutarSQL (g_strdup_printf ("SELECT date_part('day', fecha_inicio) AS dia, date_part('month', fecha_inicio) AS mes, date_part('year', fecha_inicio) AS ano FROM caja WHERE id=(SELECT last_value FROM caja_id_seq)"));

  if ((PQntuples (res)) == 0)
    return FALSE;

  res = EjecutarSQL (g_strdup_printf ("SELECT ((SELECT inicio FROM caja WHERE date_part('year', fecha_inicio)=%s AND date_part('month', fecha_inicio)=%s AND date_part('day', fecha_inicio)=%s) + (SELECT SUM (monto) FROM venta WHERE date_part('year', fecha)=%s AND date_part('month', fecha)=%s AND date_part('day', fecha)=%s AND tipo_venta=%d) + (SELECT SUM(t1.monto) FROM cheques AS t1 WHERE id_venta IN (SELECT id FROM ventas WHERE date_part('year', fecha)=%s AND date_part('month', fecha)=%s AND date_part('day', fecha)=%s AND date_part('year', fecha)=date_part('year', t1.fecha) AND date_part('month', fecha)=date_part('month', t1.fecha) AND date_part('day', fecha)=date_part('day', t1.fecha))) + (SELECT SUM(monto_abonado) FROM abono WHERE date_part('year', fecha_abono)=%s AND date_part('month', fecha_abono)=%s AND date_part('day', fecha_abono)=%s) + (SELECT SUM(monto) FROM ingreso WHERE date_part('year', fecha)=%s AND date_part('month', fecha)=%s AND date_part('day', fecha)=%s)) - ((SELECT SUM (monto) FROM egreso WHERE date_part('year', fecha)=%s AND date_part('month', fecha)=%s AND date_part('day', fecha)=%s AND tipo=1) + (SELECT SUM (monto) FROM egresos WHERE date_part('year', fecha)=%s AND date_part('month', fecha)=%s AND date_part('day', fecha)=%s AND tipo=3) + (SELECT SUM (monto) FROM egresos WHERE date_part('year', fecha)=%s AND date_part('month', fecha)=%s AND date_part('day', fecha)=%s AND tipo=2) + (SELECT monto FROM factura_compra WHERE id IN (SELECT id_fact FROM pagos  WHERE caja='t')))", PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0), PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0), CASH, PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0), PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0), PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0), PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0), PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0), PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0)));

  if ((PQntuples (res)) == 0)
    return FALSE;

  return atoi (PQgetvalue (res, 0, 0));
}

gboolean
CerrarCaja (gint monto)
{
  PGresult *res;

  if (monto == -1)
    monto = ArqueoCaja ();

  res = EjecutarSQL
    (g_strdup_printf ("UPDATE caja SET fecha_termino=NOW(), termino=%d WHERE id=(SELECT last_value"
                      " FROM caja_id_seq)", monto));

  if (res != NULL || PQntuples (res) == 0)
    return TRUE;
  else
    return FALSE;
}

gint
CalcularPerdida (void)
{
  PGresult *res;
  gint perdida, cash_sell, cierre_caja;
  gchar *q;

  q = g_strdup_printf ("SELECT SUM (monto) as total_sell FROM venta WHERE "
		       "date_trunc('day', fecha) = CURRENT_DATE"
		       "AND tipo_venta=%d", CASH);
  cash_sell = atoi (GetDataByOne (q));
  g_free(q);

  cierre_caja = atoi (GetDataByOne ("SELECT termino FROM caja WHERE id=(SELECT last_value FROM caja_id_seq)"));

  perdida = cash_sell - cierre_caja;

  res = EjecutarSQL (g_strdup_printf ("INSERT INTO caja (perdida) VALUES(%d) WHERE id="
                                      "(SELECT last_value FROM caja_id_seq)", perdida));

  if (res != NULL)
    return TRUE;
  else
    return FALSE;
}

/**
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

void
CloseCajaWin (void)
{
  if (caja->win != NULL)
    gtk_widget_destroy (caja->win);

  caja->win = NULL;

  //gtk_window_set_focus (GTK_WINDOW (main_window), GTK_WIDGET (gtk_builder_get_object (builder, "barcode_entry")));

}

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

  if (inicio < monto)
    VentanaIngreso (monto - inicio);
  else if (inicio > monto)
    VentanaEgreso (inicio - monto);
}

void
CerrarCajaWin (gboolean last_user)
{
  GtkWidget *widget;
  gint amount_must_have;

  amount_must_have = ArqueoCaja();

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "lbl_caja_close_must_have"));
  gtk_label_set_text(GTK_LABEL(widget), g_strdup_printf("$ %d", amount_must_have));

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_caja_close_amount"));
  gtk_entry_set_text(GTK_ENTRY(widget), g_strdup_printf("%d", amount_must_have));
  gtk_widget_grab_focus(widget);

  widget = GTK_WIDGET (gtk_builder_get_object(builder, "wnd_caja_close"));
  gtk_widget_show_all(widget);
}

void
CerrarLaCaja (GtkWidget *widget, gpointer data)
{
  GtkLabel *label = (GtkLabel *)data;
  gint monto;

  monto = atoi (CutPoints (g_strdup (gtk_label_get_text (label))));

  if (CerrarCaja (monto) == FALSE)
    {
      CloseCajaWin ();
      //ErrorMSG (main_window, "Error al intentar Cerrar la caja");
    }
  else
    CloseCajaWin ();
}

/**
 * 
 *
 */
void
prepare_caja (void)
{
  gboolean closed_caja;

  closed_caja = check_caja();

  if (user_data->user_id == 0)
    {
      if (closed_caja)
	  open_caja (FALSE);
    }
  else
    {
      if (closed_caja)
	open_caja (TRUE);
    }
}

/**
 * initializes a new caja
 *
 * @param automatic_mode TRUE if does NOT must prompt a dialog
 * interaction with the user
 */
void
open_caja (gboolean automatic_mode)
{
  gint last_amount;
  gint last_caja;

  last_caja = caja_get_last_amount();

  if (automatic_mode)
    InicializarCaja(last_amount);
  else
      InicializarCajaWin (last_amount);
}


gint
caja_get_last_amount (void)
{
  PGresult *res;
  gchar *q;
  gint last_amount;
  gint last_caja;

  res = EjecutarSQL("select max(id) from caja");
  last_caja = atoi(PQgetvalue(res, 0, 0));

  q = g_strdup_printf("select (termino - perdida) from caja where id=%d",
		      last_caja);
  res = EjecutarSQL(q);
  g_free(q);

  last_amount = atoi(PQgetvalue(res, 0, 0));

  return last_amount;
}
