/*rizoma-config.c
*
*    Copyright (C) 2006 Rizoma Tecnologia Limitada <jonathan@rizoma.cl>
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

#include<libgnomeui/libgnomeui.h>
#include<gnome.h>

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

gboolean
get_db_user_info( GnomeDruidPage *page, GtkWidget *widget, gpointer user_data ) {

	local_user_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (pg_local_user)));
	host_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (pg_host)));
	user_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (pg_user)));
	pass_pg = g_strdup (gtk_entry_get_text (GTK_ENTRY (pg_pass)));

	return FALSE;
}

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

      g_signal_connect (G_OBJECT (button2), "clicked",
			G_CALLBACK (close_window), NULL);

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

	  g_signal_connect (G_OBJECT (button2), "clicked",
			    G_CALLBACK (close_window), NULL);

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

	  g_signal_connect (G_OBJECT (button2), "clicked",
			    G_CALLBACK (close_window), NULL);

	  gtk_widget_show_all (window);
	}
    }
}

int main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *druid, *page;
  GtkWidget *button;

  gtk_init (&argc, &argv);

  user = getenv("USER");

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Rizoma: Configuracion");

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (gtk_main_quit), NULL);

  druid = gnome_druid_new ();
  gtk_container_add (GTK_CONTAINER (window), druid);


  page = gnome_druid_page_edge_new_with_vals (GNOME_EDGE_START, FALSE, "Introduccion",
					      "\n\n\nCon el siguiente Wizard, podra configurar sus los usuarios, crear la base "
					      "de datos y asignar los datos iniciales para la configuracion de sus POS.\n\n"
					      "Para la configuracion de PostgreSQL se requiere accesso root por lo que es recomendable "
					      "correr esta aplicacion usando gksu\n\n",
					      NULL, NULL, NULL);

  gnome_druid_append_page (GNOME_DRUID (druid), GNOME_DRUID_PAGE (page));
  gnome_druid_set_page (GNOME_DRUID (druid), GNOME_DRUID_PAGE (page));

  g_signal_connect (G_OBJECT (page), "cancel",
		    G_CALLBACK (gtk_main_quit), NULL);

  /* The PostgreSQL page */

  page = gnome_druid_page_standard_new_with_vals ("Crear el usuario de PostgreSQL", NULL, NULL);
  gnome_druid_append_page (GNOME_DRUID (druid), GNOME_DRUID_PAGE (page));

  pg_local_user = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (pg_local_user), "postgres");
  gnome_druid_page_standard_append_item (GNOME_DRUID_PAGE_STANDARD (page), "Usuario local de PostgreSQL",
					 pg_local_user, "(generalmente es: postgres)");

  pg_host = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (pg_host), "127.0.0.1");
  gnome_druid_page_standard_append_item (GNOME_DRUID_PAGE_STANDARD (page), "El host donde esta ubicada la BD",
					 pg_host, "(generalmente es: 127.0.0.1)");

  pg_user = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (pg_user), user);
  gnome_druid_page_standard_append_item (GNOME_DRUID_PAGE_STANDARD (page), "Usuario para acceceder a la BD",
					 pg_user, "Se recomienda que sea el mismo usuario que ejecuta el POS");

  pg_pass = gtk_entry_new ();
  gnome_druid_page_standard_append_item (GNOME_DRUID_PAGE_STANDARD (page), "Contraseña para el accesso a la BD",
					 pg_pass, NULL);


  button = gtk_button_new_with_label ("Crear usuario");
  gnome_druid_page_standard_append_item (GNOME_DRUID_PAGE_STANDARD (page), "A continuacion se creara el usuario PostgreSQL",
					 button, NULL);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (create_db_user), NULL);

  g_signal_connect (G_OBJECT (page), "cancel",
		    G_CALLBACK (gtk_main_quit), NULL);

  g_signal_connect( G_OBJECT( page ), "next",
					G_CALLBACK( get_db_user_info ), (gpointer)page );

  /* The third page, the init data */
  page = gnome_druid_page_standard_new_with_vals ("Datos iniciales de la BD", NULL, NULL);
  gnome_druid_append_page (GNOME_DRUID (druid), GNOME_DRUID_PAGE (page));

  admin_user = gtk_entry_new ();
  gnome_druid_page_standard_append_item (GNOME_DRUID_PAGE_STANDARD (page), "Usuario administrador del POS",
					 admin_user, NULL);

  admin_pass = gtk_entry_new ();
  gnome_druid_page_standard_append_item (GNOME_DRUID_PAGE_STANDARD (page), "Contraseña del administrador del POS",
					 admin_pass, NULL);

  db_name = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (db_name), "rizoma");
  gnome_druid_page_standard_append_item (GNOME_DRUID_PAGE_STANDARD (page), "Nombre de la Base de datos",
					 db_name, "Este sera el nombre con el que se refirar a la BD");

  data_sql = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (data_sql), DATADIR);
  gnome_druid_page_standard_append_item (GNOME_DRUID_PAGE_STANDARD (page), "La ruta a los archivos SQL",
					 data_sql, g_strdup_printf ("Normalmente estan en: %s", DATADIR));


  button = gtk_button_new_with_label ("Volcar base de datos");
  gnome_druid_page_standard_append_item (GNOME_DRUID_PAGE_STANDARD (page), "Despues de este paso se habran ingresado las tablas mas algunos\n "
					 "datos iniciales a la base de datos, si no esta seguro comouniquese con su administrador.",
					 button, NULL);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (volcar_db), NULL);

  g_signal_connect (G_OBJECT (page), "cancel",
		    G_CALLBACK (gtk_main_quit), NULL);

  /* End Page */
  page = gnome_druid_page_edge_new_with_vals (GNOME_EDGE_FINISH, TRUE, "Fin",
					      "\n\nHemos creado la base de datos junto con usuario y contraseña ahora puede usar el POS."
					      "\n\nAl finalizar el Wizard se creara el archivo de configuracion para el POS.",
					      NULL, NULL, NULL);
  gnome_druid_append_page (GNOME_DRUID (druid), GNOME_DRUID_PAGE (page));


  g_signal_connect (G_OBJECT (page), "finish",
		    G_CALLBACK (create_config), NULL);

  g_signal_connect (G_OBJECT (page), "cancel",
		    G_CALLBACK (gtk_main_quit), NULL);

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
