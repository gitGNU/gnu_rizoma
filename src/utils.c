/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
       c-indentation-style: gnu -*- */
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
#include<stdlib.h>

#include "tipos.h"
#include "postgres-functions.h"
#include "config_file.h"
#include "utils.h"

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

void
control_decimal (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model,
                 GtkTreeIter *iter, gpointer user_data)
{
  gint column = (gint) user_data;
  gchar  buf[20];
  GType column_type = gtk_tree_model_get_column_type (model, column);

  switch (column_type)
    {
    case G_TYPE_DOUBLE:
      {
        gdouble number;
        gtk_tree_model_get(model, iter, column, &number, -1);
        g_snprintf (buf, sizeof (buf), "%.3f", number);
        g_object_set(renderer, "text", buf, NULL);
      }
      break;
    case G_TYPE_INT:
      {
        gint number;
        gtk_tree_model_get(model, iter, column, &number, -1);
        g_snprintf (buf, sizeof (buf), "%d", number);
        g_object_set(renderer, "text", PutPoints (buf), NULL);
      }
      break;
    default:
      break;
    }

}

/**
 * This callbacks handle the double-click event for the
 * display_calendar function
 *
 * @param calendar
 * @param user_data
 */
void
on_calendar_day_selected_double_click (GtkCalendar *calendar, gpointer user_data)
{
  GtkEntry *entry = (GtkEntry *)user_data;
  gchar str_date[256];
  guint year;
  guint month;
  guint day;
  GDate *date;

  gtk_calendar_get_date (calendar, &year, &month, &day);

  date = g_date_new_dmy (day, month + 1, year);

  if (g_date_valid (date))
    {
      if (g_date_strftime (str_date, sizeof (str_date), "%x", date) == 0) strcpy (str_date, "---");

      gtk_entry_set_text (entry, str_date);
    }

  gtk_widget_destroy (GTK_WIDGET (gtk_widget_get_parent_window (GTK_WIDGET (calendar))));
}

gboolean
calendar_focus_out (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gtk_widget_destroy (widget);

  return TRUE;
}

/**
 * display_calendar must be attached to a button, and the with clicked
 * event this will display a GtkCalender just under the GtkEntry
 * passed as the argument. When a double-click event occur on the
 * GtkCalendar the selected date will be saved on the GtkEntry and the
 * GtkCalendar will die.
 *
 * @param entry The Gtk entry which will contain the date
 */
void
display_calendar (GtkEntry *entry)
{
  GtkWidget *window;
  GtkCalendar *calendar;
  GtkRequisition req;
  gint x, y;
  gint entry_height, entry_width;
  const gchar *previous_date = gtk_entry_get_text (entry);
  GDate *date;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width (GTK_CONTAINER (window), 0);
  gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  gtk_window_stick (GTK_WINDOW (window));

  gtk_widget_set_parent_window (window, GTK_WIDGET (entry)->window);
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);

  g_signal_connect (G_OBJECT (window), "focus-out-event",
                    G_CALLBACK (calendar_focus_out), NULL);

  calendar = GTK_CALENDAR (gtk_calendar_new ());
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (calendar));
  gtk_widget_show (GTK_WIDGET (calendar));

  gtk_widget_set_parent_window (GTK_WIDGET (calendar), (GdkWindow *)window);

  date = g_date_new ();

  if (g_str_equal (previous_date, ""))
    {
      g_date_set_time_t (date, time (NULL));
    }
  else
    {
      g_date_set_parse (date, previous_date);
    }

  if ( ! g_date_valid (date))
    {
      gtk_widget_destroy (window);
      return;
    }

  gtk_calendar_select_day (calendar, g_date_get_day (date));
  gtk_calendar_select_month (calendar, g_date_get_month (date) - 1, g_date_get_year (date));

  g_signal_connect (G_OBJECT (calendar), "day-selected-double-click",
                    G_CALLBACK (on_calendar_day_selected_double_click), (gpointer) entry);

  gdk_window_get_origin (GTK_WIDGET (entry)->window, &x, &y);

  gtk_widget_size_request (GTK_WIDGET (entry), &req);

  entry_width = GTK_WIDGET (entry)->allocation.width;
  entry_height = GTK_WIDGET (entry)->allocation.height;

  x += entry_width - (req.width);
  y += entry_height;

  if (x < 0) x = 0;
  if (y < 0) y = 0;

  gtk_window_move (GTK_WINDOW (window), x, y);
  gtk_window_present (GTK_WINDOW (window));

  gtk_widget_show (window);
}

/**
 * Validate a string using a regular expression
 *
 * @param pattern regexp string to run
 * @param subject string to check using pattern
 *
 * @return TRUE if the pattern it's found on subject
 */
gboolean
validate_string (gchar *pattern, gchar *subject)
{
  GRegex *regex = g_regex_new (pattern, 0, 0, NULL);
  gboolean valid = g_regex_match (regex, subject, 0, NULL);

  return valid;
}

gboolean
statusbar_pop (GtkStatusbar *statusbar)
{

  guint *context_id;

  context_id = g_object_get_data (G_OBJECT(statusbar), "context_id");

  gtk_statusbar_pop (GTK_STATUSBAR(statusbar), *context_id);

  g_free (context_id);

  return FALSE;
}

/**
 * This is a helper function to show a message in the status bar in a
 * simple way.
 *
 * @param statusbar The statusbar that will be used to display the text
 * @param text The message that will be displayed in the statusbar
 * @param duration The duration of the message
 */
void
statusbar_push (GtkStatusbar *statusbar, const gchar *text, guint duration)
{
  guint *context_id;
  guint message_id;

  context_id = g_malloc(sizeof(guint));

  *context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR(statusbar), "rizoma-compras");
  message_id = gtk_statusbar_push (statusbar, *context_id, text);

  g_object_set_data (G_OBJECT(statusbar), "context_id", context_id);

  g_timeout_add (duration, (GSourceFunc) statusbar_pop, (gpointer) statusbar);
}

void
gtk_entry_set_alert(GtkEntry *entry)
{
  /*
   * TODO: is neccesary get the gtk widget colors to setup the alert
   */
  return;
}

/**
 * This is a helper function to clean any container with an empty
 * string on entries and labels. Any widget without a number on the
 * name will be set to an empty string if it's a label or entry.
 *
 * @param container The container to check
 */
void
clean_container (GtkContainer *container)
{
  GList *list = gtk_container_get_children (container);
  GtkWidget *widget;
  gchar *widget_name = NULL;
  GtkTreeModel *model;

  while (list != NULL)
    {
      if (! GTK_IS_TREE_VIEW (list->data) && GTK_IS_CONTAINER (list->data))
        {
          clean_container (GTK_CONTAINER (list->data));
        }
      else
        {
          widget = GTK_WIDGET (list->data);
          widget_name = g_strdup (gtk_widget_get_name (widget));
          if (!validate_string ("[0-9]+", widget_name) && !validate_string ("Gtk", widget_name))
            {
              if (GTK_IS_ENTRY (widget))
                {
                  gtk_entry_set_text (GTK_ENTRY (widget), "");
                }
              else if (GTK_IS_LABEL (widget))
                {
                  gtk_label_set_text (GTK_LABEL (widget), "");
                }
              else if (GTK_IS_TREE_VIEW (widget))
                {
                  model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
                  if (GTK_IS_LIST_STORE (model))
                    {
                      gtk_list_store_clear (GTK_LIST_STORE (model));
                    }
                }
            }
          g_free (widget_name);
        }

      list = g_list_next (list);
    }

  g_list_free (list);
}

/**
 * This function aim to parse without any errors a rut.
 * This function will return a gchar ** with the rut as the element 0
 * and the dv as the element 1.
 *
 * @param rut The rut to parse.
 */
gchar **
parse_rut (gchar *rut)
{
  gchar *pattern_clean = "[[:^alnum:]]";
  gchar **parsed_rut = g_new0 (gchar *, 2);
  GRegex *regex = g_regex_new (pattern_clean, 0, 0, NULL);
  size_t str_len = 0;

  if (strlen (rut) == 0) return parsed_rut;

  rut = g_regex_replace (regex, rut, -1, 0, "", G_REGEX_MATCH_NOTEMPTY, NULL);

  str_len = strlen (rut);

  parsed_rut[0] = g_strndup (rut, (gsize)(str_len - 1));
  parsed_rut[1] = g_strdup_printf("%c", rut[str_len-1]);

  return parsed_rut;
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

/**
 * Register log in/out of a user from the system
 *
 * @param User info
 * @param TRUE if the user will login, FALSE if he will logout.
 */
gboolean
log_register_access (User *info_user, gboolean login)
{
  gint machine = rizoma_get_value_int ("MAQUINA");
  gint seller = info_user->user_id;
  gchar *query;
  PGresult *res;

  if (login)
    {
      query = g_strdup_printf ("insert into log (id, fecha, maquina, seller, text) values (DEFAULT, NOW(), %d, %d, 'Login')", machine, seller);
    }
  else
    {
      query = g_strdup_printf ("insert into log (id, fecha, maquina, seller, text) values (DEFAULT, NOW(), %d, %d, 'Logout')", machine, seller);
    }

  res = EjecutarSQL (query);
  g_free (query);

  return res != NULL ? TRUE : FALSE;
}
