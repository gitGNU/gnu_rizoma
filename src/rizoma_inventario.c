/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
/*rizoma-inventario.c
 *
 *    Copyright (C) 2008 Rizoma Tecnologia Limitada <info@rizoma.cl>
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

#include<gtk/gtk.h>
#include<gdk/gdkkeysyms.h>

#include<math.h>
#include<stdlib.h>
#include<string.h>


#include "tipos.h"

#include "postgres-functions.h"
#include "config_file.h"
#include "utils.h"
#include "manejo_productos.h"

GtkBuilder *builder;


/**
 * Es llamada por la funcion "on_btn_ejecutar_Inv" y recibe el parametro tipo
 * FILE, donde se carga el archivo
 * 
 * Esta funcion lee lineas del archivo y retorna una linea 
 *
 * @param FILE contiene los datos del archivo
 *
 * @return line contiene una linea del archivo
 */

gchar *
get_line (FILE *fd)
{
  int c;
  int i = 0;
  char *line = (char *) malloc (255);

  memset (line, '\0', 255);

  for (c = fgetc (fd); c != '#' && c != '\n' && c != EOF; c = fgetc (fd))
    {
      *line++ = c;
      i++;
    }


  for (; c != '\n' && c != EOF; c = fgetc (fd));


  if (i > 0 || c != EOF)
    {
      for (; i != 0; i--)
        *line--;

      return line;
    }
  else
    return NULL;
}

/**
 * Es llamada por la funcion "check_passwd".
 *
 * Esta funcion carga los datos "rizoma-inventario.ui" y visualiza la ventana "wnd_inventario"
 */
 
void
inventario_win ()
{
  
  GtkWidget *inventario_gui,*fcb;
  GError *error = NULL;
  GtkFileFilter *filter;

  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-inventario.ui", &error);

  if (error != NULL) {
    g_printerr ("%s: %s\n", G_STRFUNC, error->message);
  }

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-common.ui", &error);

  if (error != NULL) {
    g_printerr ("%s: %s\n", G_STRFUNC, error->message);
  }
  /*filtro para seleccionar archivos*/
  filter = gtk_file_filter_new();
  gtk_file_filter_add_pattern (filter, "*.csv");
  gtk_file_filter_set_name (filter, "CSV Hojas de Calculo");

  fcb = GTK_WIDGET (gtk_builder_get_object (builder, "filechooserbutton1"));

  /* añade el filtro al filechooserbutton*/
  gtk_file_chooser_set_filter(GTK_FILE_CHOOSER (fcb), 
			      filter);

  gtk_builder_connect_signals (builder, NULL);
  inventario_gui = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_inventario"));
  gtk_widget_show_all (inventario_gui);

  
}

/**
 * Connected to accept button of the login window
 *
 * This funcion is in charge of validate the user/password and then
 * call the initialization functions
 *
 * @param widget the widget that emits the signal
 * @param data the user data
 */

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

      gtk_widget_destroy (GTK_WIDGET(gtk_builder_get_object (builder,"login_window")));
      g_object_unref ((gpointer) builder);
      builder = NULL;

      inventario_win ();

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

/**
 * Funcion Principal
 *
 * Carga los valores de rizoma-login.ui y visualiza la ventana del
 * login "login_window"
 *
 */

int
main (int argc, char **argv)
{
  GtkWindow *login_window;
  GError *err = NULL;

  GtkComboBox *combo;
  GtkListStore *model;
  GtkTreeIter iter;
  GtkCellRenderer *cell;

  
  GKeyFile *key_file;
  gchar **profiles;

  key_file = rizoma_open_config();

  if (key_file == NULL)
    {
      g_error ("Cannot open config file\n");
      return -1;
    }

  gtk_init (&argc, &argv);

  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-login.ui", &err);
  if (err) {
    g_error ("ERROR: %s\n", err->message);
    return -1;
  }
  gtk_builder_connect_signals (builder, NULL);

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

  gtk_widget_show_all (GTK_WIDGET (builder_get (builder, "login_window")));

  gtk_main ();

  return 0;
}

/**
 * Es llamada cuando el boton "btn_ejecutar_Inv" es presionado (signal click).
 *
 * Esta funcion carga el archivo que contiene el inventario y carga los datos
 * de cada producto (barcode, cantidad, precio_compra, precio_final) y registra
 * la compra con un proveedor ficticio llamado "inventario".
 *
 * @param button the button
 * @param user_data the user data
 */

void
on_btn_ejecutar_Inv (GtkButton *button, gpointer data)
{  
  GKeyFile *key_file;
  FILE *fp;
  char *line = NULL;
  char *barcode;
  double cant = 100.0;
  double pcomp = 0.0;
  int precio = 1000;
  int margen = 20;
  int cont_products_no_BD = 0,cont_products = 0;
  char * pEnd;
  gchar *dir, *string, *stringfinal;
  GtkTextBuffer *buffer;
  GtkTextMark *mark;
  GtkTextIter iter;
  GdkColor color;

  gdk_color_parse ("red", &color);
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (gtk_builder_get_object (builder, "textview1")));
  gtk_text_buffer_create_tag (buffer, "color_red", "foreground-gdk", &color, NULL);

  gtk_text_buffer_create_tag (buffer, "size_medium", "scale", PANGO_SCALE_LARGE, NULL);
  gtk_text_buffer_create_tag (buffer, "style_italic", "style", PANGO_STYLE_ITALIC, NULL); 


  dir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER  (builder_get (builder, "filechooserbutton1")));
  fp = fopen (dir, "r");

  if (fp == NULL)
    {
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "msg_error_file")));
      return;
    }
  
  compra = (Compra *) g_malloc (sizeof (Compra));
  compra->header = NULL;
  compra->products_list = NULL;
  compra->header_compra = NULL;
  compra->products_compra = NULL;
  compra->current = NULL;

  key_file = rizoma_open_config();

  if (key_file == NULL)
    {
      g_error ("Cannot open config file\n");
      
    }
  
  if (!DataExist ("SELECT rut FROM proveedor WHERE rut=99999999"))
    AddProveedorToDB ("99999999-9", "Inventario", "", "", "", "0", "", "","Inventario", "Inventario");
 
  /* Carga los valores de los pruductos linea por linea del archivo */
  do
    {
      line = get_line (fp);
      if (line != NULL)
        {
          barcode = strtok (line, ",");
          
          pcomp = g_ascii_strtod (strtok (NULL, ","), &pEnd);
          
          precio = atoi (strtok (NULL, ","));

          cant = g_ascii_strtod (strtok (NULL, ","), &pEnd);
          
          if ((DataExist (g_strdup_printf ("SELECT barcode FROM producto WHERE barcode=%s", barcode))))
            {
              CompraAgregarALista (barcode, cant, precio, pcomp, margen,FALSE);
              string = g_strdup_printf (" El producto con barcode %s se ingreso correctamente \n", barcode);
              mark = gtk_text_buffer_get_insert(buffer);
              gtk_text_buffer_get_iter_at_offset (buffer, &iter, (gint) mark);
              gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, string, -1
                                                       ,"size_medium", "style_italic" ,NULL);             
            }
          
          else
            {
              string = g_strdup_printf ("El producto %s no esta en la bd \n", barcode);
              mark = gtk_text_buffer_get_insert(buffer);
              gtk_text_buffer_get_iter_at_offset (buffer, &iter,(gint) mark);
              gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, string, -1
                                                       ,"size_medium", "style_italic" ,"color_red",NULL);
              cont_products_no_BD++;
            }

          cont_products++;

        }
    } while (line != NULL);


  /* Mensaje si no realiza ningun inventario */
  if (cont_products_no_BD == cont_products)
    {
      gtk_widget_show (GTK_WIDGET (builder_get (builder, "msg_NoOK")));
      return;
    }
  else
    {
        /* Mensaje si realiza correctamente el inventario */
      if(cont_products_no_BD == 0)
        {
          AgregarCompra ("99999999", "", 1);
          gtk_widget_show (GTK_WIDGET (builder_get (builder, "msg_OK")));
          return;
        }
      
        /* Mensaje si se realiza parcialmente el inventario*/
      else
        {
          AgregarCompra ("99999999", "", 1);
          gtk_widget_show (GTK_WIDGET (builder_get (builder, "msg_casiOK")));;
          return;
        }
    }   
}

