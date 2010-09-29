/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
/*admin.c
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

#include<gtk/gtk.h>
#include<stdlib.h>

#include"../config.h"

#include"admin.h"
#include"tipos.h"
#include"usuario.h"
#include"config_file.h"
#include"postgres-functions.h"
#include"encriptar.h"
#include"rizoma_errors.h"
#include"impuestos.h"
#include"datos_negocio.h"
#include"parametros.h"
#include"credito.h"

//the builder that contains all the UI
GtkBuilder *builder;


/**
 * The main function of the rizoma-admin application
 *
 * @param argc the number of arguments passed
 * @param argv an array of arguments
 *
 * @return the exit code
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

  if (err) 
    {
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
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), cell,
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

/**
 * Callback connected to the login button.
 *
 *Check the username and password, and if the pair is correct raise
 * the main window, otherwise raise an alert
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
      
      if(user_data->level == 0)
	{
	  gtk_widget_destroy (GTK_WIDGET(gtk_builder_get_object (builder,"login_window")));
	  g_object_unref ((gpointer) builder);
	  builder = NULL;
	  admin_win();
	}
      else
	{
	  gtk_entry_set_text ((GtkEntry *) gtk_builder_get_object (builder,"user_entry"), "");
	  gtk_entry_set_text ((GtkEntry *) gtk_builder_get_object (builder,"passwd_entry"), "");
	  rizoma_errors_set ("El usuario no tiene Permiso a Rizoma Admin", "AcceptPassword ()", ALERT);
	  rizoma_error_window ((GtkWidget *) gtk_builder_get_object (builder,"user_entry"));
	}
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
 * Setup the main window of the administration application (rizoma-admin).
 *
 */
void
admin_win()
{
  GtkWidget *admin_gui;
  GError *error=NULL;

  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-admin.ui", &error);

  if (error != NULL)
    g_printerr ("%s: %s\n", G_STRFUNC, error->message);

  error = NULL;

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-common.ui", &error);

  if (error != NULL)
    g_printerr ("%s: %s\n", G_STRFUNC, error->message);

  gtk_builder_connect_signals (builder, NULL);

  admin_gui = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_admin"));

  // check if the window must be set to fullscreen
  if (rizoma_get_value_boolean("FULLSCREEN"))
    gtk_window_fullscreen(GTK_WINDOW(admin_gui));

  //setup the different tabs
  user_box(); //users tab
  taxes_box(); //taxes tab
  datos_box(); //business info
  preferences_box(); //preferences tab
  clientes_box(); //clients tab
  gtk_widget_show_all (admin_gui);
}

/**
 * Callback connected to the quit button and delete-event of the main window.
 *
 * @param button the button that emits the signal
 * @param user_data the user data
 */
void
on_btn_quit_clicked (GtkButton *button, gpointer user_data)
{
  GtkWidget *widget;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "quit_message"));

  gtk_widget_show_all(widget);
}

/**
 * Quits the application
 *
 * @param button the button that emits the signal
 * @param user_data the user data
 */
void
on_quit_application (GtkButton *button, gpointer user_data)
{
  gtk_main_quit ();
}


/**
 * Callback connected to the switch-page signal of the main
 * gtknotebook in rizoma-admin
 *
 * This function refresh the information that contains every page when
 * the user switch to the page_num.
 *
 * @param notebook the notebook that emits the signal
 * @param page the current page
 * @param page_num the page number
 * @param data the user data
 */
void
on_notebook1_switch_page (GtkNotebook *notebook,
                          GtkNotebookPage *page,
                          guint page_num,
                          gpointer data)
{

  switch (page_num)
    {
    case 0:
      FillUsers();
    }

}
