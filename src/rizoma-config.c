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

#include<libpq-fe.h>

#include<gtk/gtk.h>

#include<glib/gstdio.h>

#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>

/* The Builder */
GtkBuilder *builder;

/* Server data info - Used when the user select client mode */
gchar *server_host;
gchar *server_port;
gchar *server_db_name;
gchar *server_db_user;
gchar *server_db_pass;
gboolean server_ssl;

gboolean local;
gboolean network;
gboolean client;

/* Store the info to access the PostgreSQL to create a pguser */
gchar *pg_port;
gchar *pg_user;
gchar *pg_pass;
gboolean pg_ssl;

/* User and pass to connect the DB */
gchar *user_db_name;
gchar *user_db_pass;

/* DB Info */
gchar *db_name;
gchar *admin_user;
gchar *admin_pass;

/* Path to SQL files */
gchar *sql_path;

void
create_config (GtkAssistant *asistente, gpointer data_user)
{
  GKeyFile *file;
  gchar *rizoma_path;
  //gchar *ssl;

  rizoma_path = g_strconcat(g_getenv("HOME"), "/.rizoma", NULL);

  file = g_key_file_new();

  if (local || (network && !client))
    {
      g_key_file_set_string (file, "RIZOMA", "DB_NAME", db_name);
      g_key_file_set_string (file, "RIZOMA", "USER", user_db_name);
      g_key_file_set_string (file, "RIZOMA", "PASSWORD", user_db_pass);
      g_key_file_set_string (file, "RIZOMA", "SERVER_HOST", "localhost");
      g_key_file_set_string (file, "RIZOMA", "PORT", pg_port);

      //ssl = pg_ssl ? "require" : "disable";
      g_key_file_set_string (file, "RIZOMA", "SSLMODE", "require");
    }
  else
    {
      g_key_file_set_string (file, "RIZOMA", "DB_NAME", server_db_name);
      g_key_file_set_string (file, "RIZOMA", "USER", server_db_user);
      g_key_file_set_string (file, "RIZOMA", "PASSWORD", server_db_pass);
      g_key_file_set_string (file, "RIZOMA", "SERVER_HOST", server_host);
      g_key_file_set_string (file, "RIZOMA", "PORT", server_port);

      //ssl = server_ssl ? "require" : "disable";
      g_key_file_set_string (file, "RIZOMA", "SSLMODE", "require");
    }

  g_key_file_set_string (file, "RIZOMA", "TEMP_FILES", "/tmp");
  g_key_file_set_string (file, "RIZOMA", "VALE_DIR", "/tmp");
  g_key_file_set_string (file, "RIZOMA", "VALE_COPY", "1");
  g_key_file_set_string (file, "RIZOMA", "VENDEDOR", "0");
  g_key_file_set_string (file, "RIZOMA", "MAQUINA", "1");
  g_key_file_set_string (file, "RIZOMA", "SPREADSHEET_APP", "gnumeric");
  g_key_file_set_string (file, "RIZOMA", "VENTA_DIRECTA", "0");
  g_key_file_set_string (file, "RIZOMA", "PRINT_COMMAND", "lpr");
  g_key_file_set_string (file, "RIZOMA", "IMPRESORA", "0");
  g_key_file_set_string (file, "RIZOMA", "PRINT_FACTURA", "0");
  g_key_file_set_string (file, "RIZOMA", "VALE_SELECTIVO", "NO");
  g_key_file_set_string (file, "RIZOMA", "VALE_CONTINUO", "0");
  g_key_file_set_string (file, "RIZOMA", "LOGO", "");
  g_key_file_set_string (file, "RIZOMA", "FULLSCREEN", "no");
  g_key_file_set_string (file, "RIZOMA", "CAJA", "0");
  g_key_file_set_string (file, "RIZOMA", "TRASPASO", "0");
  g_key_file_set_string (file, "RIZOMA", "SENCILLO_DIRECTO", "0");
  g_key_file_set_string (file, "RIZOMA", "GAVETA_DEV", "/dev/ttyUSB0");
  g_key_file_set_string (file, "RIZOMA", "ROPA", "0");
  g_key_file_set_string (file, "RIZOMA", "RECIBO_MOV_CAJA", "0");
  g_key_file_set_string (file, "RIZOMA", "RECIBO_TRASPASO", "0");
  g_key_file_set_string (file, "RIZOMA", "RECIBO_COMPRA", "0");
  g_key_file_set_string (file, "RIZOMA", "BARCODE_FRAGMENTADO", "0");
  g_key_file_set_string (file, "RIZOMA", "BAR_RANGO_PRODUCTO_A", "1");
  g_key_file_set_string (file, "RIZOMA", "BAR_RANGO_PRODUCTO_B", "1");
  g_key_file_set_string (file, "RIZOMA", "BAR_RANGO_CANTIDAD_A", "1");
  g_key_file_set_string (file, "RIZOMA", "BAR_RANGO_CANTIDAD_B", "1");
  g_key_file_set_string (file, "RIZOMA", "BAR_NUM_DECIMAL", "1");
  g_key_file_set_string (file, "RIZOMA", "MONTO_BASE_CAJA", "0");
  g_key_file_set_string (file, "RIZOMA", "MODO_INVENTARIO", "0");
  g_key_file_set_string (file, "RIZOMA", "PRECIO_DISCRECIONAL", "0");
  

  if (g_file_set_contents (rizoma_path, g_key_file_to_data (file, NULL, NULL), -1, NULL))
    {
      gtk_widget_show_all (GTK_WIDGET (gtk_builder_get_object (builder, "error_config_file")));
    }
  else
    {
      gtk_widget_show_all (GTK_WIDGET (gtk_builder_get_object (builder, "ok_config_file")));
    }

}

void
on_page_change (GtkAssistant *assistant, GtkWidget *page, gpointer user_data)
{
  gint current_page;

  local = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "radio_btn_local")));
  network = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "radio_btn_network")));
  client = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "radio_btn_client")));

  current_page = gtk_assistant_get_current_page (assistant);

  if (current_page == 1)
    {
      gtk_assistant_set_page_complete (assistant, page, TRUE);
    }
  else if (current_page == 2)
    {
      GtkNotebook *notebook = GTK_NOTEBOOK (gtk_builder_get_object (builder, "ntbk_data_ingress"));

      if (local || (network && !client))
        {
          gtk_notebook_set_current_page (notebook, 0);
        }
      else
        {
          gtk_notebook_set_current_page (notebook, 1);
        }
    }
  else if (current_page == 3)
    {
      if (local || (network && client))
        {
          gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object (builder, "sql_path")), DATADIR);
        }
      gtk_assistant_set_page_complete (assistant, page, TRUE);
    }
  else if (current_page == 4)
    {
      gtk_assistant_set_page_complete (assistant, page, TRUE);
    }
}

gint
forward_function (gint current_page, gpointer data)
{
  if (current_page == 2 && client && network)
    {
      return 4;
    }

  return current_page + 1;
}

int
main (int argc, char *argv[])
{
  GError *err = NULL;
  GtkAssistant *assistant;
  GtkWidget *page;

  gtk_init (&argc, &argv);

  builder = gtk_builder_new ();

  gtk_builder_add_from_file (builder, DATADIR"/ui/rizoma-config.ui", &err);

  if (err) 
    {
      g_error ("ERROR: %s\n", err->message);
      return -1;
    }

  gtk_builder_connect_signals (builder, NULL);

  assistant = GTK_ASSISTANT (gtk_builder_get_object (builder, "asistente"));

  page = gtk_assistant_get_nth_page (assistant, 0 );
  gtk_assistant_set_page_complete (assistant, page, TRUE);

  gtk_assistant_set_forward_page_func (assistant, forward_function, NULL, NULL);

  gtk_widget_show_all (GTK_WIDGET (assistant));

  gtk_main ();

  return 0;
}


void
on_radio_btn_type_network_toggled (GtkWidget *vbox, GtkToggleButton *button)
{
  gboolean active = gtk_toggle_button_get_active (button);

  gtk_widget_set_sensitive (vbox, active);
}

void
on_btn_conneciton_test_clicked (GtkAssistant *assistant)
{
  GtkWidget *wnd;
  GtkWidget *page = gtk_assistant_get_nth_page (assistant, gtk_assistant_get_current_page (assistant));

  PGconn *connection;
  ConnStatusType status;

  gchar *ssl;
  gchar *str_conn;

  server_host = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_client_host"))));
  server_port = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_client_port"))));
  server_db_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_client_db"))));
  server_db_user = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_client_user"))));
  server_db_pass = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "entry_client_pass"))));
  server_ssl = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "chk_btn_ssl")));

  ssl = server_ssl ? "require" : "disable";

  str_conn = g_strdup_printf ("host=%s port=%s dbname=%s user=%s password=%s sslmode=%s",
                              server_host, server_port, server_db_name, server_db_user, server_db_pass, ssl);

  connection = PQconnectdb (str_conn);
  g_free (str_conn);

  status = PQstatus (connection);

  switch (status)
    {
    case CONNECTION_OK:
      wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_server_conn_ok"));
      PQfinish (connection);
      gtk_assistant_set_page_complete (assistant, page, TRUE);
      break;
    case CONNECTION_BAD:
      wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_server_conn_bad"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (wnd), PQerrorMessage (connection));
      gtk_assistant_set_page_complete (assistant, page, FALSE);
      break;
    default: /* We cover all the other options that we do not need */
      break;
    }

  gtk_widget_show_all (wnd);
}

void
on_btn_create_pg_user_clicked (GtkAssistant *assistant)
{
  PGconn *connection;
  ConnStatusType status;
  PGresult *res;
  ExecStatusType status_res;

  gchar *ssl;
  gchar *str_conn;

  gchar *sql_query;

  GtkWidget *wnd;
  GtkWidget *page = gtk_assistant_get_nth_page (assistant, gtk_assistant_get_current_page (assistant));

  pg_user = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "pg_user"))));
  pg_pass = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "pg_pass"))));
  pg_port = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "pg_port"))));
  pg_ssl = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder, "chk_btn_ssl_pg")));

  user_db_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "user_db_name"))));
  user_db_pass = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "user_db_pass"))));

  ssl = pg_ssl ? "require" : "disable";

  if (g_str_equal (pg_user, "") || g_str_equal (pg_port, "") || g_str_equal (user_db_name, ""))
    {
      wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_missing_data"));
    }
  else
    {
      str_conn = g_strdup_printf ("host=localhost port=%s dbname=template1 user=%s password=%s sslmode=%s",
                                  pg_port, pg_user, pg_pass, ssl);

      connection = PQconnectdb (str_conn);
      g_free (str_conn);

      status = PQstatus (connection);

      switch (status)
        {
        case  CONNECTION_OK:
          if (g_str_equal (user_db_pass, ""))
            {
              sql_query = g_strdup_printf ("CREATE USER %s CREATEDB NOCREATEUSER", user_db_name);
            }
          else
            {
              sql_query = g_strdup_printf ("CREATE USER %s ENCRYPTED PASSWORD '%s' CREATEDB NOCREATEUSER", user_db_name, user_db_pass);
            }

          res = PQexec (connection, sql_query);
          status_res = PQresultStatus (res);

          if ((status_res != PGRES_COMMAND_OK) && (status_res != PGRES_TUPLES_OK))
            {
              wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_error_pg_user"));
              gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (wnd),
                                                        g_strdup_printf( "Error: %s\nMsg:%s",
                                                                         PQresStatus (status_res),
                                                                         PQresultErrorMessage (res)));
              gtk_assistant_set_page_complete (assistant, page, FALSE);
            }
          else
            {
              wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_ok_pg_user"));
              gtk_assistant_set_page_complete (assistant, page, TRUE);
            }
          break;
        case CONNECTION_BAD:
          wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_server_conn_bad"));
          gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (wnd), PQerrorMessage (connection));
          gtk_assistant_set_page_complete (assistant, page, FALSE);
          break;
        default:
          break;
        }
    }
  PQfinish (connection);

  gtk_widget_show_all (wnd);
}

void
on_btn_dump_data_clicked (GtkAssistant *assistant)
{
  gchar *str_conn;
  gchar *ssl;

  gchar *sql_query;
  gchar *content;

  PGconn *connection;
  PGresult *res;
  ConnStatusType status;
  ExecStatusType est;

  GtkWidget *wnd;
  GtkWidget *page = gtk_assistant_get_nth_page (assistant, gtk_assistant_get_current_page (assistant));
  GError *error = NULL;

  db_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "db_name"))));
  admin_user = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "admin_user"))));
  admin_pass = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "admin_pass"))));
  sql_path = g_strdup (gtk_entry_get_text (GTK_ENTRY (gtk_builder_get_object (builder, "sql_path"))));

  if (g_str_equal (db_name, "") || g_str_equal (admin_user, "") || g_str_equal (admin_pass, "") || g_str_equal (sql_path, ""))
    {
      gtk_widget_show_all (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_missing_data")));
      return;
    }

  ssl = pg_ssl ? "require" : "disable";

  str_conn = g_strdup_printf ("host=localhost port=%s dbname=template1 user=%s password=%s sslmode=%s",
                              pg_port, user_db_name, user_db_pass, ssl);

  connection = PQconnectdb (str_conn);
  g_free (str_conn);

  status = PQstatus (connection);

  if (status == CONNECTION_OK)
    {
      sql_query = g_strdup_printf ("DROP DATABASE IF EXISTS %s;", db_name);
      res = PQexec (connection, sql_query);
      g_free (sql_query);

      sql_query = g_strdup_printf ("CREATE DATABASE %s;", db_name);

      res = PQexec (connection, sql_query);
      g_free (sql_query);
      est = PQresultStatus (res);

      if ((est != PGRES_COMMAND_OK) && (est != PGRES_TUPLES_OK))
        {
          wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_error_create_db"));
          gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (wnd),
                                                    g_strdup_printf( "Error: %s\nMsg:%s",
                                                                     PQresStatus (est),
                                                                     PQresultErrorMessage (res)));
          gtk_widget_show_all (wnd);
          gtk_assistant_set_page_complete (assistant, page, FALSE);
          PQfinish (connection);
          return;
        }
    }
  else if (status == CONNECTION_BAD)
    {
      wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_error_create_db"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (wnd), PQerrorMessage (connection));
      gtk_assistant_set_page_complete (assistant, page, FALSE);
      gtk_widget_show_all (wnd);
      PQfinish (connection);
      return;
    }

  PQfinish (connection);

  str_conn = g_strdup_printf ("host=localhost port=%s dbname=%s user=%s password=%s sslmode=%s",
                              pg_port, db_name, user_db_name, user_db_pass, ssl);
  connection = PQconnectdb (str_conn);
  g_free (str_conn);

  status = PQstatus (connection);

  if (status == CONNECTION_BAD)
    {
      wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_server_conn_bad"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (wnd), PQerrorMessage (connection));
      gtk_assistant_set_page_complete (assistant, page, FALSE);
      gtk_widget_show_all (wnd);
      PQfinish (connection);
      return;
    }


  if (!g_file_get_contents (g_strdup_printf ("%s/rizoma.structure", sql_path), &content, NULL, &error))
    {
      wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_error_sql_data_schema"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (wnd), error->message);
      gtk_assistant_set_page_complete (assistant, page, FALSE);
      gtk_widget_show_all (wnd);
      PQfinish (connection);
      return;
    }
  else
    {
      res = PQexec (connection, content);
      est = PQresultStatus (res);

      if (est == PGRES_BAD_RESPONSE || est ==PGRES_NONFATAL_ERROR || est == PGRES_FATAL_ERROR)
        {
          wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_error_sql_create_schema"));
          gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (wnd), PQresultErrorMessage (res));
          gtk_assistant_set_page_complete (assistant, page, FALSE);
          gtk_widget_show_all (wnd);
          PQfinish (connection);
          return;
        }
    }

  g_free (content);
  content = NULL;

  if (!g_file_get_contents (g_strdup_printf ("%s/rizoma.initvalues", sql_path), &content, NULL, &error))
    {
      wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_error_sql_data_values"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (wnd), error->message);
      gtk_assistant_set_page_complete (assistant, page, FALSE);
      gtk_widget_show_all (wnd);
      PQfinish (connection);
      return;
    }
  else
    {
      res = PQexec (connection, content);
      est = PQresultStatus (res);

      if (est == PGRES_BAD_RESPONSE || est ==PGRES_NONFATAL_ERROR || est == PGRES_FATAL_ERROR)
        {
          wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_error_sql_insert_values"));
          gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (wnd), PQresultErrorMessage (res));
          gtk_assistant_set_page_complete (assistant, page, FALSE);
          gtk_widget_show_all (wnd);
          PQfinish (connection);
          return;
        }
    }

  sql_query = g_strdup_printf ("INSERT INTO users (id, usuario, passwd, rut, dv, nombre, apell_p, apell_m, fecha_ingreso, level )"
                               " VALUES (DEFAULT, '%s', md5('%s'), 0, 0, 'Administrador', '', '', NOW(), 0)",
                               admin_user, admin_pass);

  res = PQexec (connection, sql_query);
  g_free (sql_query);
  est = PQresultStatus (res);

  if (est == PGRES_BAD_RESPONSE || est ==PGRES_NONFATAL_ERROR || est == PGRES_FATAL_ERROR)
    {
      wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_error_create_admin_user"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (wnd), PQresultErrorMessage (res));
      gtk_assistant_set_page_complete (assistant, page, FALSE);
      gtk_widget_show_all (wnd);
      PQfinish (connection);
      return;
    }

  g_free (content);
  content = NULL;

  if (!g_file_get_contents (g_strdup_printf ("%s/funciones.sql", sql_path), &content, NULL, &error))
    {
      wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_error_create_plsql"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (wnd), error->message);
      gtk_assistant_set_page_complete (assistant, page, FALSE);
      gtk_widget_show_all (wnd);
      PQfinish (connection);
      return;
    }
  else
    {
      res = PQexec (connection, content);
      est = PQresultStatus (res);

      if (est == PGRES_BAD_RESPONSE || est ==PGRES_NONFATAL_ERROR || est == PGRES_FATAL_ERROR)
        {
          wnd = GTK_WIDGET (gtk_builder_get_object (builder, "wnd_error_create_plsql"));
          gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (wnd), PQresultErrorMessage (res));
          gtk_assistant_set_page_complete (assistant, page, FALSE);
          gtk_widget_show_all (wnd);
          PQfinish (connection);
          return;
        }
    }

  PQfinish (connection);

  gtk_widget_show_all (GTK_WIDGET (gtk_builder_get_object (builder, "wnd_ok_volcado")));
  gtk_assistant_set_page_complete (assistant, page, TRUE);
}
