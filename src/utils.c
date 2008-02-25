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

#include<glib.h>
#include<gtk/gtk.h>

#include<string.h>

void
SetToggleMode (GtkToggleButton *widget, gpointer data)
{

  if (gtk_toggle_button_get_active (widget) == TRUE)
    gtk_toggle_button_set_active (widget, FALSE);
  else
    gtk_toggle_button_set_active (widget, TRUE);
}

gboolean
HaveCharacters (gchar *string)
{
  gint i, len = strlen (string);

  for (i = 0; i <= len; i++)
    {
      if (g_ascii_isalpha (string[i]) == TRUE)
	return TRUE;
    }

  return FALSE;
}

void
SendCursorTo (GtkWidget *widget, gpointer data)
{
  /* GtkWindow *window = GTK_WINDOW (gtk_widget_get_toplevel ((GtkWidget *)data));
  GtkWidget *destiny = (GtkWidget *) data;

  gtk_window_set_focus (window, destiny);*/
  gtk_widget_grab_focus (GTK_WIDGET (data));
}

gchar *
PutPoints (gchar *number)
{
  gchar *alt = g_malloc0 (15), *alt3 = g_malloc0 (15);
  int len;
  gint points;
  gint i, unidad = 0, point = 0;

  if (number == NULL)
    return "";

  len = strlen (number);

  if (len <= 3)
    return number;

  if ((len % 3) != 0)
    points = len / 3;
  else
    points = (len / 3) - 1;

  for (i = len; i >= 0; i--)
    {
      if (unidad == 3 && point < points && number[i] != '.' && number[i] != ',')
	{
	  g_snprintf (alt, 15, ".%c%s", number[i], alt3);
	  unidad = 0;
	  point++;
	}
      else
	g_snprintf (alt, 15, "%c%s", number[i], alt3);

      strcpy (alt3, alt);

      unidad++;
    }

  return alt;
}

gchar *
CutPoints (gchar *number_points)
{
  gint len = strlen (number_points);
  gchar *number = g_malloc0 (len);
  gint i, o = 0;

  if (strcmp (number_points, "") == 0)
    return number_points;

  for (i = 0; i <= len; i++)
    if (number_points[i] != '.')
      number[o++] = number_points[i];

  g_free (number_points);

  return number;
}
