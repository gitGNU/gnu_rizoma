/*rizoma-config.c
 *
 *    Copyright (C) 2006,2008 Rizoma Tecnologia Limitada <info@rizoma.cl>
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
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include<gtk/gtk.h>

#include<unistd.h>
#include<sys/types.h>

#include<libpq-fe.h>

/* The postgres entry */
GtkWidget *pg_local_user;
GtkWidget *pg_host;
GtkWidget *pg_user;
GtkWidget *pg_pass;

gchar *local_user_pg;
gchar *host_pg;
gchar *user_pg;
gchar *pass_pg;

/* Rizoma init values */
GtkWidget *admin_user;
GtkWidget *admin_pass;
GtkWidget *db_name;
GtkWidget *data_sql;

gchar *user_admin;
gchar *pass_admin;
gchar *name_db;
gchar *sql_data;

gchar *user;


void
create_config (void)
{

  FILE *fp;

  name_db = g_strdup (gtk_entry_get_text (GTK_ENTRY (db_name)));
  host_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (pg_host)));
  user_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (pg_user)));
  pass_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (pg_pass)));

  fp = fopen (g_strdup_printf ("%s/.rizoma", getenv("HOME")), "w+");

  fprintf (fp, "DB_NAME = %s\n", name_db);
  fprintf (fp, "USER = %s\n", user_pg);
  fprintf (fp, "PASSWORD = %s\n", pass_pg);
  fprintf (fp, "SERVER_HOST = %s\n", host_pg);
  fprintf (fp, "TEMP_FILES = /tmp\n");
  fprintf (fp, "VALE_DIR = /tmp\n");
  fprintf (fp, "VALE_COPY = 1\n");
  fprintf (fp, "VENDEDOR = 1\n");
  fprintf (fp, "MAQUINA = 1\n");
  fprintf (fp, "VENTA_DIRECTA = 0\n");

  fclose (fp);

  gtk_main_quit ();
}

void
close_window (GtkWidget *widget, gpointer user_data)
{
  gtk_widget_destroy (gtk_widget_get_toplevel (widget));
}

void
volcar_db (GtkWidget *button, gpointer user_data)
{
  FILE *fp;
  gchar *path;
  gchar *command;
  gchar *sql;

  GtkWidget *window;
  GtkWidget *button2;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *hbox;

  user_admin = g_strdup (gtk_entry_get_text (GTK_ENTRY (admin_user)));
  pass_admin = g_strdup (gtk_entry_get_text (GTK_ENTRY (admin_pass)));
  name_db = g_strdup (gtk_entry_get_text (GTK_ENTRY (db_name)));
  sql_data = g_strdup (gtk_entry_get_text (GTK_ENTRY (data_sql)));

  path = g_strdup_printf ("%s/rizoma.structure", sql_data);

  fp = fopen (path, "r");
  if (fp == NULL)
    {
	  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gtk_widget_get_toplevel (button)));

	  vbox = gtk_hbox_new (FALSE, 3);
	  gtk_container_add (GTK_CONTAINER (window), vbox);

	  label = gtk_label_new ("No fue posible encontrar el archivo SQL con las tablas de rizoma.\n"
							 "Por favor, asegurese de que la ruta es correcta y vuelva repetir el paso.");
	  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);

	  hbox = gtk_hbox_new (FALSE, 3);
	  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

	  button2 = gtk_button_new_from_stock (GTK_STOCK_OK);
	  gtk_box_pack_end (GTK_BOX (hbox), button2, FALSE, FALSE, 3);

	  g_signal_connect (G_OBJECT (button2), "clicked",
						G_CALLBACK (close_window), NULL);

	  gtk_widget_show_all (window);
	  return;
    }
  else
	fclose (fp);

  path = g_strdup_printf ("%s/rizoma.initvalues", sql_data);

  fp = fopen (path, "r");
  if (fp == NULL)
    {
	  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gtk_widget_get_toplevel (button)));

	  vbox = gtk_hbox_new (FALSE, 3);
	  gtk_container_add (GTK_CONTAINER (window), vbox);

	  label = gtk_label_new ("No fue posible encontrar el archivo SQL con los datos iniciales de rizoma.\n"
							 "Por favor, asegurese de que la ruta es correcta y vuelva repetir el paso.");
	  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);

	  hbox = gtk_hbox_new (FALSE, 3);
	  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

	  button2 = gtk_button_new_from_stock (GTK_STOCK_OK);
	  gtk_box_pack_end (GTK_BOX (hbox), button2, FALSE, FALSE, 3);

	  g_signal_connect (G_OBJECT (button2), "clicked",
						G_CALLBACK (close_window), NULL);

	  gtk_widget_show_all (window);
	  return;
    }
  else
	fclose (fp);

  command = g_strdup_printf ("su - %s -c \"psql template1 -c \\\"DROP DATABASE %s; CREATE DATABASE %s OWNER %s;\\\"\"", local_user_pg, name_db, name_db, user_pg);
  printf ("%s\n",command);
  if ((system (command)) != 0)
    {
	  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gtk_widget_get_toplevel (button)));

	  vbox = gtk_hbox_new (FALSE, 3);
	  gtk_container_add (GTK_CONTAINER (window), vbox);

	  label = gtk_label_new ("Hubo problemas mientras se creaba la base de datos.\n"
							 "Por favor, asegurese de tener los permisos necesarios y que esta usando gksu\n"
							 "y de que ha creado el usuario previamente.");
	  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);

	  hbox = gtk_hbox_new (FALSE, 3);
	  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

	  button2 = gtk_button_new_from_stock (GTK_STOCK_OK);
	  gtk_box_pack_end (GTK_BOX (hbox), button2, FALSE, FALSE, 3);

	  g_signal_connect (G_OBJECT (button2), "clicked",
						G_CALLBACK (close_window), NULL);

	  gtk_widget_show_all (window);
	  return;
    }



  path = g_strdup_printf ("%s/rizoma.structure", sql_data);
  command = g_strdup_printf ("su - %s -c \"psql -U %s %s -f %s\"", local_user_pg, user_pg, name_db, path);
  printf ("%s\n", command);
  if ((system (command)) != 0)
	{
	  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gtk_widget_get_toplevel (button)));

	  vbox = gtk_hbox_new (FALSE, 3);
	  gtk_container_add (GTK_CONTAINER (window), vbox);

	  label = gtk_label_new ("Hubo un problema desconocido mientras se intentaba crear las tablas.\n"
							 "Por favor, asegurese de tener los permisos necesarios y que esta usando gksu.");
	  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);

	  hbox = gtk_hbox_new (FALSE, 3);
	  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

	  button2 = gtk_button_new_from_stock (GTK_STOCK_OK);
	  gtk_box_pack_end (GTK_BOX (hbox), button2, FALSE, FALSE, 3);

	  g_signal_connect (G_OBJECT (button2), "clicked",
						G_CALLBACK (close_window), NULL);

	  gtk_widget_show_all (window);
	  return;
	}

  path = g_strdup_printf ("%s/rizoma.initvalues", sql_data);
  command = g_strdup_printf ("su - %s -c \"psql -U %s %s -f %s\"", local_user_pg, user_pg, name_db, path);
  printf("%s\n", command);
  if ((system (command)) != 0)
	{
	  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gtk_widget_get_toplevel (button)));

	  vbox = gtk_hbox_new (FALSE, 3);
	  gtk_container_add (GTK_CONTAINER (window), vbox);

	  label = gtk_label_new ("Hubo un problema desconocido mientras se intentaba ingresar los datos inicialess.\n"
							 "Por favor, asegurese de tener los permisos necesarios y que esta usando gksu.");
	  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);

	  hbox = gtk_hbox_new (FALSE, 3);
	  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

	  button2 = gtk_button_new_from_stock (GTK_STOCK_OK);
	  gtk_box_pack_end (GTK_BOX (hbox), button2, FALSE, FALSE, 3);

	  g_signal_connect (G_OBJECT (button2), "clicked",
						G_CALLBACK (close_window), NULL);

	  gtk_widget_show_all (window);
	  return;
	}


  sql = g_strdup_printf ("INSERT INTO users (id, usuario, passwd, rut, dv, nombre, apell_p, apell_m, fecha_ingreso, level ) VALUES (DEFAULT, '%s', md5('%s'), 0, 0, 'Administrador', '', '', NOW(), 0)", user_admin, pass_admin);

  command = g_strdup_printf ("su - %s -c \"psql -U %s %s -c \\\"%s\\\"\"", local_user_pg, user_pg, name_db, sql);

  g_print( command );
  if ((system (command)) != 0)
	{
	  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gtk_widget_get_toplevel (button)));

	  vbox = gtk_hbox_new (FALSE, 3);
	  gtk_container_add (GTK_CONTAINER (window), vbox);

	  label = gtk_label_new (g_strdup_printf ("Hubo un problema desconocido mientras se creaba el usuario administrador\n"
											  "Por favor, sin este usuario no podra crear los datos no intente repetir este paso\n"
											  "ya que se repetiran datos que podrian causar error. Asegurese de tener los datos eliminados\n"
											  "si desea repetir el wizard, de lo contrario ejecute esta llamada SQL:\n\n%s\n", sql));
	  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);

	  hbox = gtk_hbox_new (FALSE, 3);
	  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

	  button2 = gtk_button_new_from_stock (GTK_STOCK_OK);
	  gtk_box_pack_end (GTK_BOX (hbox), button2, FALSE, FALSE, 3);

	  g_signal_connect (G_OBJECT (button2), "clicked",
						G_CALLBACK (close_window), NULL);

	  gtk_widget_show_all (window);
	  return;
	}

  path = g_strdup_printf ("%s/funciones.sql", sql_data);
  command = g_strdup_printf ("su - %s -c \"psql -U %s %s -f %s\"", local_user_pg, user_pg, name_db, path);
  g_print( command );
  if ((system (command)) != 0)
	{
	  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gtk_widget_get_toplevel (button)));

	  vbox = gtk_hbox_new (FALSE, 3);
	  gtk_container_add (GTK_CONTAINER (window), vbox);

	  label = gtk_label_new (g_strdup_printf ("Hubo un problema desconocido mientras se creaban las funciones PL/SQL\n"
											  "Sin estas funciones el sistema no puede funcionar por lo que el problema debe ser resuelto.\n"
											  "Para crear las funciones ejecute el siguiente archivo en la base de datos:\n"
											  "\n%s\n", path ));
	  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);

	  hbox = gtk_hbox_new (FALSE, 3);
	  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

	  button2 = gtk_button_new_from_stock (GTK_STOCK_OK);
	  gtk_box_pack_end (GTK_BOX (hbox), button2, FALSE, FALSE, 3);

	  g_signal_connect (G_OBJECT (button2), "clicked",
						G_CALLBACK (close_window), NULL);

	  gtk_widget_show_all (window);
	  return;
	}


}


/*gboolean
get_db_user_info( GnomeDruidPage *page, GtkWidget *widget, gpointer user_data ) {

  local_user_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (pg_local_user)));
  host_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (pg_host)));
  user_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (pg_user)));
  pass_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (pg_pass)));

  return FALSE;
}

*/

void
create_db_user (GtkWidget *button, gpointer user_data)
{
  gchar *command;
  uid_t uid;

  GtkWidget *window;
  GtkWidget *button2;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *hbox;
  g_print( "estoy aqui\n");
  uid = getuid ();

  if (uid != 0)
    {
	  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gtk_widget_get_toplevel (button)));

	  vbox = gtk_hbox_new (FALSE, 3);
	  gtk_container_add (GTK_CONTAINER (window), vbox);

	  label = gtk_label_new ("No se pudo crear el usuario de postgres ya que no consta con los permisos para ello.\n"
							 "Es recomendable correr rizoma-config usando gksu.");
	  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);

	  hbox = gtk_hbox_new (FALSE, 3);
	  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

	  button2 = gtk_button_new_from_stock (GTK_STOCK_OK);
	  gtk_box_pack_end (GTK_BOX (hbox), button2, FALSE, FALSE, 3);

	  gtk_widget_show_all (window);

    }
  else
    {
	  command = g_strdup_printf ("su - %s -c \"psql template1 -c \\\"CREATE USER %s ENCRYPTED PASSWORD \'%s\' CREATEDB NOCREATEUSER;\\\"\"",
								 local_user_pg, user_pg, pass_pg);
	  if ((system (command)) == 0)
		{
		  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gtk_widget_get_toplevel (button)));

		  vbox = gtk_hbox_new (FALSE, 3);
		  gtk_container_add (GTK_CONTAINER (window), vbox);

		  label = gtk_label_new ("El usuario de postgres se ha creado con exito\n");
		  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);

		  hbox = gtk_hbox_new (FALSE, 3);
		  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

		  button2 = gtk_button_new_from_stock (GTK_STOCK_OK);
		  gtk_box_pack_end (GTK_BOX (hbox), button2, FALSE, FALSE, 3);

		  gtk_widget_show_all (window);
		}
	  else
		{
		  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (gtk_widget_get_toplevel (button)));

		  vbox = gtk_hbox_new (FALSE, 3);
		  gtk_container_add (GTK_CONTAINER (window), vbox);

		  label = gtk_label_new ("Se produjo un error desconocido mientras se creaba el usuario postgres.\n"
								 "Por favor, asegurse de que los datos ingresados son correctos y vuelva intentar la operacion.");
		  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);

		  hbox = gtk_hbox_new (FALSE, 3);
		  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);

		  button2 = gtk_button_new_from_stock (GTK_STOCK_OK);
		  gtk_box_pack_end (GTK_BOX (hbox), button2, FALSE, FALSE, 3);

		  gtk_widget_show_all (window);
		}
    }
}


int
main (int argc, char *argv[])
{
  GtkBuilder *builder;
  GError *err = NULL;
  GtkAssistant *asistente;
  GtkWidget *page;
  gtk_init (&argc, &argv);

  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-config.ui", &err);
  if (err) {
	g_error ("ERROR: %s\n", err->message);
	return -1;
  }

  gtk_builder_connect_signals (builder, NULL);

  asistente = (GtkAssistant *) gtk_builder_get_object (builder, "asistente" );

  page = gtk_assistant_get_nth_page (asistente, 0 );

  gtk_assistant_set_page_complete (asistente, page, TRUE);

  gtk_widget_show_all ( (GtkWidget *) asistente);

  gtk_main ();

  return 0;
}
