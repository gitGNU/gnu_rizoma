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
  
  GtkWidget *inventario_gui;
  GError *error = NULL;

  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-inventario.ui", &error);

  if (error != NULL) {
    g_printerr ("%s: %s\n", G_STRFUNC, error->message);
  }

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-common.ui", &error);

  if (error != NULL) {
    g_printerr ("%s: %s\n", G_STRFUNC, error->message);
  }

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
 * Es llamada por la funcion "on_btn_ejecutar_Inv" y recibe los parametros
 * char cadena y int indice
 *
 * Esta funcion recibe una cadena de texto en este caso la direccion del
 * archivo, y corta los primeros 7 caracteres, en este caso (File://) y
 * retorna la cadena resultante
 *
 * @param cadena es un cadena de texto que contiene la direccion del archivo.
 * @param indice es un entero que contiene el valor
 * @return subcadena cadena resultante del recorte
 */

char *
cortar(char *cadena, int indice)
{
   char *subcadena;
   int i;
   /* Verifica longitud */

   if(indice>strlen (cadena)-1)
     return '\0';
   
   /* Realiza copia desde el indice hasta el final de la cadena */
   
   for(i=indice; i<strlen (cadena); i++)
     subcadena[i-indice] = cadena[i];

   /* Coloca caracter de fin de cadena */

   subcadena[i-indice] = '\0';

   return subcadena;
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
  char * pEnd, *dir,*dir2;
  dir = g_strdup (gtk_entry_get_text (GTK_ENTRY (builder_get (builder, "direccion_entry"))));
  dir2 = cortar(dir,7);
  
  fp = fopen (dir2, "r");

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
            CompraAgregarALista (barcode, cant, precio, pcomp, margen,FALSE);
          
          else
            {
              printf ("El producto %s no esta en la bd y \n",barcode);
              cont_products_no_BD++;
            }

          cont_products++;

        }
    } while (line != NULL);
  
  fclose (fp);
  //IngresarFakeCompra ();

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

/**
 * Es llamada cuando el boton "btn_abrir" es presionado (signal click).
 *
 * Esta funcion carga los datos de FileChooser y visualiza la ventana filechooserdialog
 *
 * @param button the button
 * @param user_data the user data
 */

void
on_btn_abrir_clicked (GtkButton *button, gpointer data)
{

  GtkFileChooser *window;
  GtkFileFilter *filter;
  
  filter = gtk_file_filter_new ();
  gtk_file_filter_add_pattern (filter, "*.csv");
  gtk_file_filter_set_name (filter, "CSV Hojas de Calculo");
  window = GTK_FILE_CHOOSER (gtk_builder_get_object (builder, "filechooserdialog"));
  gtk_file_chooser_add_filter (window, filter);

  clean_container (GTK_CONTAINER (window));
  gtk_widget_show_all (GTK_WIDGET (window));

}

/**
 * Es llamada cuando el boton "btn_abrir_filechooser" es presionado (signal
 * click) o cuando se hace doble click sobre el archivo seleccionado .
 *
 * Esta funcion carga la direccion del arhivo y visuliza la dirreccion en el
 * entry "direccion_entry"
 *
 * @param chooser es el valor tipo Filechooser
 * @param user_data the user data
 */

void
on_btn_abrir_filechooser (GtkFileChooser *chooser, gpointer data)
{
  gchar *q;
  q = gtk_file_chooser_get_uri(chooser);
  gtk_entry_set_text ((GtkEntry *) gtk_builder_get_object (builder,"direccion_entry"),q);
  gtk_widget_hide (GTK_WIDGET (builder_get (builder, "filechooserdialog")));
  gtk_widget_grab_focus (GTK_WIDGET (builder_get (builder, "btn_abrir")));
        
}
