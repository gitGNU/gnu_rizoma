/* -*- Mode: C; tab-width: 4; indent-tabs-mode: f; c-basic-offset: 4 -*- */
/*proveedores.c
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

#include"tipos.h"

#include"postgres-functions.h"
#include"printing.h"
#include"dimentions.h"
#include"compras.h"
#include"credito.h"
#include"errors.h"

GtkWidget *rut;
GtkWidget *razon;
GtkWidget *direccion;
GtkWidget *comuna;
GtkWidget *ciudad;
GtkWidget *fono;
GtkWidget *fax;
GtkWidget *web;
GtkWidget *contacto;
GtkWidget *email;
GtkWidget *fono_directo;
GtkWidget *giro;

GtkWidget *compras_totales;
GtkWidget *contrib_total;
GtkWidget *contrib_proyect;
GtkWidget *inci_compras;
GtkWidget *stock_valorizado;
GtkWidget *merma_uni;
GtkWidget *merma_porc;
GtkWidget *ventas_totales;
GtkWidget *contrib_agreg;
GtkWidget *inci_ventas;
GtkWidget *total_pen_fact;
GtkWidget *indice_t;


GtkWidget *search_entry;

GtkTreeStore *proveedores_store;
GtkWidget *proveedores_tree;

void
BuscarProveedor (GtkWidget *widget, gpointer data)
{
  GtkTreeIter iter;
  PGresult *res;
  gint tuples, i;
  gchar *str_axu;
  gchar *q;
  gchar *string = g_strdup (gtk_entry_get_text (GTK_ENTRY (search_entry)));

  q = g_strdup_printf ("SELECT rut, dv, nombre, giro, contacto "
		       "FROM buscar_proveedor('%%%s%%')",
		       string);
  res = EjecutarSQL (q);
  g_free (q);

  tuples = PQntuples (res);

  gtk_tree_store_clear (proveedores_store);

  for (i = 0; i < tuples; i++)
    {
      str_axu = g_strconcat(PQvaluebycol (res, i, "rut"),"-",
			    PQvaluebycol (res, i, "dv"), NULL);
      gtk_tree_store_append (GTK_TREE_STORE (proveedores_store), &iter, NULL);
      gtk_tree_store_set (GTK_TREE_STORE (proveedores_store), &iter,
			  0, str_axu,
			  1, PQvaluebycol (res, i, "nombre"),
			  2, PQvaluebycol (res, i, "giro"),
			  3, PQvaluebycol (res, i, "contacto"),
			  -1);
      g_free (str_axu);
    }
}

void
ListarProveedores (void)
{
  PGresult *res;
  gint tuples, i;
  GtkTreeIter iter;
  gchar *str_axu;

  res = EjecutarSQL ("SELECT rut, dv, nombre, giro, contacto FROM buscar_proveedor('%') ORDER BY nombre ASC");

  tuples = PQntuples (res);

  gtk_tree_store_clear (GTK_TREE_STORE (proveedores_store));

  for (i = 0; i < tuples; i++)
    {
      str_axu = g_strconcat(PQvaluebycol(res, i, "rut"), "-", PQvaluebycol(res, i, "dv"), NULL);
      gtk_tree_store_append (GTK_TREE_STORE (proveedores_store), &iter, NULL);
      gtk_tree_store_set (GTK_TREE_STORE (proveedores_store), &iter,
			  0, str_axu,
			  1, PQvaluebycol (res, i, "nombre"),
			  2, PQvaluebycol (res, i, "giro"),
			  3, PQvaluebycol (res, i, "contacto"),
			  -1);
      g_free (str_axu);
    }
}

void
LlenarDatosProveedor (GtkTreeSelection *selection, gpointer data)
{
  PGresult *res;
  GtkTreeIter iter;
  gchar *rut_proveedor;
  gchar **aux, *q;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == FALSE)
    return;

  gtk_tree_model_get (GTK_TREE_MODEL (proveedores_store), &iter,
		      0, &rut_proveedor,
		      -1);

  aux = g_strsplit(rut_proveedor, "-", 0);
  q = g_strdup_printf ("SELECT * FROM select_proveedor(%s)", aux[0]);
  res = EjecutarSQL (q);
  g_free (q);
  g_strfreev(aux);

  if (res == NULL || PQntuples (res) == 0)
    return;

  gtk_entry_set_text (GTK_ENTRY (razon), PQvaluebycol (res, 0, "nombre"));

  q = g_strconcat(PQvaluebycol (res, 0, "rut"), "-", PQvaluebycol(res, 0, "dv"), NULL);
  gtk_label_set_text (GTK_LABEL (rut), q);
  g_free (q);

  gtk_entry_set_text (GTK_ENTRY (direccion), PQvaluebycol (res, 0, "direccion"));

  gtk_entry_set_text (GTK_ENTRY (ciudad), PQvaluebycol (res, 0, "ciudad"));

  gtk_entry_set_text (GTK_ENTRY (comuna), PQvaluebycol (res, 0, "comuna"));

  gtk_entry_set_text (GTK_ENTRY (fono), PQvaluebycol (res, 0, "telefono"));

  gtk_entry_set_text (GTK_ENTRY (web), PQvaluebycol (res, 0, "web"));

  gtk_entry_set_text (GTK_ENTRY (contacto), PQvaluebycol (res, 0, "contacto"));

  gtk_entry_set_text (GTK_ENTRY (email), PQvaluebycol (res, 0, "email"));

  gtk_entry_set_text (GTK_ENTRY (giro), PQvaluebycol (res, 0, "giro"));
}

void
CloseAgregarProveedorWindow (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *window = (GtkWidget *) user_data;

  gtk_widget_destroy (window);
}

void
AgregarProveedor (GtkWidget *widget, gpointer user_data)
{
  gchar *rut_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->rut_add)));
  gchar *rut_ver = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->rut_ver)));
  gchar *nombre_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->nombre_add)));
  gchar *direccion_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->direccion_add)));
  gchar *ciudad_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->ciudad_add)));
  gchar *comuna_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->comuna_add)));
  gchar *telefono_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->telefono_add)));
  gchar *email_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->email_add)));
  gchar *web_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->web_add)));
  gchar *contacto_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->contacto_add)));
  gchar *giro_c = g_strdup (gtk_entry_get_text (GTK_ENTRY (compra->giro_add)));


if (g_strcmp0 (rut_c, "") == 0)
    {
      ErrorMSG (compra->rut_add, "Debe Escribir el rut completo");
      return;
    }
  else if ((GetDataByOne
	    (g_strdup_printf ("SELECT * FROM proveedor WHERE rut='%s-%s'", rut_c, rut_ver))) != NULL)
    {
      ErrorMSG (compra->rut_add, "Ya existe un proveedor con el mismo rut");
      return;
    }
  else if (g_strcmp0 (rut_ver, "") == 0)
    {
      ErrorMSG (compra->rut_ver, "Debe ingresar el digito verificador del rut");
      return;
    }
  else if (g_strcmp0 (nombre_c, "") == 0)
    {
      ErrorMSG (compra->nombre_add, "Debe escribir el nombre del proveedor");
      return;
    }
  else if (g_strcmp0 (direccion_c, "") == 0)
    {
      ErrorMSG (compra->direccion_add, "Debe escribir la direccion");
      return;
    }
  else if (g_strcmp0 (comuna_c, "") == 0)
    {
      ErrorMSG (compra->comuna_add, "Debe escribir la comuna");
      return;
    }
  else if (g_strcmp0 (telefono_c, "") == 0)
    {
      ErrorMSG (compra->telefono_add, "Debe escribir el telefono");
      return;
    }
  else if (g_strcmp0 (giro_c, "") == 0)
    {
      ErrorMSG (compra->giro_add, "Debe escribir el giro");
      return;
    }

  if (VerificarRut (rut_c, rut_ver) != TRUE)
    {
      ErrorMSG (compra->rut_ver, "El rut no es valido!");
      return;
    }

  CloseAgregarProveedorWindow (NULL, user_data);

  AddProveedorToDB (g_strdup_printf ("%s-%s", rut_c, rut_ver), nombre_c, direccion_c, ciudad_c, comuna_c, telefono_c, email_c, web_c, contacto_c, giro_c);

  ListarProveedores ();

}

void
AgregarProveedorWindow (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *window;

  GtkWidget *hbox;
  GtkWidget *vbox;

  GtkWidget *button;
  GtkWidget *label;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_widget_show (window);
  gtk_window_present (GTK_WINDOW (window));

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  /*
    Cajas
  */

  /*
    Rut Proveedor
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Rut: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->rut_ver = gtk_entry_new_with_max_length (1);
  gtk_widget_set_size_request (compra->rut_ver, 20, -1);
  gtk_widget_show (compra->rut_ver);
  gtk_box_pack_end (GTK_BOX (hbox), compra->rut_ver, FALSE, FALSE, 3);

  label = gtk_label_new ("-");
  gtk_widget_show (label);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->rut_add = gtk_entry_new_with_max_length (10);
  gtk_widget_set_size_request (compra->rut_add, 75, -1);
  gtk_widget_show (compra->rut_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->rut_add, FALSE, FALSE, 3);

  gtk_window_set_focus (GTK_WINDOW (window), compra->rut_add);

  /*
    Nombre
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("*Nombre: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->nombre_add = gtk_entry_new_with_max_length (200);
  gtk_widget_show (compra->nombre_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->nombre_add, FALSE, FALSE, 3);

  /*
    Direccion
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("*Direccion: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->direccion_add = gtk_entry_new_with_max_length (200);
  gtk_widget_show (compra->direccion_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->direccion_add, FALSE, FALSE, 3);

  /*
    Ciudad
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("*Ciudad: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->ciudad_add = gtk_entry_new_with_max_length (100);
  gtk_widget_show (compra->ciudad_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->ciudad_add, FALSE, FALSE, 3);


  /*
    Comuna
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("*Comuna: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->comuna_add = gtk_entry_new_with_max_length (20);
  gtk_widget_show (compra->comuna_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->comuna_add, FALSE, FALSE, 3);

  /*
    Telefono
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("*Telefono: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->telefono_add = gtk_entry_new_with_max_length (20);
  gtk_widget_show (compra->telefono_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->telefono_add, FALSE, FALSE, 3);

  /*
    E-Mail
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("E-Mail: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->email_add = gtk_entry_new_with_max_length (300);
  gtk_widget_show (compra->email_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->email_add, FALSE, FALSE, 3);

  /*
    Web
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Web: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->web_add = gtk_entry_new_with_max_length (300);
  gtk_widget_show (compra->web_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->web_add, FALSE, FALSE, 3);

  /*
    Contacto
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("Contacto: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->contacto_add = gtk_entry_new_with_max_length (200);
  gtk_widget_show (compra->contacto_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->contacto_add, FALSE, FALSE, 3);

  /*
    Giro
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  label = gtk_label_new ("*Giro: ");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);

  compra->giro_add = gtk_entry_new_with_max_length (200);
  gtk_widget_show (compra->giro_add);
  gtk_box_pack_end (GTK_BOX (hbox), compra->giro_add, FALSE, FALSE, 3);

  /*
    Fin Cajas
  */

  /*
    Mensaje Inferior
  */
  label = gtk_label_new ("Los datos con * son obligatorios");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);

  /*
    Fin
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

  button = gtk_button_new_from_stock (GTK_STOCK_ADD);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (AgregarProveedor), (gpointer)window);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (CloseAgregarProveedorWindow), (gpointer)window);

}

void
ModificarProveedor (void)
{
  gchar *rut_c = (gchar *) gtk_label_get_text (GTK_LABEL (rut));
  gchar *razon_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (razon));
  gchar *direccion_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (direccion));
  gchar *comuna_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (comuna));
  gchar *ciudad_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (ciudad));
  gchar *fono_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (fono));
  gchar *web_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (web));
  gchar *contacto_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (contacto));
  gchar *email_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (email));
  gchar *giro_c = (gchar *) gtk_entry_get_text (GTK_ENTRY (giro));

  SetModificacionesProveedor (rut_c, razon_c, direccion_c, comuna_c, ciudad_c, fono_c,
                              web_c, contacto_c, email_c, giro_c);
}

void
proveedores_box (GtkWidget *main_box)
{
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;
  GtkWidget *box;
  GtkWidget *frame;

  GtkWidget *button;

  GtkWidget *label;

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  GtkWidget *scroll;

  Print *proveedores_print = (Print *) g_malloc0 (sizeof (Print));

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (main_box), vbox, FALSE, FALSE, 3);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  search_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), search_entry, FALSE, FALSE, 3);
  gtk_widget_show (search_entry);

  g_signal_connect (G_OBJECT (search_entry), "activate",
                    G_CALLBACK (BuscarProveedor), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_FIND);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (BuscarProveedor), NULL);

  button = gtk_button_new_from_stock (GTK_STOCK_ADD);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (AgregarProveedorWindow), NULL);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scroll);
  gtk_widget_set_size_request (scroll, MODULE_BOX_WIDTH - 10, 200);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scroll, FALSE, FALSE, 3);

  proveedores_store = gtk_tree_store_new (4,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING,
                                          G_TYPE_STRING);

  proveedores_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (proveedores_store));
  gtk_container_add (GTK_CONTAINER (scroll), proveedores_tree);
  gtk_widget_show (proveedores_tree);

  g_signal_connect (G_OBJECT (gtk_tree_view_get_selection
                              (GTK_TREE_VIEW (proveedores_tree))), "changed",
                    G_CALLBACK (LlenarDatosProveedor), NULL);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Proveedor", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (proveedores_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Rut Proveedor", renderer,
                                                     "text", 1,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (proveedores_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Giro", renderer,
                                                     "text", 2,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (proveedores_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Contacto", renderer,
                                                     "text", 3,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (proveedores_tree), column);
  gtk_tree_view_column_set_resizable (column, FALSE);

  frame = gtk_frame_new ("Proveedores");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 3);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 6);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  label = gtk_label_new ("Rut");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  rut = gtk_label_new ("\t\t\t");
  gtk_box_pack_start (GTK_BOX (box), rut, FALSE, FALSE, 0);
  gtk_widget_show (rut);

  box = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  label = gtk_label_new ("Razon Social");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  razon = gtk_entry_new_with_max_length (35);
  gtk_widget_show (razon);
  gtk_widget_set_size_request (GTK_WIDGET (razon), 150, -1);
  gtk_box_pack_start (GTK_BOX (box), razon, FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  label = gtk_label_new ("Dirección");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  direccion = gtk_entry_new_with_max_length (35);
  gtk_widget_show (direccion);
  gtk_widget_set_size_request (GTK_WIDGET (direccion), 200, -1);
  gtk_box_pack_start (GTK_BOX (box), direccion, FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  label = gtk_label_new ("Comuna");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  comuna = gtk_entry_new_with_max_length (35);
  gtk_widget_show (comuna);
  gtk_widget_set_size_request (GTK_WIDGET (comuna), 115, -1);
  gtk_box_pack_start (GTK_BOX (box), comuna, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  label = gtk_label_new ("Ciudad");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  ciudad = gtk_entry_new_with_max_length (35);
  gtk_widget_show (ciudad);
  gtk_widget_set_size_request (GTK_WIDGET (ciudad), 115, -1);
  gtk_box_pack_start (GTK_BOX (box), ciudad, FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  label = gtk_label_new ("Fono");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  fono = gtk_entry_new_with_max_length (35);
  gtk_widget_show (fono);
  gtk_widget_set_size_request (GTK_WIDGET (fono), 80, -1);
  gtk_box_pack_start (GTK_BOX (box), fono, FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  label = gtk_label_new ("Pagina Web");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  web = gtk_entry_new_with_max_length (35);
  gtk_widget_show (web);
  gtk_widget_set_size_request (GTK_WIDGET (web), 180, -1);
  gtk_box_pack_start (GTK_BOX (box), web, FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  label = gtk_label_new ("Nombre Contacto");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  contacto = gtk_entry_new_with_max_length (35);
  gtk_widget_show (contacto);
  gtk_widget_set_size_request (GTK_WIDGET (contacto), 150, -1);
  gtk_box_pack_start (GTK_BOX (box), contacto, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  label = gtk_label_new ("Correo Electronico");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  email = gtk_entry_new_with_max_length (35);
  gtk_widget_show (email);
  gtk_widget_set_size_request (GTK_WIDGET (email), 150, -1);
  gtk_box_pack_start (GTK_BOX (box), email, FALSE, FALSE, 0);

  box = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (box);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  label = gtk_label_new ("Giro");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  giro = gtk_entry_new_with_max_length (100);
  gtk_widget_show (giro);
  gtk_box_pack_start (GTK_BOX (box), giro, FALSE, FALSE, 0);

  button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (ModificarProveedor), NULL);

  /*
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Compras Totales");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    compras_totales = gtk_label_new ("");
    gtk_widget_show (compras_totales);
    //  gtk_widget_set_size_request (GTK_WIDGET (compras_totales), 80, -1);
    gtk_box_pack_start (GTK_BOX (box), compras_totales, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Contribucion Total Real");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    contrib_total = gtk_label_new ("");
    gtk_widget_show (contrib_total);
    //  gtk_widget_set_size_request (GTK_WIDGET (contrib_total), 80, -1);
    gtk_box_pack_start (GTK_BOX (box), contrib_total, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Contribucion Proyectada");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    contrib_proyect = gtk_label_new ("");
    gtk_widget_show (contrib_proyect);
    //  gtk_widget_set_size_request (GTK_WIDGET (contrib_proyect), 80, -1);
    gtk_box_pack_start (GTK_BOX (box), contrib_proyect, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Incidencia Compras");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    inci_compras = gtk_label_new ("");
    gtk_widget_show (inci_compras);
    //  gtk_widget_set_size_request (GTK_WIDGET (inci_compras), 80, -1);
    gtk_box_pack_start (GTK_BOX (box), inci_compras, FALSE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Stock Valorizado");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    stock_valorizado = gtk_label_new ("");
    gtk_widget_show (stock_valorizado);
    //  gtk_widget_set_size_request (GTK_WIDGET (stock_valorizado), 80, -1);
    gtk_box_pack_start (GTK_BOX (box), stock_valorizado, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Merma Unid.");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    merma_uni = gtk_label_new ("");
    gtk_widget_show (merma_uni);
    //  gtk_widget_set_size_request (GTK_WIDGET (merma_uni), 80, -1);
    gtk_box_pack_start (GTK_BOX (box), merma_uni, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Merma %");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    merma_porc = gtk_label_new ("");
    gtk_widget_show (merma_porc);
    //  gtk_widget_set_size_request (GTK_WIDGET (merma_porc), 80, -1);
    gtk_box_pack_start (GTK_BOX (box), merma_porc, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Ventas Totales");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    ventas_totales = gtk_label_new ("");
    gtk_widget_show (ventas_totales);
    //  gtk_widget_set_size_request (GTK_WIDGET (ventas_totales), 80, -1);
    gtk_box_pack_start (GTK_BOX (box), ventas_totales, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Contrib. Agreg.");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    contrib_agreg = gtk_label_new ("");
    gtk_widget_show (contrib_agreg);
    //  gtk_widget_set_size_request (GTK_WIDGET (contrib_agreg), 80, -1);
    gtk_box_pack_start (GTK_BOX (box), contrib_agreg, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Incidencia Ventas");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    inci_ventas = gtk_label_new ("");
    gtk_widget_show (inci_ventas);
    //  gtk_widget_set_size_request (GTK_WIDGET (inci_ventas), 80, -1);
    gtk_box_pack_start (GTK_BOX (box), inci_ventas, FALSE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Total Facturas Pendientes");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    total_pen_fact = gtk_label_new ("");
    gtk_widget_show (total_pen_fact);
    //  gtk_widget_set_size_request (GTK_WIDGET (total_pen_fact), 80, -1);
    gtk_box_pack_start (GTK_BOX (box), total_pen_fact, FALSE, FALSE, 0);

    box = gtk_vbox_new (FALSE, 3);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
    label = gtk_label_new ("Indice T");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    indice_t = gtk_label_new ("");
    gtk_widget_show (indice_t);
    //  gtk_widget_set_size_request (GTK_WIDGET (indice_t), 80, -1);
    gtk_box_pack_start (GTK_BOX (box), indice_t, FALSE, FALSE, 0);
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_label ("Imprimir listado de Proveedores");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  proveedores_print->tree = GTK_TREE_VIEW (proveedores_tree);
  proveedores_print->title = "Lista de Proveedores";
  proveedores_print->name = "proveedores";
  proveedores_print->date_string = NULL;
  proveedores_print->cols[0].name = "Nombre";
  proveedores_print->cols[0].num = 0;
  proveedores_print->cols[1].name = "Rut";
  proveedores_print->cols[1].num = 1;
  proveedores_print->cols[2].name = NULL;

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (PrintTree), (gpointer)proveedores_print);

}
