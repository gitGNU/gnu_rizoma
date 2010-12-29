/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*utils.h
*
*    Copyright (C) 2008 Rizoma Tecnologia Limitada <info@rizoma.cl>
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

#ifndef UTILS_H_

#define UTILS_H_

#define CUT(num) CutComa (num)

#define PUT(num) PutComa (num)

#define builder_get(builder,object) gtk_builder_get_object(builder,object)

#define YEAR(x) x + 1900

void SetToggleMode (GtkToggleButton *widget, gpointer data);

gboolean HaveCharacters (gchar *string);

void SendCursorTo (GtkWidget *widget, gpointer data);

gchar * PutPoints (gchar *number);

gchar * CutPoints (gchar *number_points);

void control_decimal (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);

void display_calendar (GtkEntry *entry);

void statusbar_push (GtkStatusbar *statusbar, const gchar *text, guint duration);

gboolean validate_string (gchar *pattern, gchar *subject);

void clean_container (GtkContainer *container);

gchar ** parse_rut (gchar *rut);

gchar * CurrentDate (int tipo);

gchar * CurrentTime (void);

gboolean log_register_access (User *info_user, gboolean login);

void select_back_deleted_row (gchar *treeViewName, gint deletedRowPosition);

gchar * invested_strndup (const gchar *texto, gint index);

#endif
