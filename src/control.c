/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
       c-indentation-style: gnu -*- */
/*control.c
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
#include"dimentions.h"
#include"usuario.h"
#include"impuestos.h"
#include"datos_negocio.h"
#include"parametros.h"

void
MainControl (MainBox *module_box)
{
  GtkWidget *vbox;
  //  GtkWidget *hbox;
  GtkWidget *notebook;

  if (module_box->new_box != NULL)
    gtk_widget_destroy (GTK_WIDGET (module_box->new_box));

  module_box->new_box = gtk_hbox_new (FALSE, 0);
  if (solo_venta != TRUE)
    gtk_widget_set_size_request (module_box->new_box, MODULE_BOX_WIDTH-5, MODULE_BOX_HEIGHT-5);
  else
    gtk_widget_set_size_request (module_box->new_box, MODULE_LITTLE_BOX_WIDTH - 5, MODULE_LITTLE_BOX_HEIGHT - 5);
  gtk_widget_show (module_box->new_box);
  gtk_box_pack_start (GTK_BOX (module_box->main_box), module_box->new_box, FALSE, FALSE, 0);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (module_box->new_box), notebook, FALSE, FALSE, 3);
  gtk_widget_show (notebook);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
			    gtk_label_new_with_mnemonic ("_Usuarios")); 
  gtk_widget_set_size_request (notebook, MODULE_BOX_WIDTH - 10, MODULE_BOX_HEIGHT - 10);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);

  /* Usuarios */
  user_box (vbox);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
			    gtk_label_new_with_mnemonic ("_Impuestos")); 
  gtk_widget_set_size_request (notebook, MODULE_BOX_WIDTH - 10, MODULE_BOX_HEIGHT - 10);

  /* Impuestos */
  Impuestos (vbox);

  /* Familia de productos */

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
			    gtk_label_new_with_mnemonic ("_Datos del Negocio"));
  gtk_widget_show (vbox);

  /* Datos Negocio */
  datos_box (vbox);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
			    gtk_label_new_with_mnemonic ("_Parametrización"));
  gtk_widget_show (vbox);

  /* Parametrizacion */
  Parametros (vbox);

}
