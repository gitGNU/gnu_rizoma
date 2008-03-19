/* -*- Mode: C; tab-width: 4; ident-tabs-mode: nil; c-basic-offset: 4 -*- */
/*main.c
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
#include<gdk/gdkkeysyms.h>


#include<stdlib.h>
#include<string.h>

#include"../config.h"

#include"tipos.h"

#include"main.h"
#include"administracion_productos.h"
#include"credito.h"
#include"usuario.h"
#include"encriptar.h"
#include"postgres-functions.h"
#include"errors.h"
#include"compras.h"
#include"caja.h"
#include"tiempo.h"
#include"control.h"
#include"datos.h"
#include"dimentions.h"
#include"rizoma_errors.h"
#include"config_file.h"

#include<libgksu.h>

GtkBuilder *builder;

GtkWidget *window;

GtkTreeView *treeview;

GtkWidget *hour_label;

// Definimos las variables para obtener el ancho y alto del screen
gint screen_width;
gint screen_height;

gboolean
main_key_handler (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  if (event->keyval == GDK_F12)
    {
      Question (NULL);
    }

  if (user_data->level == 0)
    {
      if (event->keyval == GDK_F7)
        {
          VentanaIngreso (NULL, NULL);
        }
      else if (event->keyval == GDK_F6)
        {
          VentanaEgreso (NULL, NULL);
        }
      else if (event->keyval == GDK_F1)
        {
          SelectMenu (NULL, "Ventas");
        }
      else if (event->keyval == GDK_F2)
        {
          SelectMenu (NULL, "Compras");
        }
      else if (event->keyval == GDK_F3)
        {
          SelectMenu (NULL, "Informes");
        }
    }

  return FALSE;
}

void
SelectMenu (GtkWidget *widget, gpointer data)
{
  gchar *name = (gchar *)data;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
  gint i;

  for (i = 0; i < sizeof modulos / sizeof modulos[0]; i++)
    if (strcmp (modulos[i].name, name) == 0)
      gtk_tree_selection_select_path (selection,
                                      gtk_tree_path_new_from_string
                                      (g_strdup_printf ("%d", i)));
}

void
Salir (MainBox *module_box)
{
  Asistencia (user_data->user_id, FALSE);
  gtk_main_quit ();
}

int
main (int argc, char *argv[])
{
  GtkWindow *login_window;
  GError *err = NULL;

  GtkComboBox *combo;
  GtkListStore *model;
  GtkTreeIter iter;
  GtkCellRenderer *cell;

  char *config_file;
  GKeyFile *key_file;
  gchar **profiles;

  /* dimension = (Dimensions *) g_malloc (sizeof (Dimensions)); */
  /* //  dimension->factura_cliente = (Position *) g_malloc (sizeof (Position)); */

  /* ParmsDimensions dimensions[17] = { */
  /* {"factura_size", &dimension->factura_size}, */
  /* {"factura_cliente", &dimension->factura_cliente}, */
  /* {"factura_address", &dimension->factura_address}, */
  /* {"factura_giro", &dimension->factura_giro}, */
  /* {"factura_rut", &dimension->factura_rut}, */
  /* {"factura_comuna", &dimension->factura_comuna}, */
  /* {"factura_fono", &dimension->factura_fono}, */
  /* {"factura_subtotal", &dimension->factura_subtotal}, */
  /* {"factura_iva", &dimension->factura_iva}, */
  /* {"factura_total", &dimension->factura_total}, */
  /* {"factura_detail_begin", &dimension->factura_detail_begin}, */
  /* {"factura_detail_end", &dimension->factura_detail_end}, */
  /* {"factura_codigo", &dimension->factura_codigo}, */
  /* {"factura_descripcion", &dimension->factura_descripcion}, */
  /* {"factura_cantidad", &dimension->factura_cantidad}, */
  /* {"factura_precio_uni", &dimension->factura_precio_uni}, */
  /* {"factura_precio_total", &dimension->factura_precio_total}, */
  /* }; */
  /* read_dimensions ("~/.rizoma_dimensions", dimensions, 17); */


  main_window = NULL;


  config_file = g_strconcat(g_getenv("HOME"),"/.rizoma", NULL);

  if (!g_file_test(config_file,
                   G_FILE_TEST_EXISTS|G_FILE_TEST_IS_REGULAR))
    {
      perror (g_strdup_printf ("Opening %s", config_file));
      printf ("Para configurar su sistema debe ejecutar rizoma-config\n");
      exit (-1);
    }
  key_file = g_key_file_new ();
  g_key_file_load_from_file(key_file, config_file, G_KEY_FILE_KEEP_COMMENTS, NULL);

  gtk_init (&argc, &argv);

  screen_width = gdk_screen_width ();
  screen_height = gdk_screen_height ();

  if (screen_width == 640 && screen_height == 480)
    solo_venta = TRUE;
  else
    solo_venta = FALSE;

  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-login.ui", &err);
  if (err) {
    g_error ("ERROR: %s\n", err->message);
    return -1;
  }
  gtk_builder_connect_signals (builder, NULL);

  login_window = GTK_WINDOW(gtk_builder_get_object (builder, "login_window"));

  profiles = g_key_file_get_groups (key_file, NULL);
  g_key_file_free (key_file);

  model = gtk_list_store_new (1,
                              G_TYPE_STRING);

  combo = (GtkComboBox *) gtk_builder_get_object (builder, "combo_profile");

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start ((GtkCellLayout *)combo, cell, TRUE);
  gtk_cell_layout_set_attributes ((GtkCellLayout *)combo, cell,
                                  "text", 0,
                                  NULL);


  do
    {
      if (*profiles != NULL)
        {
          gtk_list_store_append (model, &iter);
          gtk_list_store_set (model, &iter,
                              0, *profiles,
                              -1
                              );
        }
    } while (*profiles++ != NULL);

  gtk_combo_box_set_model (combo, (GtkTreeModel *)model);
  gtk_combo_box_set_active (combo, 0);

  gtk_widget_show_all ((GtkWidget *)login_window);

  gtk_main();

  return 0;
}

void
show_selected (GtkTreeSelection *selection, gpointer data)
{
  gint i;
  gchar *value;

  GtkTreeIter iter;
  GtkTreeView *treeview = gtk_tree_selection_get_tree_view (selection);
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE)
    {
      gtk_tree_model_get (model, &iter,
                          0, &value,
                          -1);

      for (i = 0; i < sizeof modulos / sizeof modulos[0]; i++)
        if (strcmp (modulos[i].name, value) == 0)
          {
            modulos[i].func((MainBox *) data);
            last_menu = current_menu;
            current_menu = i;
          }
    }
}

void
show_selected_in_button (GtkWidget *button, gpointer data)
{
  gint num = atoi (g_strdup (gtk_button_get_label (GTK_BUTTON (button))));

  modulos[num].func((MainBox *)data);
}


void
check_passwd (GtkWidget *widget, gpointer data)
{
  GtkTreeIter iter;
  GtkComboBox *combo = (GtkComboBox *) gtk_builder_get_object (builder, "combo_profile");
  GtkTreeModel *model = gtk_combo_box_get_model (combo);
  gchar *group_name;

  gchar *passwd = g_strdup (gtk_entry_get_text ( (GtkEntry *) gtk_builder_get_object (builder,"passwd_entry")));
  gchar *user = g_strdup (gtk_entry_get_text ( (GtkEntry *) gtk_builder_get_object (builder,"user_entry")));

  gtk_combo_box_get_active_iter (combo, &iter);
  gtk_tree_model_get (model, &iter,
                      0, &group_name,
                      -1);

  rizoma_set_profile (group_name);

  switch (AcceptPassword (passwd, user))
    {
    case TRUE:

      user_data = (User *) g_malloc (sizeof (User));

      user_data->user_id = ReturnUserId (user);
      user_data->level = ReturnUserLevel (user);
      user_data->user = user;

      Asistencia (user_data->user_id, TRUE);

      gtk_widget_destroy ( (GtkWidget *) gtk_builder_get_object (builder,"login_window"));
      g_object_unref ((gpointer) builder);
      builder = NULL;

      MainWindow ();

      break;
    case FALSE:
      gtk_entry_set_text ((GtkEntry *) gtk_builder_get_object (builder,"user_entry"), "");
      gtk_entry_set_text ((GtkEntry *) gtk_builder_get_object (builder,"passwd_entry"), "");
      rizoma_error_window ((GtkWidget *) gtk_builder_get_object (builder,"user_entry"));
      break;
    default:
      break;
    }
}

void
MainWindow (void)
{
  GError *err = NULL;

  GtkImage *image;
  gchar *image_path;

  MainBox *module_box = (MainBox *) g_malloc (sizeof (MainBox*));

  GtkAccelGroup *accel_generales;

  gint i;

  GtkListStore *store;
  GtkTreeIter iter;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *columna;
  GtkTreeSelection *selection;


  module_box->new_box = NULL;

  ingreso = (IngresoProducto *) g_malloc (sizeof (IngresoProducto));

  ingreso->products_window = NULL;

  ventastats = (VentasStats *) g_malloc (sizeof (VentasStats));

  creditos = (Creditos *) g_malloc (sizeof (Creditos));

  compra = (Compra *) g_malloc (sizeof (Compra));
  compra->header = NULL;
  compra->products_list = NULL;
  compra->header_compra = NULL;
  compra->products_compra = NULL;
  compra->current = NULL;

  caja = (Caja *) g_malloc (sizeof (Caja));

  compra->documentos = NULL;

  last_menu = 0;
  current_menu = 0;


  g_type_init ();

  /*
    Seteamos el Accel group en NULL
  */

  accel = NULL;

  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma.ui", &err);
  if (err) {
    g_error ("ERROR: %s\n", err->message);
    return;
  }
  gtk_builder_connect_signals (builder, NULL);

  /*
   * En el caso de usemos una ventan de 800x600 la configuracion sigue abajo
   */
  if (solo_venta == FALSE)
    {
      /*
        Usamos una ventana de >= 800x600
      */
      main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));
      gtk_widget_show_all (main_window);

      accel_generales = gtk_accel_group_new ();
      gtk_window_add_accel_group (GTK_WINDOW (main_window), accel_generales);


      store = gtk_list_store_new (3,
                                  G_TYPE_STRING,
                                  G_TYPE_INT,
                                  G_TYPE_INT
                                  );

      for (i = 0; i < sizeof modulos / sizeof modulos[0]; i++)
        {
          if (user_data->level == 0)
            {
              gtk_list_store_append (store, &iter);
              gtk_list_store_set (store, &iter,
                                  0, modulos[i].name,
                                  1, 15000,
                                  2, 1000,
                                  -1);
            }
          else if ((user_data->level != 0) && (modulos[i].normal_user == TRUE))
            {
              gtk_list_store_append (store, &iter);
              gtk_list_store_set (store, &iter,
                                  0, modulos[i].name,
                                  1, 15000,
                                  2, 1000,
                                  -1);
            }
        }

      treeview = GTK_TREE_VIEW (gtk_builder_get_object (builder, "menu_treeview"));

      gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (store));

      image_path = rizoma_get_value ("LOGO");

      if (image_path != NULL)
        {
          image = GTK_IMAGE (gtk_builder_get_object (builder, "logo_image"));
          gtk_image_set_from_file (image, image_path);
        }

      hour_label = GTK_WIDGET (gtk_builder_get_object (builder, "hour_label"));
      date_label = GTK_WIDGET (gtk_builder_get_object (builder, "date_label"));

      RefreshTime (NULL);

      module_box->main_box = GTK_WIDGET (gtk_builder_get_object (builder, "main_box"));

      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

      g_signal_connect (G_OBJECT (selection), "changed",
                        G_CALLBACK (show_selected),
                        (gpointer)module_box);


      renderer = gtk_cell_renderer_text_new ();
      columna = gtk_tree_view_column_new_with_attributes ("Menu",
                                                          renderer,
                                                          "text", 0,
                                                          "size", 1,
                                                          "weight", 2,
                                                          NULL);

      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), columna);

      gtk_tree_selection_select_path (selection, gtk_tree_path_new_from_string ("0"));

    }
  /*
   * Resulta que tenemos una ventana de 640x480, y esta tiene espacio solo para vender
   * por lo que necesita rediseñarse enterita :)
   */
  else if (solo_venta == TRUE)
    {
      /*
        Usamos una ventana de 640x480
      */

      main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window_chica"));
      gtk_widget_show_all (main_window);

      accel_generales = gtk_accel_group_new ();
      gtk_window_add_accel_group (GTK_WINDOW (main_window), accel_generales);

      module_box->main_box = GTK_WIDGET (gtk_builder_get_object (builder, "main_box_chico"));

      modulos[0].func(module_box);

    }
  /*
    Necesitamos saber si la caja ha sido inicializada o no
    pero solo si somos administradores
  */
  if (check_caja () == FALSE && user_data->level == 0)
    {
      if (caja->win == NULL)
        ErrorMSG (main_window, "La caja ha sido inicializada, "
                  "pero nunca cerrada");
    }
  else if (check_caja () == TRUE && user_data->level == 0)
    {
      CerrarCaja (ArqueoCajaLastDay ());
      InicializarCajaWin ();
    }

  g_timeout_add (1000, RefreshTime, (gpointer)hour_label);

  /*
   * Usamos el reloj solo si estamos en una resolucion de 800x600 o mayor
   */
  if (solo_venta == FALSE)
    g_timeout_add (1000, RefreshTime, NULL);

  gtk_main();

}

void
exit_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
  if (response_id == GTK_RESPONSE_YES)
    {
      Salir (NULL);
    }
  else if (response_id == GTK_RESPONSE_NO)
    {
      gtk_widget_hide (GTK_WIDGET (dialog));
      current_menu = last_menu;
      gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)),
                                      gtk_tree_path_new_from_string (g_strdup_printf ("%d", last_menu)));
    }
}

void
Question (MainBox *module_box)
{
  GtkWidget *window;

  window = GTK_WIDGET (gtk_builder_get_object (builder, "quit_message"));

  gtk_widget_show_all (window);
}

