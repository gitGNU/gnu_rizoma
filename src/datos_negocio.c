/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
       c-indentation-style: gnu -*- */
/*datos_negocio.c
*
*    Copyright (C) 2004 Rizoma Tecnologia Limitada <info@rizoma.cl>
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

#include<string.h>

#include"tipos.h"
#include"postgres-functions.h"
#include"errors.h"
#include"datos_negocio.h"

GtkWidget *razon_red;
GtkWidget *razon_social;
char *razon_social_value = NULL;
GtkWidget *rut_red;
GtkWidget *rut;
char *rut_value = NULL;
GtkWidget *nombre_red;
GtkWidget *nombre_fantasia;
char *nombre_fantasia_value = NULL;
GtkWidget *fono_red;
GtkWidget *fono;
char *fono_value = NULL;
GtkWidget *direccion_red;
GtkWidget *direccion;
char *direccion_value = NULL;
GtkWidget *comuna_red;
GtkWidget *comuna;
char *comuna_value = NULL;
GtkWidget *ciudad_red;
GtkWidget *ciudad;
char *ciudad_value = NULL;
GtkWidget *fax_red;
GtkWidget *fax;
char *fax_value = NULL;
GtkWidget *giro_red;
GtkWidget *giro;
char *giro_value = NULL;
GtkWidget *at_red;
GtkWidget *at;
char *at_value = NULL;

void
refresh_labels (void)
{
  /* We get all the new data */

  get_datos ();

  /* We refresh all labesl */

  if (razon_social_value != NULL)
    gtk_label_set (GTK_LABEL (razon_red), "*Razón Social");
  else
    gtk_label_set_markup (GTK_LABEL (razon_red),
			  "<span color=\"red\">*Razón Social</span>");

  if (rut_value != NULL)
    gtk_label_set (GTK_LABEL (rut_red), "*Rut");
  else
    gtk_label_set_markup (GTK_LABEL (rut_red),
			  "<span color=\"red\">*Rut</span>");
  if (nombre_fantasia_value != NULL)
    gtk_label_set (GTK_LABEL (nombre_red), "Nombre de Fantasia");
  else
    gtk_label_set_markup (GTK_LABEL (nombre_red),
			  "<span color=\"red\">Nombre de Fantasia</span>");

  if (direccion_value != NULL)
    gtk_label_set (GTK_LABEL (direccion_red), "*Dirección");
  else
    gtk_label_set_markup (GTK_LABEL (direccion_red),
			  "<span color=\"red\">*Dirección</span>");
  if (comuna_value != NULL)
    gtk_label_set (GTK_LABEL (comuna_red), "*Comuna");
  else
    gtk_label_set_markup (GTK_LABEL (comuna_red),
			  "<span color=\"red\">*Comuna</span>");

  if (ciudad_value != NULL)
    gtk_label_set (GTK_LABEL (ciudad_red), "*Ciudad");
  else
    gtk_label_set_markup (GTK_LABEL (ciudad_red),
			  "<span color=\"red\">*Ciudad</span>");

  if (fono_value != NULL)
    gtk_label_set (GTK_LABEL (fono_red), "*Fono");
  else
    gtk_label_set_markup (GTK_LABEL (fono_red),
			  "<span color=\"red\">*Fono</span>");

  if (fax_value != NULL)
    gtk_label_set (GTK_LABEL (fax_red), "Fax");
  else
    gtk_label_set_markup (GTK_LABEL (fax_red),
			  "<span color=\"red\">Fax</span>");

  if (giro_value != NULL)
    gtk_label_set (GTK_LABEL (giro_red), "*Giro");
  else
    gtk_label_set_markup (GTK_LABEL (giro_red),
			  "<span color=\"red\">*Giro</span>");

  if (at_value != NULL)
    gtk_label_set (GTK_LABEL (at_red), "A.T.");
  else
    gtk_label_set_markup (GTK_LABEL (at_red),
			  "<span color=\"red\">A.T.</span>");
}

void
SaveDatosNegocio (GtkWidget *widget, gpointer data)
{
  PGresult *res;

  razon_social_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (razon_social)));
  rut_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (rut)));
  nombre_fantasia_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (nombre_fantasia)));
  fono_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (fono)));
  direccion_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (direccion)));
  comuna_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (comuna)));
  ciudad_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (ciudad)));
  fax_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (fax)));
  giro_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (giro)));
  at_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (at)));

  res = EjecutarSQL ("SELECT * FROM negocio");

  if (PQntuples (res) == 0)
    {
      res = EjecutarSQL
	(g_strdup_printf
	 ("INSERT INTO negocio (razon_social, rut, nombre, fono, fax, direccion, comuna, ciudad, giro, at) "
	  "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')", razon_social_value, rut_value, nombre_fantasia_value,
	  fono_value, fax_value, direccion_value, comuna_value, ciudad_value, giro_value, at_value));
    }
  else
    {
      res = EjecutarSQL
	(g_strdup_printf
	 ("UPDATE negocio SET razon_social='%s', rut='%s', nombre='%s', fono='%s', fax='%s', "
	  "direccion='%s', comuna='%s', ciudad='%s', giro='%s', at='%s'", razon_social_value, rut_value, nombre_fantasia_value,
	  fono_value, fax_value, direccion_value, comuna_value, ciudad_value, giro_value, at_value));
    }

  if (res != NULL)
    ExitoMSG (razon_social, "Los datos se actualizaron exitosamente");
  else
    AlertMSG (razon_social, "Se produjo un error al intentar actualizar los datos!");

  refresh_labels ();
}

int
get_datos (void)
{
  PGresult *res;

  res = EjecutarSQL ("SELECT razon_social, rut, nombre, fono, fax, direccion, comuna, ciudad, giro, at "
		     "FROM negocio");

  if (PQntuples (res) != 1)
    return -1;

  if (strcmp (PQgetvalue (res, 0, 0), "") != 0)
    razon_social_value = PQgetvalue (res, 0, 0);
  else
    razon_social_value = NULL;
  if (strcmp (PQgetvalue (res, 0, 1), "") != 0)
    rut_value = PQgetvalue (res, 0, 1);
  else
    rut_value = NULL;
  if (strcmp (PQgetvalue (res, 0, 2), "") != 0)
    nombre_fantasia_value = PQgetvalue (res, 0, 2);
  else
    nombre_fantasia_value = NULL;
  if (strcmp (PQgetvalue (res, 0, 3), "") != 0)
    fono_value = PQgetvalue (res, 0, 3);
  else
    fono_value = NULL;
  if (strcmp (PQgetvalue (res, 0, 4), "") != 0)
    fax_value = PQgetvalue (res, 0, 4);
  else
    fax_value = NULL;
  if (strcmp (PQgetvalue (res, 0, 5), "") != 0)
    direccion_value = PQgetvalue (res, 0, 5);
  else
    direccion_value = NULL;
  if (strcmp (PQgetvalue (res, 0, 6), "") != 0)
    comuna_value = PQgetvalue (res, 0, 6);
  else
    comuna_value = NULL;
  if (strcmp (PQgetvalue (res, 0, 7), "") != 0)
    ciudad_value = PQgetvalue (res, 0, 7);
  else
    ciudad_value = NULL;
  if (strcmp (PQgetvalue (res, 0, 8), "") != 0)
    giro_value = PQgetvalue (res, 0, 8);
  else
    giro_value = NULL;
  if (strcmp (PQgetvalue (res, 0, 9), "") != 0)
    at_value = PQgetvalue (res, 0, 9);
  else
    at_value = NULL;

  return 1;
}

void
datos_box (GtkWidget *main_box)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *box;
  GtkWidget *frame;
  GtkWidget *button;

  get_datos ();

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (main_box), vbox, FALSE, FALSE, 3);
  gtk_widget_show (vbox);

  frame = gtk_frame_new ("Datos del Negocio");
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 3);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  box = gtk_vbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  gtk_widget_show (box);
  razon_red = gtk_label_new ("*Razón Social");
  gtk_widget_show (razon_red);
  gtk_box_pack_start (GTK_BOX (box), razon_red, FALSE, FALSE, 0);
  razon_social = gtk_entry_new_with_max_length (200);

  if (razon_social_value != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (razon_social), razon_social_value);
      //      g_free (razon_social_value);
    }
  else
    gtk_label_set_markup (GTK_LABEL (razon_red),
			  "<span color=\"red\">*Razón Social</span>");

  gtk_widget_set_size_request (razon_social, 150, -1);
  gtk_box_pack_start (GTK_BOX (box), razon_social, FALSE, FALSE, 0);
  gtk_widget_show (razon_social);

  box = gtk_vbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  gtk_widget_show (box);
  rut_red = gtk_label_new ("*Rut");
  gtk_widget_show (rut_red);
  gtk_box_pack_start (GTK_BOX (box), rut_red, FALSE, FALSE, 0);
  rut = gtk_entry_new_with_max_length (8);

  if (rut_value != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (rut), rut_value);
      //      g_free (rut_value);
    }
  else
    gtk_label_set_markup (GTK_LABEL (rut_red),
			  "<span color=\"red\">*Rut</span>");

  gtk_widget_set_size_request (rut, 150, -1);
  gtk_box_pack_start (GTK_BOX (box), rut, FALSE, FALSE, 0);
  gtk_widget_show (rut);

  box = gtk_vbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  gtk_widget_show (box);
  nombre_red = gtk_label_new ("Nombre de Fantasia");
  gtk_widget_show (nombre_red);
  gtk_box_pack_start (GTK_BOX (box), nombre_red, FALSE, FALSE, 0);
  nombre_fantasia = gtk_entry_new_with_max_length (200);

  if (nombre_fantasia_value != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (nombre_fantasia), nombre_fantasia_value);
      //g_free (nombre_fantasia_value);
    }
  else
    gtk_label_set_markup (GTK_LABEL (nombre_red),
			  "<span color=\"red\">Nombre de Fantasia</span>");

  gtk_widget_set_size_request (nombre_fantasia, 150, -1);
  gtk_box_pack_start (GTK_BOX (box), nombre_fantasia, FALSE, FALSE, 0);
  gtk_widget_show (nombre_fantasia);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  box = gtk_vbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  gtk_widget_show (box);
  direccion_red = gtk_label_new ("*Dirección");
  gtk_widget_show (direccion_red);
  gtk_box_pack_start (GTK_BOX (box), direccion_red, FALSE, FALSE, 0);
  direccion = gtk_entry_new_with_max_length (200);
  if (direccion_value != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (direccion), direccion_value);
      //      g_free (direccion_value);
    }
  else
    gtk_label_set_markup (GTK_LABEL (direccion_red),
			  "<span color=\"red\">*Dirección</span>");

  gtk_widget_set_size_request (direccion, 150, -1);
  gtk_box_pack_start (GTK_BOX (box), direccion, FALSE, FALSE, 0);
  gtk_widget_show (direccion);

  box = gtk_vbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  gtk_widget_show (box);
  comuna_red = gtk_label_new ("*Comuna");
  gtk_widget_show (comuna_red);
  gtk_box_pack_start (GTK_BOX (box), comuna_red, FALSE, FALSE, 0);
  comuna = gtk_entry_new_with_max_length (50);
  if (comuna_value != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (comuna), comuna_value);
      //g_free (comuna_value);
    }
  else
    gtk_label_set_markup (GTK_LABEL (comuna_red),
			  "<span color=\"red\">*Comuna</span>");

  gtk_widget_set_size_request (comuna, 150, -1);
  gtk_box_pack_start (GTK_BOX (box), comuna, FALSE, FALSE, 0);
  gtk_widget_show (comuna);

  box = gtk_vbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  gtk_widget_show (box);
  ciudad_red = gtk_label_new ("*Ciudad");
  gtk_widget_show (ciudad_red);
  gtk_box_pack_start (GTK_BOX (box), ciudad_red, FALSE, FALSE, 0);
  ciudad = gtk_entry_new_with_max_length (50);

  if (ciudad_value != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (ciudad), ciudad_value);
      //      g_free (ciudad_value);
    }
  else
    gtk_label_set_markup (GTK_LABEL (ciudad_red),
			  "<span color=\"red\">*Ciudad</span>");

  gtk_widget_set_size_request (ciudad, 150, -1);
  gtk_box_pack_start (GTK_BOX (box), ciudad, FALSE, FALSE, 0);
  gtk_widget_show (ciudad);

  box = gtk_vbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  gtk_widget_show (box);
  fono_red = gtk_label_new ("*Fono");
  gtk_widget_show (fono_red);
  gtk_box_pack_start (GTK_BOX (box), fono_red, FALSE, FALSE, 0);
  fono = gtk_entry_new_with_max_length (15);

  if (fono_value != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (fono), fono_value);
      //g_free (fono_value);
    }
  else
    gtk_label_set_markup (GTK_LABEL (fono_red),
			  "<span color=\"red\">*Fono</span>");

  gtk_widget_set_size_request (fono, 150, -1);
  gtk_box_pack_start (GTK_BOX (box), fono, FALSE, FALSE, 0);
  gtk_widget_show (fono);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  box = gtk_vbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  gtk_widget_show (box);
  fax_red = gtk_label_new ("Fax");
  gtk_widget_show (fax_red);
  gtk_box_pack_start (GTK_BOX (box), fax_red, FALSE, FALSE, 0);
  fax = gtk_entry_new_with_max_length (15);

  if (fax_value != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (fax), fax_value);
      //g_free (fax_value);
    }
  else
    gtk_label_set_markup (GTK_LABEL (fax_red),
			  "<span color=\"red\">Fax</span>");

  gtk_widget_set_size_request (fax, 150, -1);
  gtk_box_pack_start (GTK_BOX (box), fax, FALSE, FALSE, 0);
  gtk_widget_show (fax);

  box = gtk_vbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  gtk_widget_show (box);
  giro_red = gtk_label_new ("*Giro");
  gtk_widget_show (giro_red);
  gtk_box_pack_start (GTK_BOX (box), giro_red, FALSE, FALSE, 0);
  giro = gtk_entry_new_with_max_length (100);

  if (giro_value != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (giro), giro_value);
      //      g_free (giro_value);
    }
  else
    gtk_label_set_markup (GTK_LABEL (giro_red),
			  "<span color=\"red\">*Giro</span>");

  gtk_widget_set_size_request (giro, 150, -1);
  gtk_box_pack_start (GTK_BOX (box), giro, FALSE, FALSE, 0);
  gtk_widget_show (giro);

  box = gtk_vbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 3);
  gtk_widget_show (box);
  at_red = gtk_label_new ("A.T.");
  gtk_widget_show (at_red);
  gtk_box_pack_start (GTK_BOX (box), at_red, FALSE, FALSE, 0);
  at = gtk_entry_new_with_max_length (100);

  if (at_value != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (at), at_value);
      //      g_free (at_value);
    }
  else
    gtk_label_set_markup (GTK_LABEL (at_red),
			  "<span color=\"red\">A.T.</span>");

  gtk_widget_set_size_request (at, 150, -1);
  gtk_box_pack_start (GTK_BOX (box), at, FALSE, FALSE, 0);
  gtk_widget_show (at);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (SaveDatosNegocio), NULL);
}
