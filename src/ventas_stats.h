/*ventas_stats.h
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

#ifndef VENTAS_STATS_H

#define VENTAS_STATS_H

/* We need to see this vars from the main.c file */
GtkWidget *calendar_from;
GtkWidget *calendar_to;

void SetToggleMode (GtkToggleButton *widget, gpointer data);

void ask_date (MainBox *module_box);

void SetDate (GtkCalendar *calendar, gpointer data);

void control_decimal (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, 
		      GtkTreeIter *iter, gpointer user_data);

void ventas_stats_box (MainBox *module_box);

void GetSelectedDate (void);

void FillStatsStore (GtkTreeStore *store);

void FillTreeView (void);

void SetTotalSell (void);

void ChangeVenta (void);

void FillCajaStats (void);

void FillProductsRank (void);

void FillListProveedores (void);

void RefreshCurrentTab (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data);

void InformeFacturas (GtkWidget *box);

#endif
