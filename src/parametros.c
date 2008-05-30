/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
       c-indentation-style: gnu -*- */
/*control.c
*
*    Copyright (C) 2005 Rizoma Tecnologia Limitada <info@rizoma.cl>
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

#include"tipos.h"
#include"boleta.h"
#include"errors.h"
#include"config_file.h"
#include"utils.h"

GtkWidget *boleta_entry;
GtkWidget *factura_entry;

GtkWidget *db_name_entry;
GtkWidget *db_user_entry;
GtkWidget *db_pass_entry;
GtkWidget *db_host_entry;
GtkWidget *temp_entry;
GtkWidget *copy_entry;
GtkWidget *print_entry;

void
ModificarNumber (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  gint new_number;

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_ticketnum"));
  new_number = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget))));

  if ((set_ticket_number (new_number, SIMPLE)) == FALSE)
    ErrorMSG (widget, "Ocurrió un error mientras se intento modificar el número.");
  else
    {
      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
      statusbar_push(GTK_STATUSBAR(aux_widget), "Se modifico el folio de la boleta con exito.", 3000);
    }
}

void
ModificarNumberF (GtkWidget *widget, gpointer data)
{
  GtkWidget *aux_widget;
  gint new_number;

  aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_invoicenum"));
  new_number = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (aux_widget))));

  if ((set_ticket_number (new_number, FACTURA)) == FALSE)
    ErrorMSG (factura_entry, "Ocurrió un error mientras se intento modificar el número.");
  else
    {
      aux_widget = GTK_WIDGET(gtk_builder_get_object(builder, "statusbar"));
      statusbar_push (GTK_STATUSBAR(aux_widget), "Se modifico el folio de la factura con exito.", 3000);
    }
}

void
guardar_parametros (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *aux_widget;

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_dbname"));
  if ((rizoma_set_value ("DB_NAME", gtk_entry_get_text (GTK_ENTRY (aux_widget)))) != 0)
    {
      ErrorMSG (db_name_entry, "Ocurrió un error mientras se modificaban los parametros");
      return;
    }

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_dbuser"));
  if ((rizoma_set_value ("USER", gtk_entry_get_text (GTK_ENTRY (aux_widget)))) != 0)
    {
      ErrorMSG (db_user_entry, "Ocurrió un error mientras se modificaban los parametros");
      return;
    }

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_dbpass"));
  if ((rizoma_set_value ("PASSWORD", gtk_entry_get_text (GTK_ENTRY (aux_widget)))) != 0)
    {
      ErrorMSG (db_pass_entry, "Ocurrió un error mientras se modificaban los parametros");
      return;
    }

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_dbhost"));
  if ((rizoma_set_value ("SERVER_HOST", gtk_entry_get_text (GTK_ENTRY (aux_widget)))) != 0)
    {
      ErrorMSG (db_host_entry, "Ocurrió un error mientras se modificaban los parametros");
      return;
    }

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_tmpfiles"));
  if ((rizoma_set_value ("TEMP_FILES", gtk_entry_get_text (GTK_ENTRY (aux_widget)))) != 0)
    {
      ErrorMSG (temp_entry, "Ocurrió un error mientras se modificaban los parametros");
      return;
    }

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_copynum"));
  if ((rizoma_set_value ("VALE_COPY", gtk_entry_get_text (GTK_ENTRY (aux_widget)))) != 0)
    {
      ErrorMSG (copy_entry, "Ocurrió un error mientras se modificaban los parametros");
      return;
    }

  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "entry_admin_printcmd"));
  if ((rizoma_set_value ("PRINT_COMMAND", gtk_entry_get_text (GTK_ENTRY (aux_widget)))) != 0)
    {
      ErrorMSG (print_entry, "Ocurrió un error mientras se modificaban los parametros");
      return;
    }
  aux_widget = GTK_WIDGET (gtk_builder_get_object(builder, "statusbar"));
  statusbar_push (GTK_STATUSBAR(aux_widget), "Se modificaron los parametros con exito.", 3000);
}

void
preferences_box ()
{
  GtkWidget *widget;
  gint current_number;


  // Boleta/ticket
  current_number = get_ticket_number (SIMPLE) - 1;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_ticketnum"));
  gtk_entry_set_text (GTK_ENTRY (widget),
		      g_strdup_printf ("%d", current_number));

  //Factura
  current_number = get_ticket_number (FACTURA) - 1;

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_invoicenum"));
  gtk_entry_set_text (GTK_ENTRY (widget),
		      g_strdup_printf ("%d", current_number));


  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_dbname"));
  gtk_entry_set_text (GTK_ENTRY (widget), rizoma_get_value ("DB_NAME"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_dbuser"));
  gtk_entry_set_text (GTK_ENTRY (widget), rizoma_get_value ("USER"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_dbpasswd"));
  gtk_entry_set_text (GTK_ENTRY (widget), rizoma_get_value ("PASSWORD"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_dbhost"));
  gtk_entry_set_text (GTK_ENTRY (widget), rizoma_get_value ("SERVER_HOST"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_tmpfiles"));
  gtk_entry_set_text (GTK_ENTRY (widget), rizoma_get_value ("TEMP_FILES"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_copynum"));
  gtk_entry_set_text (GTK_ENTRY (widget), rizoma_get_value ("VALE_COPY"));

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "entry_admin_printcmd"));
  gtk_entry_set_text (GTK_ENTRY (widget), rizoma_get_value ("PRINT_COMMAND"));
}
