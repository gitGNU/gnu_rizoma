/*main.c
*
*    Copyright (C) 2004,2008 Rizoma Tecnologia Limitada <zeus@emacs.cl>
*
*       This program is free software; you can redistribute it and/or modify
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

#ifndef MAIN_H

#define MAIN_H

void SelectMenu (GtkWidget *widget, gpointer data);

void show_selected (GtkTreeSelection *selection, gpointer data);

void ClosePasswdWindow (void);

void check_passwd (GtkWidget *widget, gpointer data);

void MainWindow (void);

void SendCursorTo (GtkWidget *widget, gpointer data);

void Question (MainBox *module_box);

gchar * PutPoints (gchar *number);

gchar * CutPoints (gchar *number_points);

GtkWidget * Image (GtkWidget * win, char **nombre);

#endif
