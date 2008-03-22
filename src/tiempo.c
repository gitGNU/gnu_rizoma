/* -*- Mode: C; tab-width: 4; ident-tabs-mode: nil; c-basic-offset: 4;
       c-indentation-style: gnu -*- */
/*tiempo.c
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

#include<time.h>

#include"tipos.h"

#define YEAR(x) x + 1900

static gint day;
static gint month;
static gint year;

const struct __months {
  int month;
  char *name;
} months [12] =
  {
    {0, "Enero"},
    {1, "Febrero"},
    {2, "Marzo"},
    {3, "Abril"},
    {4, "Mayo"},
    {5, "Junio"},
    {6, "Julio"},
    {7, "Agosto"},
    {8, "Septiembre"},
    {9, "Octubre"},
    {10, "Noviembre"},
    {11, "Diciembre"}
  };

static struct __days {
  int day;
  char *name;
} days [7] =
  {
    {0, "Domingo"},
    {1, "Lunes"},
    {2, "Martes"},
    {3, "Miércoles"},
    {4, "Jueves"},
    {5, "Viernes"},
    {6, "Sábado"}
  };

GtkWidget *
show_date (void)
{
  GtkWidget *date;
  time_t t;
  struct tm *fecha;

  time (&t);

  fecha = localtime (&t);

  date = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (date),
			g_strdup_printf ("<span size=\"xx-large\"><b>%s %d\n%s\n%d</b></span>",
					 days[fecha->tm_wday].name, fecha->tm_mday,
					 months[fecha->tm_mon].name, YEAR (fecha->tm_year)));

  gtk_label_set_justify (GTK_LABEL (date), GTK_JUSTIFY_CENTER);

  day = fecha->tm_mday;
  month = fecha->tm_mon;
  year = fecha->tm_year;

  return date;
}

gboolean
RefreshTime (gpointer data)
{
  time_t t;
  struct tm *hora;

  time (&t);

  hora = localtime (&t);


  gtk_label_set_markup (GTK_LABEL (hour_label),
			g_strdup_printf ("<span size=\"xx-large\"><b>%2.2d:%2.2d:%2.2d</b></span>",
					 hora->tm_hour, hora->tm_min, hora->tm_sec));

  if (hora->tm_mday != day || hora->tm_mon != month || hora->tm_year != year)
    {
      gtk_label_set_markup (GTK_LABEL (date_label),
			    g_strdup_printf ("<span size=\"xx-large\"><b>%s %d\n%s\n%d</b></span>",
					     days[hora->tm_wday].name, hora->tm_mday,
					     months[hora->tm_mon].name, YEAR (hora->tm_year)));
    }

  return TRUE;
}

gchar *
CurrentDate (void)
{
  time_t t;
  struct tm *fecha;

  time (&t);

  fecha = localtime (&t);

  return g_strdup_printf ("%.2d-%.2d-%.2d", fecha->tm_mday, fecha->tm_mon+1, YEAR (fecha->tm_year));
}
gchar *
CurrentTime (void)
{
  time_t t;
  struct tm *hora;

  time (&t);

  hora = localtime (&t);

  return g_strdup_printf ("%2.2d:%2.2d:%2.2d", hora->tm_hour, hora->tm_min, hora->tm_sec);
}
