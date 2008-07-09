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
#include"utils.h"

gchar *razon_social_value = NULL;
gchar *rut_value = NULL;
gchar *nombre_fantasia_value = NULL;
gchar *fono_value = NULL;
gchar *direccion_value = NULL;
gchar *comuna_value = NULL;
gchar *ciudad_value = NULL;
gchar *fax_value = NULL;
gchar *giro_value = NULL;
gchar *at_value = NULL;

void
refresh_labels (void)
{
  GtkWidget *widget;
  /* We get all the new data */

  get_datos ();

  /* We refresh all labesl */

  //Razon
  if (razon_social_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_razon"));
      gtk_entry_set_text (GTK_ENTRY (widget), razon_social_value);
    }

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_razon"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        razon_social_value !=NULL ?
                        "*Razón Social:":
                        "<span color=\"red\">*Razón Social:</span>");

  //rut
  if (rut_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_rut"));
      gtk_entry_set_text (GTK_ENTRY (widget), rut_value);
    }

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_rut"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        rut_value != NULL ?
                        "*Rut:":
                        "<span color=\"red\">*Rut:</span>");

  //nombre de fantasia
  if (nombre_fantasia_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_fantasy"));
      gtk_entry_set_text (GTK_ENTRY (widget), nombre_fantasia_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_fantasy"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        nombre_fantasia_value != NULL ?
                        "Nombre de Fantasia:":
                        "<span color=\"red\">Nombre de Fantasia</span>");

  //direccion
  if (direccion_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_addr"));
      gtk_entry_set_text (GTK_ENTRY (widget), direccion_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_addr"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        direccion_value != NULL ?
                        "*Direccion:":
                        "<span color=\"red\">*Dirección</span>");

  //comuna
  if (comuna_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_comuna"));
      gtk_entry_set_text (GTK_ENTRY (widget), comuna_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_comuna"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        comuna_value != NULL ?
                        "*Comuna:":
                        "<span color=\"red\">*Comuna</span>");

  //ciudad
  if (ciudad_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_city"));
      gtk_entry_set_text (GTK_ENTRY (widget), ciudad_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_city"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        ciudad_value != NULL ?
                        "*Ciudad:":
                        "<span color=\"red\">*Ciudad</span>");


  //fono
  if (fono_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_phone"));
      gtk_entry_set_text (GTK_ENTRY (widget), fono_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_phone"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        fono_value != NULL ?
                        "*Fono":
                        "<span color=\"red\">*Fono</span>");

  //fax
  if (fax_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_fax"));
      gtk_entry_set_text (GTK_ENTRY (widget), fax_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_fax"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        fax_value != NULL ?
                        "Fax:":
                        "<span color=\"red\">Fax</span>");

  //Giro
  if (giro_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_giro"));
      gtk_entry_set_text (GTK_ENTRY (widget), giro_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_giro"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        giro_value != NULL ?
                        "*Giro:":
                        "<span color=\"red\">*Giro</span>");

  //AT
  if (at_value != NULL)
    {
      widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_at"));
      gtk_entry_set_text (GTK_ENTRY (widget), at_value);
    }
  widget = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_admin_at"));
  gtk_label_set_markup (GTK_LABEL (widget),
                        at_value != NULL ?
                        "A.T.:":
                        "<span color=\"red\">A.T.</span>");
}

void
SaveDatosNegocio (GtkWidget *widget, gpointer data)
{
  PGresult *res;
  GtkWidget *aux_widget;

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_razon"));
  razon_social_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_rut"));
  rut_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_fantasy"));
  nombre_fantasia_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_phone"));
  fono_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_addr"));
  direccion_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_comuna"));
  comuna_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_city"));
  ciudad_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_fax"));
  fax_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_giro"));
  giro_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_at"));
  at_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget)));

  res = EjecutarSQL ("SELECT count(*) FROM negocio");

  if (g_str_equal(PQgetvalue (res, 0, 0), "0"))
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
    {
      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
      statusbar_push (GTK_STATUSBAR(aux_widget), "Los datos del negocio se actualizaron exitosamente", 3000);
    }
  else
    AlertMSG (aux_widget, "Se produjo un error al intentar actualizar los datos!");

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
  refresh_labels();
}
