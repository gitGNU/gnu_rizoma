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

  if (strcmp (PQvaluebycol (res, 0, "razon_social"), "") != 0)
    razon_social_value = PQvaluebycol (res, 0, "razon_social");
  else
    razon_social_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "rut"), "") != 0)
    rut_value = PQvaluebycol (res, 0, "rut");
  else
    rut_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "nombre"), "") != 0)
    nombre_fantasia_value = PQvaluebycol (res, 0, "nombre");
  else
    nombre_fantasia_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "fono"), "") != 0)
    fono_value = PQvaluebycol (res, 0, "fono");
  else
    fono_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "fax"), "") != 0)
    fax_value = PQvaluebycol (res, 0, "fax");
  else
    fax_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "direccion"), "") != 0)
    direccion_value = PQvaluebycol (res, 0, "direccion");
  else
    direccion_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "comuna"), "") != 0)
    comuna_value = PQvaluebycol (res, 0, "comuna");
  else
    comuna_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "ciudad"), "") != 0)
    ciudad_value = PQvaluebycol (res, 0, "ciudad");
  else
    ciudad_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "giro"), "") != 0)
    giro_value = PQvaluebycol (res, 0, "giro");
  else
    giro_value = NULL;

  if (strcmp (PQvaluebycol (res, 0, "at"), "") != 0)
    at_value = PQvaluebycol (res, 0, "at");
  else
    at_value = NULL;

  return 1;
}

void
datos_box ()
{
  GtkWidget *widget;

  get_datos ();

  if (razon_social_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_razon"));
      gtk_entry_set_text (GTK_ENTRY (widget), razon_social_value);
    }
  else
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_razon"));
      gtk_label_set_markup (GTK_LABEL (widget),
			    "<span color=\"red\">*Razón Social:</span>");
    }

  if (rut_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_rut"));
      gtk_entry_set_text (GTK_ENTRY (widget), rut_value);
    }
  else
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_rut"));
      gtk_label_set_markup (GTK_LABEL (widget),
			    "<span color=\"red\">*Rut</span>");
    }

  if (nombre_fantasia_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_fantasy"));
      gtk_entry_set_text (GTK_ENTRY (widget), nombre_fantasia_value);
    }
  else
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_fantasy"));
      gtk_label_set_markup (GTK_LABEL (widget),
			    "<span color=\"red\">Nombre de Fantasia</span>");
    }

  if (direccion_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_addr"));
      gtk_entry_set_text (GTK_ENTRY (widget), direccion_value);
    }
  else
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_addr"));
      gtk_label_set_markup (GTK_LABEL (widget),
			    "<span color=\"red\">*Dirección</span>");
    }

  if (comuna_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_comuna"));
      gtk_entry_set_text (GTK_ENTRY (widget), comuna_value);
    }
  else
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_comuna"));
      gtk_label_set_markup (GTK_LABEL (widget),
			    "<span color=\"red\">*Comuna</span>");
    }

  if (ciudad_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_city"));
      gtk_entry_set_text (GTK_ENTRY (widget), ciudad_value);
    }
  else
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_city"));
      gtk_label_set_markup (GTK_LABEL (widget),
			    "<span color=\"red\">*Ciudad</span>");
    }

  if (fono_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_phone"));
      gtk_entry_set_text (GTK_ENTRY (widget), fono_value);
    }
  else
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_phone"));
      gtk_label_set_markup (GTK_LABEL (widget),
			    "<span color=\"red\">*Fono</span>");
    }

  if (fax_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_fax"));
      gtk_entry_set_text (GTK_ENTRY (widget), fax_value);
    }
  else
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_fax"));
      gtk_label_set_markup (GTK_LABEL (widget),
			    "<span color=\"red\">Fax</span>");
    }

  if (giro_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_giro"));
      gtk_entry_set_text (GTK_ENTRY (widget), giro_value);
    }
  else
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_giro"));
      gtk_label_set_markup (GTK_LABEL (widget),
			    "<span color=\"red\">*Giro</span>");
    }

  if (at_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_at"));
      gtk_entry_set_text (GTK_ENTRY (widget), at_value);
    }
  else
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_at"));
      gtk_label_set_markup (GTK_LABEL (widget),
			    "<span color=\"red\">A.T.</span>");
    }
}
