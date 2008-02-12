/*credito.h
*
*    Copyright 2004 Rizoma Tecnologia Limitada <jonathan@rizoma.cl>
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

#ifndef CREDITO_H

#define CREDITO_H

//void creditos_box (MainBox *module_box);
void creditos_box (GtkWidget *main_box);

void AddClient (GtkWidget *widget, gpointer data);

void CloseAddClientWindow (void);

GtkWidget * caja_entrada (gchar *text, gint largo_maximo, gint ancho, GtkWidget *entry);

void AgregarClienteABD (GtkWidget *widget, gpointer data);

gboolean VerificarRut (gchar *rut, gchar *ver);

gint FillClientStore (GtkListStore *store);

void DatosDeudor (void);

void FillVentasDeudas (gint rut);

void ChangeDetalle (void);

gint AbonarWindow (void);

void Abonar (void);

void ModificarCliente (void);

void ClientStatus (void);

gboolean VentaPosible (gint rut, gint total_venta);

gint ToggleClientCredit (GtkCellRendererToggle *toggle, char *path_str, gpointer data);

void EliminarCliente (void);

void ModificarClienteDB (void);

gint LimiteCredito (gchar *rut);

#endif
