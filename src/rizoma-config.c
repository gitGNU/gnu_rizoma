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
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include<gtk/gtk.h>

#include<unistd.h>
#include<sys/types.h>

#include<libpq-fe.h>

/* The Builder */
GtkBuilder *builder;

gchar *local_user_pg;
gchar *host_pg;
gchar *user_pg;
gchar *pass_pg;

gchar *user_admin;
gchar *pass_admin;
gchar *name_db;
gchar *sql_data;

gchar *user;

void
create_config (void)
{

  FILE *fp;

  /*  name_db = g_strdup (gtk_entry_get_text (GTK_ENTRY (db_name)));
  host_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (pg_host)));
  user_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (pg_user)));
  pass_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (pg_pass)));
  */
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
dialog_close (GtkDialog *dialog, gint response_id, gpointer user_data)
{
  if (response_id == -7 || response_id == -9)
	{
	  gtk_widget_hide ( (GtkWidget *) dialog);
	}
  else if (response_id == -8 || response_id == -5)
	{
	  GtkAssistant *asistente;
	  GtkWidget *page;
	  gint current_page;

	  asistente = (GtkAssistant *) gtk_builder_get_object (builder, "asistente" );

	  current_page = gtk_assistant_get_current_page (asistente);
	  page = gtk_assistant_get_nth_page (asistente, current_page);
	  gtk_assistant_set_page_complete (asistente, page, TRUE);

	  gtk_widget_hide ( (GtkWidget *) dialog);
	}
}

void
volcar_db (GtkWidget *button, gpointer user_data)
{
  FILE *fp;
  gchar *path;
  gchar *command;
  gchar *sql;

  GtkMessageDialog *dialog;

  user_admin = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "admin_user"))));
  pass_admin = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "admin_pass"))));
  name_db = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "db_name"))));
  sql_data = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "ruta_sql"))));

  if( strcmp (user_admin, "") == 0 || strcmp (pass_admin, "") == 0 || strcmp (name_db, "") == 0 || strcmp (sql_data, "") == 0 )
	{
	  dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "missing_data");
	  gtk_widget_show_all ( (GtkWidget *) dialog );
	  return;
	}

  command = g_strdup_printf ("su - %s -c \"dropdb %s; createdb -O %s %s;\"",
							 local_user_pg, name_db, user_pg, name_db);
  g_print ("Command: %s\n",command);

  if ((system (command)) != 0)
    {
	  dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "error_create_db");
	  gtk_widget_show_all ( (GtkWidget *) dialog );

	  return;
    }

  path = g_strdup_printf ("%s/rizoma.structure", sql_data);
  fp = fopen (path, "r");

  if (fp == NULL)
	{
	  dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "error_sql_data_schema");
	  gtk_widget_show_all ( (GtkWidget *) dialog );
	  return;
	}
  else
	{
	  fclose (fp);

	  command = g_strdup_printf ("su - %s -c \"psql -U %s %s -f %s\"",
								 local_user_pg, user_pg, name_db, path);
	  g_print ("Command: %s\n", command);

	  if ((system (command)) != 0)
		{
		  dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "error_sql_create_schema");
		  gtk_widget_show_all ( (GtkWidget *) dialog );
		  return;
		}

	}

  path = g_strdup_printf ("%s/rizoma.initvalues", sql_data);

  fp = fopen (path, "r");
  if (fp == NULL)
    {
	  dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "error_sql_data_values");
	  gtk_widget_show_all ( (GtkWidget *) dialog );

	  return;
    }
  else
	{
	  fclose (fp);

	  path = g_strdup_printf ("%s/rizoma.initvalues", sql_data);
	  command = g_strdup_printf ("su - %s -c \"psql -U %s %s -f %s\"",
								 local_user_pg, user_pg, name_db, path);
	  g_print ("Command: %s\n", command);
	  if ((system (command)) != 0)
		{
		  dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "error_sql_insert_values");
		  gtk_widget_show_all ( (GtkWidget *) dialog );

		  return;
		}
	}

  sql = g_strdup_printf ("INSERT INTO users (id, usuario, passwd, rut, dv, nombre, apell_p, apell_m, fecha_ingreso, level )"
						 " VALUES (DEFAULT, '%s', md5('%s'), 0, 0, 'Administrador', '', '', NOW(), 0)",
						 user_admin, pass_admin);
  command = g_strdup_printf ("su - %s -c \"psql -U %s %s -c \\\"%s\\\"\"",
							 local_user_pg, user_pg, name_db, sql);

  g_print ("Command: %s\n", command);

  if ((system (command)) != 0)
	{
	  dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "error_create_admin_user");
	  gtk_widget_show_all ( (GtkWidget *) dialog );

	  return;
	}

  path = g_strdup_printf ("%s/funciones.sql", sql_data);
  command = g_strdup_printf ("su - %s -c \"psql -U %s %s -f %s\"", local_user_pg, user_pg, name_db, path);
  g_print( command );
  if ((system (command)) != 0)
	{
	  dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "error_create_plsql");
	  gtk_widget_show_all ( (GtkWidget *) dialog );

	  return;
	}

  dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "ok_volcado");
  gtk_widget_show_all ( (GtkWidget *) dialog );
}

void
pg_user_created (GtkDialog *dialog, gint response_id, gpointer user_data)
{
  GtkAssistant *asistente;
  GtkWidget *page;
  gint current_page;

  asistente = (GtkAssistant *) gtk_builder_get_object (builder, "asistente" );

  current_page = gtk_assistant_get_current_page (asistente);
  page = gtk_assistant_get_nth_page (asistente, current_page);
  gtk_assistant_set_page_complete (asistente, page, TRUE);

  gtk_widget_hide ( (GtkWidget *)dialog);
}

void
create_db_user (GtkWidget *button, gpointer user_data)
{
  gchar *command;
  uid_t uid;

  GtkMessageDialog *dialog;

  GtkWidget *window;
  GtkWidget *button2;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *hbox;

  uid = getuid ();

  if (uid != 0)
    {
	  dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "error_gksu");
	  gtk_widget_show_all ( (GtkWidget *) dialog );

    }
  else
    {
	  local_user_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "local_pg_user"))));
	  user_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "user_pg"))));
	  pass_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "pass_pg"))));

	  if( strcmp (local_user_pg, "") == 0|| strcmp (user_pg, "") == 0 )
		{
		  dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "missing_data");
		  gtk_widget_show_all ( (GtkWidget *) dialog );
		}
	  else
		{
		  command = g_strdup_printf ("su - %s -c \"psql template1 -c \\\"CREATE USER %s ENCRYPTED PASSWORD \'%s\' CREATEDB NOCREATEUSER;\\\"\"",
									 local_user_pg, user_pg, pass_pg);
		  if ((system (command)) == 0)
			{
			  dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "ok_pg_user");
			  gtk_widget_show_all ( (GtkWidget *) dialog );
			}
		  else
			{
			  dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "error_pg_user");
			  gtk_widget_show_all ( (GtkWidget *) dialog );
			}
		}
	}
}

void
on_page_change (GtkAssistant *assistant, GtkWidget *page, gpointer user_data)
{
  gint current_page;

  current_page = gtk_assistant_get_current_page (assistant);

  if (current_page == 2)
	{
	  GtkEntry *ruta_sql = (GtkEntry *) gtk_builder_get_object (builder, "ruta_sql");

	  gtk_entry_set_text (ruta_sql, DATADIR);
	}

  if (current_page == 3)
	{
	  GtkAssistant *asistente;
	  GtkWidget *page;
	  gint current_page;

	  asistente = (GtkAssistant *) gtk_builder_get_object (builder, "asistente" );

	  current_page = gtk_assistant_get_current_page (asistente);
	  page = gtk_assistant_get_nth_page (asistente, current_page);
	  gtk_assistant_set_page_complete (asistente, page, TRUE);
	}
}

int
main (int argc, char *argv[])
{
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
