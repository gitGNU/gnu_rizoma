/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
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
#include<libgksu.h>

#include<glib/gstdio.h>

#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>

/* The Builder */
GtkBuilder *builder;

gchar *local_user_pg;
gchar *host_pg;
gchar *user_pg;
gchar *pass_pg;
gchar *port_pg;

gchar *user_admin;
gchar *pass_admin;
gchar *name_db;
gchar *sql_data;

gchar *user;

void
create_config (GtkAssistant *asistente, gpointer data_user)
{
  GKeyFile *file;
  gchar *rizoma_path;
  GtkMessageDialog *dialog;

  rizoma_path = g_strconcat(g_getenv("HOME"), "/.rizoma", NULL);

  file = g_key_file_new();

  name_db = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "db_name"))));
  host_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "host_pg"))));
  port_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "port_pg"))));
  user_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "user_pg"))));
  pass_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "pass_pg"))));

  g_key_file_set_string (file, "DEFAULT", "DB_NAME", name_db);
  g_key_file_set_string (file, "DEFAULT", "USER", user_pg);
  g_key_file_set_string (file, "DEFAULT", "PASSWORD", pass_pg);
  g_key_file_set_string (file, "DEFAULT", "SERVER_HOST", host_pg);
  g_key_file_set_string (file, "DEFAULT", "PORT", port_pg);
  g_key_file_set_string (file, "DEFAULT", "TEMP_FILES", "/tmp");
  g_key_file_set_string (file, "DEFAULT", "VALE_DIR", "/tmp");
  g_key_file_set_string (file, "DEFAULT", "VALE_COPY", "1");
  g_key_file_set_string (file, "DEFAULT", "VENDEDOR", "1");
  g_key_file_set_string (file, "DEFAULT", "MAQUINA", "1");
  g_key_file_set_string (file, "DEFAULT", "VENTA_DIRECTA", "0");
  g_key_file_set_string (file, "DEFAULT", "SSLMODE", "require");
  g_key_file_set_string (file, "DEFAULT", "PRINT_COMMAND", "lpr");
  g_key_file_set_string (file, "DEFAULT", "LOGO", "");
  g_key_file_set_string (file, "DEFAULT", "FULLSCREEN", "no");


  if (g_file_set_contents (rizoma_path, g_key_file_to_data (file, NULL, NULL), -1, NULL))
    {
      dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "error_config_file");

      gtk_widget_show_all ( (GtkWidget *)dialog);
    }
  else
    {
      g_remove (g_strdup_printf ("%s/.pgpass", g_getenv ("HOME")));
      dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "ok_config_file");

      gtk_widget_show_all ( (GtkWidget *)dialog);
    }

}


void
dialog_close (GtkDialog *dialog, gint response_id, gpointer user_data)
{
  if (response_id == GTK_RESPONSE_CLOSE || response_id == GTK_RESPONSE_NO)
    {
      gtk_widget_hide ( (GtkWidget *) dialog);
    }
  else if (response_id == GTK_RESPONSE_YES || response_id == GTK_RESPONSE_OK)
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

gboolean
run_gksu_command (gchar *command)
{
  GksuContext *context = NULL;
  GError *error = NULL;

  context = gksu_context_new ();
  gksu_context_set_user (context, "root");
  gksu_context_set_description (context, "POS Rizoma Comercio - Configuración");
  //gksu_context_set_debug (context, TRUE);

  gksu_context_set_command (context, command);

  gksu_su_full (context, NULL, NULL, NULL, NULL, &error);
  gksu_context_free (context);
  if (error != NULL )
    {
      g_print ("error: %s\n", error->message);
      return FALSE;
    }
  else
    {
      return TRUE;
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

  if( g_ascii_strcasecmp (user_admin, "") == 0 || g_ascii_strcasecmp (pass_admin, "") == 0 || g_ascii_strcasecmp (name_db, "") == 0 || g_ascii_strcasecmp (sql_data, "") == 0 )
    {
      dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "missing_data");
      gtk_widget_show_all ( (GtkWidget *) dialog );
      return;
    }

  command = g_strdup_printf ("su - %s -c \\\"dropdb %s; createdb -O %s %s;\\\"",
                             local_user_pg, name_db, user_pg, name_db);
  if (!run_gksu_command (command))
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

      command = g_strdup_printf ("psql -f %s -d %s", path, name_db);
      if (system (command) != 0)
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

      command = g_strdup_printf ("psql -f %s -d %s", path, name_db);
      if (system (command) != 0)
        {
          dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "error_sql_insert_values");
          gtk_widget_show_all ( (GtkWidget *) dialog );

          return;
        }
    }

  sql = g_strdup_printf ("INSERT INTO users (id, usuario, passwd, rut, dv, nombre, apell_p, apell_m, fecha_ingreso, level )"
                         " VALUES (DEFAULT, '%s', md5('%s'), 0, 0, 'Administrador', '', '', NOW(), 0)",
                         user_admin, pass_admin);

  command = g_strdup_printf ("psql -f %s -d %s -c \"%s\"", path, name_db, sql);
  if (system (command) != 0)
    {
      dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "error_create_admin_user");
      gtk_widget_show_all ( (GtkWidget *) dialog );

      return;
    }

  path = g_strdup_printf ("%s/funciones.sql", sql_data);
  command = g_strdup_printf ("psql -f %s -d %s", path, name_db);
  if (system (command) != 0)
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
  gchar *pgpass_string;
  GtkMessageDialog *dialog;

  local_user_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "local_pg_user"))));
  host_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "host_pg"))));
  port_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "port_pg"))));
  user_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "user_pg"))));
  pass_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "pass_pg"))));

  if( g_ascii_strcasecmp (local_user_pg, "") == 0|| g_ascii_strcasecmp (user_pg, "") == 0 )
    {
      dialog = (GtkMessageDialog *) gtk_builder_get_object (builder, "missing_data");
      gtk_widget_show_all ( (GtkWidget *) dialog );
    }
  else
    {
      command = g_strdup_printf ("su %s -c \\\"psql template1 -c \\\\\\\"CREATE USER %s ENCRYPTED PASSWORD \'%s\' CREATEDB NOCREATEUSER;\\\\\\\"\\\"",
                                 local_user_pg, user_pg, pass_pg);

      pgpass_string = g_strdup_printf ("%s:%s:*:%s:%s", host_pg, port_pg, user_pg, pass_pg);

      if (run_gksu_command (command) && g_file_set_contents (g_strdup_printf ("%s/.pgpass", g_getenv ("HOME")), pgpass_string, -1, NULL))
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

  asistente = (GtkAssistant *) gtk_builder_get_object (builder, "asistente");

  page = gtk_assistant_get_nth_page (asistente, 0 );
  gtk_assistant_set_page_complete (asistente, page, TRUE);

  gtk_widget_show_all ( (GtkWidget *) asistente);

  gtk_main ();

  return 0;
}
