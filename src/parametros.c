/* -*- Mode: C; tab-width: 4; ident-tabs-mode: nil; c-basic-offset: 4;
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
  gint new_number = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (boleta_entry))));

  if ((set_ticket_number (new_number, SIMPLE)) == FALSE)
    ErrorMSG (boleta_entry, "Ocurrió un error mientras se intento modificar el número.");
  else
    ExitoMSG (boleta_entry, "Se modifico el folio con exito.");
}

void
ModificarNumberF (GtkWidget *widget, gpointer data)
{
  gint new_number = atoi (g_strdup (gtk_entry_get_text (GTK_ENTRY (factura_entry))));

  if ((set_ticket_number (new_number, FACTURA)) == FALSE)
    ErrorMSG (factura_entry, "Ocurrió un error mientras se intento modificar el número.");
  else
    ExitoMSG (factura_entry, "Se modifico el folio con exito.");
}

void
guardar_parametros (GtkWidget *widget, gpointer user_data)
{

  if ((rizoma_set_value ("DB_NAME", (gchar *)gtk_entry_get_text (GTK_ENTRY (db_name_entry)))) != 0)
    {
      ErrorMSG (db_name_entry, "Ocurrió un error mientras se modificaban los parametros");
      return;
    }

  if ((rizoma_set_value ("USER", (gchar *)gtk_entry_get_text (GTK_ENTRY (db_user_entry)))) != 0)
    {
      ErrorMSG (db_user_entry, "Ocurrió un error mientras se modificaban los parametros");
      return;
    }
  if ((rizoma_set_value ("PASSWORD", (gchar *)gtk_entry_get_text (GTK_ENTRY (db_pass_entry)))) != 0)
    {
      ErrorMSG (db_pass_entry, "Ocurrió un error mientras se modificaban los parametros");
      return;
    }
  if ((rizoma_set_value ("SERVER_HOST", (gchar *)gtk_entry_get_text (GTK_ENTRY (db_host_entry)))) != 0)
    {
      ErrorMSG (db_host_entry, "Ocurrió un error mientras se modificaban los parametros");
      return;
    }
  if ((rizoma_set_value ("TEMP_FILES", (gchar *)gtk_entry_get_text (GTK_ENTRY (temp_entry)))) != 0)
    {
      ErrorMSG (temp_entry, "Ocurrió un error mientras se modificaban los parametros");
      return;
    }
  if ((rizoma_set_value ("VALE_COPY", (gchar *)gtk_entry_get_text (GTK_ENTRY (copy_entry)))) != 0)
    {
      ErrorMSG (copy_entry, "Ocurrió un error mientras se modificaban los parametros");
      return;
    }
  if ((rizoma_set_value ("PRINT_COMMAND", (gchar *)gtk_entry_get_text (GTK_ENTRY (print_entry)))) != 0)
    {
      ErrorMSG (print_entry, "Ocurrió un error mientras se modificaban los parametros");
      return;
    }

  ExitoMSG (db_name_entry, "Se modificaron los parametros con exito.");

}

void
modificar_config (GtkWidget *box)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 3);
  gtk_widget_show (vbox);

  label = gtk_label_new ("Nombre base de datos");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  db_name_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (db_name_entry), rizoma_get_value ("DB_NAME"));
  gtk_widget_set_size_request (db_name_entry, 50, -1);
  gtk_box_pack_start (GTK_BOX (vbox), db_name_entry, FALSE, FALSE, 3);
  gtk_widget_show (db_name_entry);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 3);
  gtk_widget_show (vbox);

  label = gtk_label_new ("Usuario BD");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  db_user_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (db_user_entry), rizoma_get_value ("USER"));
  gtk_widget_set_size_request (db_user_entry, 50, -1);
  gtk_box_pack_start (GTK_BOX (vbox), db_user_entry, FALSE, FALSE, 3);
  gtk_widget_show (db_user_entry);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 3);
  gtk_widget_show (vbox);

  label = gtk_label_new ("Contraseña BD");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  db_pass_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (db_pass_entry), rizoma_get_value ("PASSWORD"));
  gtk_widget_set_size_request (db_pass_entry, 50, -1);
  gtk_box_pack_start (GTK_BOX (vbox), db_pass_entry, FALSE, FALSE, 3);
  gtk_widget_show (db_pass_entry);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 3);
  gtk_widget_show (vbox);

  label = gtk_label_new ("Host BD");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  db_host_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (db_host_entry), rizoma_get_value ("SERVER_HOST"));
  gtk_widget_set_size_request (db_host_entry, 80, -1);
  gtk_box_pack_start (GTK_BOX (vbox), db_host_entry, FALSE, FALSE, 3);
  gtk_widget_show (db_host_entry);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 3);
  gtk_widget_show (vbox);

  label = gtk_label_new ("Archivos temporales");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  temp_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (temp_entry), rizoma_get_value ("TEMP_FILES"));
  gtk_widget_set_size_request (temp_entry, 80, -1);
  gtk_box_pack_start (GTK_BOX (vbox), temp_entry, FALSE, FALSE, 3);
  gtk_widget_show (temp_entry);


  vbox = gtk_vbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 3);
  gtk_widget_show (vbox);

  label = gtk_label_new ("Copias por vale");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  copy_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (copy_entry), rizoma_get_value ("VALE_COPY"));
  gtk_widget_set_size_request (copy_entry, 80, -1);
  gtk_box_pack_start (GTK_BOX (vbox), copy_entry, FALSE, FALSE, 3);
  gtk_widget_show (copy_entry);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 3);
  gtk_widget_show (vbox);

  label = gtk_label_new ("Comando de impresion");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  print_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (print_entry),
		      rizoma_get_value ("PRINT_COMMAND"));
  gtk_widget_set_size_request (print_entry, 80, -1);
  gtk_box_pack_start (GTK_BOX (vbox), print_entry, FALSE, FALSE, 3);
  gtk_widget_show (print_entry);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (guardar_parametros), NULL);

}

void
Parametros (GtkWidget *main_box)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *label;
  GtkWidget *button;

  gint current_number = get_ticket_number (SIMPLE);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (main_box), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  /*
    Ticket
  */

  frame = gtk_frame_new ("Ajustar número boleta");
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 3);;
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new ("Ingrese el ultimo número de boleta emitido");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox2 = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 3);
  gtk_widget_show (hbox2);

  boleta_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox2), boleta_entry, FALSE, FALSE, 3);
  gtk_widget_show (boleta_entry);

  gtk_entry_set_text (GTK_ENTRY (boleta_entry),
		      g_strdup_printf ("%d", current_number));

  g_signal_connect (G_OBJECT (boleta_entry), "activate",
		    G_CALLBACK (ModificarNumber), (gpointer)SIMPLE);

  button = gtk_button_new_with_label ("Modificar");
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (ModificarNumber), NULL);


  /*
    Factura
  */

  current_number = get_ticket_number (FACTURA);

  frame = gtk_frame_new ("Ajustar número de Factura");
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 3);;
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new ("Ingrese el ultimo número de factura emitido");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 3);
  gtk_widget_show (label);

  hbox2 = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 3);
  gtk_widget_show (hbox2);

  factura_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox2), factura_entry, FALSE, FALSE, 3);
  gtk_widget_show (factura_entry);

  gtk_entry_set_text (GTK_ENTRY (factura_entry),
		      g_strdup_printf ("%d", current_number));

  g_signal_connect (G_OBJECT (factura_entry), "activate",
		    G_CALLBACK (ModificarNumberF), (gpointer)FACTURA);

  button = gtk_button_new_with_label ("Modificar");
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 3);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (ModificarNumberF), NULL);

  /*
    Una nueva linea
  */

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (main_box), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  /*
    Configuracion
  */

  frame = gtk_frame_new ("Modificar configuracion");
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 3);;
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  modificar_config (vbox);
}
