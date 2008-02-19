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

#include"../pixmaps/tux.xpm"

#include"../config.h"

#include"tipos.h"

#include"main.h"
#include"ventas.h"
#include"administracion_productos.h"
#include"ventas_stats.h"
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

GtkWidget *treeview;

GtkWidget *hour_label;

// Definimos las variables para obtener el ancho y alto del screen
gint screen_width;
gint screen_height;

void
SelectMenu (GtkWidget *widget, gpointer data)
{
  gchar *name = (gchar *)data;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  gint i;

  if (strcmp (name, "Informes") != 0)
	{
	  if (calendar_from != NULL || calendar_to != NULL)
		{
		  if (calendar_from != NULL)
			gtk_widget_destroy (calendar_from);
		  if (calendar_to != NULL)
			gtk_widget_destroy (calendar_to);
		}
	}

  for (i = 0; i < sizeof modulos / sizeof modulos[0]; i++)
	if (strcmp (modulos[i].name, name) == 0)
	  gtk_tree_selection_select_path (selection,
									  gtk_tree_path_new_from_string
									  (g_strdup_printf ("%d", i)));
}

void
Salir (GtkWidget *widget, gpointer data)
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
  /* 	{"factura_size", &dimension->factura_size}, */
  /* 	{"factura_cliente", &dimension->factura_cliente}, */
  /* 	{"factura_address", &dimension->factura_address}, */
  /* 	{"factura_giro", &dimension->factura_giro}, */
  /* 	{"factura_rut", &dimension->factura_rut}, */
  /* 	{"factura_comuna", &dimension->factura_comuna}, */
  /* 	{"factura_fono", &dimension->factura_fono}, */
  /* 	{"factura_subtotal", &dimension->factura_subtotal}, */
  /* 	{"factura_iva", &dimension->factura_iva}, */
  /* 	{"factura_total", &dimension->factura_total}, */
  /* 	{"factura_detail_begin", &dimension->factura_detail_begin}, */
  /* 	{"factura_detail_end", &dimension->factura_detail_end}, */
  /* 	{"factura_codigo", &dimension->factura_codigo}, */
  /* 	{"factura_descripcion", &dimension->factura_descripcion}, */
  /* 	{"factura_cantidad", &dimension->factura_cantidad}, */
  /* 	{"factura_precio_uni", &dimension->factura_precio_uni}, */
  /* 	{"factura_precio_total", &dimension->factura_precio_total}, */
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

  login_window = (GtkWindow *) gtk_builder_get_object (builder, "login_window");

  profiles = g_key_file_get_groups (key_file, NULL);
  g_key_file_free (key_file);

  model = gtk_list_store_new (1,
  							  G_TYPE_STRING
  							  );

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

	  if (strcmp (value, "Informes") != 0)
		if (calendar_from != NULL || calendar_to != NULL)
		  {
			if (calendar_from != NULL)
			  {
				gtk_widget_destroy (calendar_from);
				calendar_from = NULL;
			  }
			if (calendar_to != NULL)
			  {
				gtk_widget_destroy (calendar_to);
				calendar_to = NULL;
			  }
		  }

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

  if (calendar_from != NULL || calendar_to != NULL)
	{
	  if (calendar_from != NULL)
		{
		  gtk_widget_destroy (calendar_from);
		  calendar_from = NULL;
		}
	  if (calendar_to != NULL)
		{
		  gtk_widget_destroy (calendar_to);
		  calendar_to = NULL;
		}
	}

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
  MainBox *module_box = (MainBox *) g_malloc (sizeof (MainBox*));

  GtkAccelGroup *accel_generales;

  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *image;
  gint i;

  GtkListStore *store;
  GtkTreeIter iter;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *columna;
  GtkTreeSelection *selection;

  GtkWidget *scroll;

  module_box->new_box = NULL;

  venta = (Venta *) g_malloc (sizeof (Venta));
  venta->header = NULL;
  venta->products = NULL;
  venta->window = NULL;

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

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (main_window),
						"Rizoma: " PACKAGE_VERSION);

  gtk_window_set_position (GTK_WINDOW (main_window),
						   GTK_WIN_POS_CENTER_ALWAYS);

  gtk_window_set_decorated (GTK_WINDOW (main_window), FALSE);
  gtk_widget_show (main_window);
  gtk_window_unmaximize (GTK_WINDOW (main_window));
  gtk_window_set_resizable (GTK_WINDOW (main_window), FALSE);

  g_signal_connect (G_OBJECT (main_window), "delete_event",
					G_CALLBACK (Question), NULL);

  accel_generales = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (main_window), accel_generales);

  /*
   * En el caso de usemos una ventan de 800x600 la configuracion sigue abajo
   */
  if (solo_venta == FALSE)
	{
	  /*
		Usamos una ventana de 800x600
	  */
	  gtk_widget_set_size_request (main_window,
								   MAIN_WINDOW_WIDTH,
								   MAIN_WINDOW_HEIGHT);

	  vbox = gtk_vbox_new (FALSE, 0);
	  gtk_container_add (GTK_CONTAINER (main_window), vbox);
	  gtk_widget_show (vbox);

	  hbox = gtk_hbox_new (FALSE, 0);
	  gtk_widget_show (hbox);
	  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	  /*
		button = gtk_button_new_from_stock (GTK_STOCK_QUIT);
		gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
		gtk_widget_show (button);

		g_signal_connect (G_OBJECT (button), "clicked",
		G_CALLBACK (gtk_main_quit), NULL);
	  */

	  frame = gtk_frame_new ("Rizoma Comercio");
	  gtk_widget_set_size_request (frame, MAIN_FRAME_WIDTH, MAIN_FRAME_HEIGHT);
	  gtk_widget_show (frame);
	  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	  hbox = gtk_hbox_new (FALSE, 0);
	  gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
	  gtk_container_add (GTK_CONTAINER (frame), hbox);
	  gtk_widget_show (hbox);

	  vbox = gtk_vbox_new (FALSE, 0);
	  gtk_widget_set_size_request (vbox,
								   MODULES_TREE_WIDTH,
								   MODULES_TREE_HEIGHT);
	  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	  gtk_widget_show (vbox);

	  store = gtk_list_store_new (3,
								  G_TYPE_STRING,
								  G_TYPE_INT,
								  G_TYPE_INT);

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
		  else if ((user_data->level != 0) &&
				   (modulos[i].normal_user == TRUE))
			{
			  gtk_list_store_append (store, &iter);
			  gtk_list_store_set (store, &iter,
								  0, modulos[i].name,
								  1, 15000,
								  2, 1000,
								  -1);
			}
		}

	  scroll = gtk_scrolled_window_new (NULL, NULL);
	  gtk_widget_show (scroll);
	  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
									  GTK_POLICY_AUTOMATIC,
									  GTK_POLICY_AUTOMATIC);
	  gtk_widget_set_size_request (scroll,
								   MODULES_TREE_WIDTH,
								   MODULES_TREE_HEIGHT);

	  gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 0);

	  treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	  gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview), 0);
	  gtk_widget_set_size_request (GTK_WIDGET (treeview),
								   MODULES_TREE_WIDTH,
								   MODULES_TREE_HEIGHT);
	  gtk_container_add (GTK_CONTAINER (scroll), treeview);
	  gtk_widget_show (treeview);

	  gchar *logo_path = rizoma_get_value("LOGO");
	  if (logo_path == NULL)
		image = Image (main_window, tux_xpm);
	  else
		image = gtk_image_new_from_file(logo_path);

	  gtk_widget_show (image);
	  gtk_box_pack_start (GTK_BOX (vbox), image, FALSE, FALSE, 0);


	  /*
		Todo dentro de hbox no se vera !!!
		Y es solo para el usuario admin
	  */
	  if (user_data->level == 0)
		{
		  hbox2 = gtk_hbox_new (FALSE, 0);
		  gtk_box_pack_end (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
		  gtk_widget_set_size_request (hbox2, 0, 0);
		  gtk_widget_show (hbox2);

		  button = gtk_button_new ();
		  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
		  gtk_widget_set_size_request (button, 0, 0);
		  gtk_widget_show (button);

		  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);

		  g_signal_connect (G_OBJECT (button), "clicked",
							G_CALLBACK (VentanaIngreso), NULL);

		  gtk_widget_add_accelerator (button, "clicked",
									  accel_generales,
									  GDK_F7, GDK_LOCK_MASK,
									  GTK_ACCEL_VISIBLE);

		  button = gtk_button_new ();
		  gtk_box_pack_end (GTK_BOX (hbox2), button,
							FALSE, FALSE, 0);
		  gtk_widget_set_size_request (button, 0, 0);
		  gtk_widget_show (button);

		  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);

		  g_signal_connect (G_OBJECT (button), "clicked",
							G_CALLBACK (VentanaEgreso), NULL);

		  gtk_widget_add_accelerator (button, "clicked",
									  accel_generales,
									  GDK_F6, GDK_LOCK_MASK,
									  GTK_ACCEL_VISIBLE);

		  for (i = 0; i < sizeof (modulos) / sizeof (modulos[0]); i++)
			{
			  button = gtk_button_new ();
			  gtk_box_pack_end (GTK_BOX (hbox2), button,
								FALSE, FALSE, 0);
			  gtk_widget_set_size_request (button, 0, 0);
			  gtk_widget_show (button);

			  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);

			  g_signal_connect (G_OBJECT (button), "clicked",
								G_CALLBACK (SelectMenu),
								(gpointer)modulos[i].name);

			  gtk_widget_add_accelerator (button, "clicked",
										  accel_generales,
										  modulos[i].accel_key,
										  GDK_LOCK_MASK,
										  GTK_ACCEL_VISIBLE);
			}
		}

	  hour_label = gtk_label_new ("");
	  gtk_box_pack_end (GTK_BOX (vbox), hour_label,
						FALSE, FALSE, 0);
	  gtk_widget_show (hour_label);

	  date_label = show_date ();
	  gtk_box_pack_end (GTK_BOX (vbox), date_label,
						FALSE, FALSE, 0);
	  gtk_widget_show (date_label);

	  RefreshTime (NULL);

	  module_box->main_box = gtk_vbox_new (FALSE, 0);
	  gtk_box_pack_start (GTK_BOX (hbox),
						  module_box->main_box,
						  FALSE, FALSE, 0);
	  gtk_widget_set_size_request
		(GTK_WIDGET (module_box->main_box),
		 MODULE_BOX_WIDTH-5,
		 MODULE_BOX_HEIGHT);

	  gtk_widget_show (module_box->main_box);


	  GTK_WIDGET_UNSET_FLAGS (treeview, GTK_CAN_FOCUS);

	  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

	  g_signal_connect (G_OBJECT (selection), "changed",
						G_CALLBACK (show_selected),
						(gpointer)module_box);

	  gtk_tree_selection_select_path (selection,
									  gtk_tree_path_new_from_string ("0"));

	  renderer = gtk_cell_renderer_text_new ();
	  columna = gtk_tree_view_column_new_with_attributes ("Menu",
														  renderer,
														  "text", 0,
														  "size", 1,
														  "weight", 2,
														  NULL);

	  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), columna);

	  gtk_tree_selection_select_path (selection,
									  gtk_tree_path_new_from_string ("0"));

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
	  gtk_widget_set_size_request (main_window, 640, 480);

	  vbox = gtk_vbox_new (FALSE, 0);
	  gtk_container_add (GTK_CONTAINER (main_window), vbox);
	  gtk_widget_show (vbox);

	  hbox = gtk_hbox_new (FALSE, 0);
	  gtk_widget_show (hbox);
	  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	  frame = gtk_frame_new ("Rizoma Comercio");
	  gtk_widget_set_size_request (frame, MAIN_FRAME_WIDTH, MAIN_FRAME_HEIGHT);
	  gtk_widget_show (frame);
	  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	  module_box->main_box = gtk_vbox_new (FALSE, 0);
	  gtk_box_pack_start (GTK_BOX (hbox), module_box->main_box, FALSE, FALSE, 0);
	  gtk_widget_set_size_request (GTK_WIDGET (module_box->main_box), MODULE_LITTLE_BOX_WIDTH, MODULE_LITTLE_BOX_HEIGHT);
	  gtk_widget_show (module_box->main_box);

	  modulos[0].func(module_box);

	  hbox = gtk_hbox_new (FALSE, 0);
	  gtk_box_pack_end (GTK_BOX (module_box->main_box), hbox, FALSE, FALSE, 0);
	  gtk_widget_set_size_request (hbox, 0, 0);
	  gtk_widget_show (hbox);

	  button = gtk_button_new_with_label ("0");
	  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	  gtk_widget_set_size_request (button, 0, 0);
	  gtk_widget_show (button);

	  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);

	  g_signal_connect (G_OBJECT (button), "clicked",
						G_CALLBACK (show_selected_in_button), (gpointer)module_box);

	  gtk_widget_add_accelerator (button, "clicked",
								  accel_generales,
								  GDK_F1, GDK_LOCK_MASK,
								  GTK_ACCEL_VISIBLE);

	  /* Solo el admnistrador tiene accesso */

	  if (user_data->level == 0)
		{
		  button = gtk_button_new_with_label ("1");
		  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
		  gtk_widget_set_size_request (button, 0, 0);
		  gtk_widget_show (button);

		  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);

		  g_signal_connect (G_OBJECT (button), "clicked",
							G_CALLBACK (show_selected_in_button),
							(gpointer)module_box);

		  gtk_widget_add_accelerator (button, "clicked",
									  accel_generales,
									  GDK_F2, GDK_LOCK_MASK,
									  GTK_ACCEL_VISIBLE);

		  button = gtk_button_new_with_label ("2");
		  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
		  gtk_widget_set_size_request (button, 0, 0);
		  gtk_widget_show (button);

		  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);

		  g_signal_connect (G_OBJECT (button), "clicked",
							G_CALLBACK (show_selected_in_button),
							(gpointer)module_box);

		  gtk_widget_add_accelerator (button, "clicked",
									  accel_generales,
									  GDK_F3, GDK_LOCK_MASK,
									  GTK_ACCEL_VISIBLE);

		  button = gtk_button_new_with_label ("3");
		  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
		  gtk_widget_set_size_request (button, 0, 0);
		  gtk_widget_show (button);

		  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);

		  g_signal_connect (G_OBJECT (button), "clicked",
							G_CALLBACK (show_selected_in_button),
							(gpointer)module_box);

		  gtk_widget_add_accelerator (button, "clicked",
									  accel_generales,
									  GDK_F4, GDK_LOCK_MASK,
									  GTK_ACCEL_VISIBLE);
		}
	}

  button = gtk_button_new_with_label ("4");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_size_request (button, 0, 0);
  gtk_widget_show (button);

  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);

  g_signal_connect (G_OBJECT (button), "clicked",
					G_CALLBACK (show_selected_in_button),
					(gpointer)module_box);

  gtk_widget_add_accelerator (button, "clicked",
							  accel_generales,
							  GDK_F12, GDK_LOCK_MASK,
							  GTK_ACCEL_VISIBLE);


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

  //  gtk_timeout_add (1000, RefreshTime, (gpointer)hour_label);

  /*
   * Usamos el reloj solo si estamos en una resolucion de 800x600 o mayor
   */
  if (solo_venta == FALSE)
	gtk_timeout_add (1000, RefreshTime, NULL);

  gtk_main();

}

void
SendCursorTo (GtkWidget *widget, gpointer data)
{
  GtkWindow *window = GTK_WINDOW (gtk_widget_get_toplevel ((GtkWidget *)data));
  GtkWidget *destiny = (GtkWidget *) data;

  gtk_window_set_focus (window, destiny);
}

void
KillQuestionWindow (GtkWidget *widget, gpointer data)
{
  if (window != NULL)
	gtk_widget_destroy (window);
  window = NULL;

  gtk_widget_set_sensitive (main_window, TRUE);

  current_menu = last_menu;
  gtk_tree_selection_select_path
	(gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)),
	 gtk_tree_path_new_from_string (g_strdup_printf ("%d", last_menu)));

}

void
Question (MainBox *module_box)
{
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *image;
  GtkWidget *button;

  gtk_widget_set_sensitive (main_window, FALSE);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Confirmación de Salida");
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER_ALWAYS);
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  gtk_widget_set_size_request (window, 550, 130);
  gtk_window_present (GTK_WINDOW (window));
  //  gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (main_window));
  gtk_widget_show (window);

  g_signal_connect (G_OBJECT (window), "destroy",
					G_CALLBACK (KillQuestionWindow), NULL);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 3);

  label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (label),
						"<span size=\"20000\">Â¿Realmente desea salir del sistema?</span>");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_NO);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
					G_CALLBACK (KillQuestionWindow), (gpointer)module_box);

  button = gtk_button_new_from_stock (GTK_STOCK_YES);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
					G_CALLBACK (Salir), NULL);

  gtk_window_set_focus (GTK_WINDOW (window), button);

}

gchar *
PutPoints (gchar *number)
{
  gchar *alt = g_malloc0 (15), *alt3 = g_malloc0 (15);
  int len;
  gint points;
  gint i, unidad = 0, point = 0;

  if (number == NULL)
	return "";

  len = strlen (number);

  if (len <= 3)
	return number;

  if ((len % 3) != 0)
	points = len / 3;
  else
	points = (len / 3) - 1;

  for (i = len; i >= 0; i--)
	{
	  if (unidad == 3 && point < points && number[i] != '.' && number[i] != ',')
		{
		  g_snprintf (alt, 15, ".%c%s", number[i], alt3);
		  unidad = 0;
		  point++;
		}
	  else
		g_snprintf (alt, 15, "%c%s", number[i], alt3);

	  strcpy (alt3, alt);

	  unidad++;
	}

  return alt;
}

gchar *
CutPoints (gchar *number_points)
{
  gint len = strlen (number_points);
  gchar *number = g_malloc0 (len);
  gint i, o = 0;

  if (strcmp (number_points, "") == 0)
	return number_points;

  for (i = 0; i <= len; i++)
	if (number_points[i] != '.')
	  number[o++] = number_points[i];

  g_free (number_points);

  return number;
}

GtkWidget *
Image (GtkWidget * win, char **nombre)
{

  GdkColormap *colormap;
  GdkBitmap *mask;
  GdkPixmap *pixmap;
  GtkWidget *pixmapw;

  colormap = gtk_widget_get_colormap (win);
  pixmap =
	gdk_pixmap_colormap_create_from_xpm_d (win->window, colormap, &mask, NULL,
										   nombre);
  pixmapw = gtk_image_new_from_pixmap (pixmap, mask);

  return pixmapw;
}
