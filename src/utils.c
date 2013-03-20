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
#include<locale.h>

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
  
  for (i = 0; i < len; i++)
    if (g_ascii_isdigit (string[i]) == FALSE)
      return TRUE;

  return FALSE;
}


gboolean
is_numeric (gchar *string)
{
  gint i, len = strlen (string);
  gint points = 0;

  // Si solamente es un '.' o ',' se toma como texto
  if (len == 1 && (string[0] == '.' || string[0] == ','))
    return FALSE;

  //Si tiene un caracter distinto a un numero, '.' o ',' y además
  //hay más de un '.' o ',' se considera texto
  for (i = 0; i < len; i++)
    {
      if (string[i] == ',' || string[i] == '.')
	points++;
      
      if (points > 1)
	return FALSE;

      if (g_ascii_isdigit (string[i]) == FALSE &&
	  string[i] != ',' && string[i] != '.')
        return FALSE;
    }

  return TRUE;
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
  gchar *decimales, *l_number;
  int len;
  gint points = 0, position = 0;
  gint i, unidad = 0, point = 0;
  
  /*separador de miles, separador de decimales*/
  gchar *sm, *sd;

  struct lconv *locale  = localeconv ();
  sd = locale->mon_decimal_point;
  sm = locale->mon_thousands_sep;

  if (number == NULL)
    return "";

  len = strlen (number);

  if (len <= 3)
    return number;

  if ((len % 3) != 0)
    points = len / 3;
  else
    points = (len / 3) - 1;

  //Se buscan puntos decimales
  for (i = len; i >= 0; i--)
    if (number[i] == '.' || number[i] == ',')
      {
	point++;
	position=i;
      }
  
  //Si tiene mas de un '.' o ',' se retorna tal cual 
  if (point > 1)
    return number;

  //Si es un decimal se retorna el entero con los puntos, la coma decimal y los decimales
  if (point == 1)
    {
      decimales = invested_strndup (number, position+1);
      l_number = PutPoints (g_strndup (number, position));
      l_number = g_strconcat (l_number, sd, decimales, NULL);
      return l_number;
    }

  //Si no hay puntos decimales que afecten, se le ponen puntos al entero
  for (i = len; i >= 0; i--)
    {
      if (unidad == 3 && point < points && number[i] != '.' && number[i] != ',')
        {
          g_snprintf (alt, 15, "%s%c%s", sm, number[i], alt3);
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
  gchar buf[20];
  GType column_type = gtk_tree_model_get_column_type (model, column);

  switch (column_type)
    {
    case G_TYPE_DOUBLE:
      {
        gdouble number;
        gtk_tree_model_get(model, iter, column, &number, -1);
        g_snprintf (buf, sizeof (buf), "%.2f", number);
        g_object_set(renderer, "text", PutPoints (buf), NULL);
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


gchar *
formato_rut (gchar *rut)
{
  gboolean primero;
  gint largo, contador;
  gchar *rut_format;

  largo = strlen (rut);
  primero = FALSE;
  contador = 0;

  do {
    if (primero == FALSE) {
      rut_format = g_strdup_printf ("-%c", rut[largo-1]);
      primero = TRUE;
    } else {
      if (contador == 3) {
	rut_format = g_strdup_printf ("%c.%s", rut[largo-1], rut_format);
	contador = 0;
      } else {
	rut_format = g_strdup_printf ("%c%s", rut[largo-1], rut_format);
      }
      contador++;
    }
    largo--;	    
  } while (largo > 0);

  return rut_format;
}

void
control_rut (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model,
	     GtkTreeIter *iter, gpointer user_data)
{
  gint column = (gint) user_data;
  gchar buf[20];
  GType column_type = gtk_tree_model_get_column_type (model, column);

  switch (column_type)
    {
    case G_TYPE_STRING:
      {
        gchar *rut, *rut_format;

        gtk_tree_model_get (model, iter, column, &rut, -1);
	rut_format = formato_rut (rut);

        g_snprintf (buf, sizeof (buf), "%s", rut_format);
        g_object_set (renderer, "text", buf, NULL);
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
  guint *context_id = NULL;

  //TODO: g_object_get_data retorna un gpointer... ver implicancias de ello
  context_id = g_object_get_data (G_OBJECT(statusbar), "context_id");

  if (context_id != NULL)
    {
      gtk_statusbar_pop (GTK_STATUSBAR(statusbar), *context_id);
      //g_free se comenta puesto que provoca una caída.
      //g_free (context_id);
    }

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
  //guint message_id;

  context_id = g_malloc(sizeof(guint));

  *context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR(statusbar), "rizoma-compras");
  gtk_statusbar_push (statusbar, *context_id, text);

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
      if (!GTK_IS_TREE_VIEW (list->data) && GTK_IS_CONTAINER (list->data))
        {
	  if (!validate_string ("Button", g_strdup (gtk_widget_get_name (list->data))))
	    {
	      clean_container (GTK_CONTAINER (list->data));
	    }
        }
      else
        {
          widget = GTK_WIDGET (list->data);
          widget_name = g_strdup (gtk_buildable_get_name (GTK_BUILDABLE (widget)));
	  if (widget_name != NULL && !validate_string ("[0-9]+", widget_name) && !validate_string ("Gtk", widget_name))
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
CurrentDate (int tipo)
{
  time_t t;
  struct tm *fecha;

  time (&t);

  fecha = localtime (&t);
  if(tipo == 0)
    return g_strdup_printf ("%.2d-%.2d-%.2d", fecha->tm_mday, fecha->tm_mon+1, YEAR (fecha->tm_year));
  else
    return g_strdup_printf ("%.2d/%.2d/%.2d", fecha->tm_mday, fecha->tm_mon+1, YEAR (fecha->tm_year-2000));
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


/**
 * Selecciona la fila anterior (de un treeview) a la eliminada
 * de no haber un elemento anterior selecciona el Ãºltimo disponible
 *
 * @param Nombre del treeview
 * @param Numero de la fila eliminada
 *
 * TODO: Ver la posibilidad de unificar las funciones que son llamadas por
 * botones para eliminar productos seleccionados, ver eliminarDeLista
 *
 */
void
select_back_deleted_row (gchar *treeViewName, gint deletedRowPosition)
{
  GtkTreeView *tree = GTK_TREE_VIEW(gtk_builder_get_object(builder, treeViewName));

  if (deletedRowPosition > 0)
    gtk_tree_selection_select_path (gtk_tree_view_get_selection(tree),
				    gtk_tree_path_new_from_string(g_strdup_printf("%d", deletedRowPosition - 1)));
  else
    gtk_tree_selection_select_path (gtk_tree_view_get_selection(tree),
				    gtk_tree_path_new_from_string(g_strdup_printf("%d", deletedRowPosition)));
  /* Si se elimina el último elemento de la lista que se seleccione el último disponible */
}


/**
 * return a gchar* from index number (from string)
 * to end, also could be understand as invested g_strndup.
 *
 * @param texto, text to cut
 * @param index, initiation number index to cut.
 */
gchar *
invested_strndup (const gchar *texto, gint index)
{
  gchar *texto_local = g_strdup (texto);
  index = strlen (texto_local) - index;
  texto_local = g_strreverse (g_strndup (g_strreverse (texto_local), index));
  return texto_local;
}

/**
 * Returns the treeview length that you specified
 *
 * @param GtkTreeView treeview: The treeview point
 * @return gint length: the treeview length
 */
gint
get_treeview_length (GtkTreeView *treeview)
{
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  GtkTreeIter iter;
  gboolean valid;
  gint length = 0;

  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid)
    {
      length++;
      valid = gtk_tree_model_iter_next (model, &iter);
    }
  return length;
}


/**
 * This function divides the code into fragments corresponding
 *
 * @param: gchar *clothes_code: The clothes code
 */
GArray *
decode_clothes_code (gchar *clothes_code)
{
  gchar *fragmento;
  GArray *decode;
  gint i = 0;

  decode = g_array_new (FALSE, FALSE, sizeof (gchar*));

  if (strlen (clothes_code) != 16)
    return NULL;

  do
    {
      if (i==1 || i==3)
	{
	  fragmento = g_strndup (clothes_code, 3);
	  g_array_append_val (decode, fragmento);
	  clothes_code = invested_strndup (clothes_code, 3);
	}
      else
	{
	  fragmento = g_strndup (clothes_code, 2);
	  g_array_append_val (decode, fragmento);
	  clothes_code = invested_strndup (clothes_code, 2);
	}

      printf ("extraido: %s restante: %s \n", g_array_index (decode, gchar*, i), clothes_code);
      i++;
    } while (strlen (clothes_code) != 0);

  return decode;
}


/**
 * This function only allows the insertion 
 * of integer numerical values in the specified entry
 * ("insert-text" signal)
 * 
 * The "insert-text" signal of GtkEditable (from corresponding GtkEntry)
 * must be hooked to this function.
 *
 * @param: GtkEditable *editable: the object which received the signal.-
 * @param: gchar *new_text: the new text to insert.-
 * @param: gint new_text_length: the length of the new text, in bytes, 
 *         or -1 if new_text is nul-terminated.-
 * @param: gint *position: the position, in characters, at which to insert the new text. 
 *         this is an in-out parameter. After the signal emission is finished,
 *         it should point after the newly inserted text.-
 * @param: gpointer data: user data set when the signal handler was connected.-
 */
void
only_numberi_filter (GtkEditable *editable,
		     gchar *new_text,
		     gint new_text_length,
		     gint *position,
		     gpointer user_data)
{
  gchar *result;
  
  if (HaveCharacters (new_text))
    {
      new_text_length = new_text_length-1;
      result = g_strndup (new_text, new_text_length);
    }
  else
    result = g_strndup (new_text, new_text_length);

  g_signal_handlers_block_by_func (editable,
				   (gpointer) only_numberi_filter, user_data);
  gtk_editable_insert_text (editable, result, new_text_length, position);
  g_signal_handlers_unblock_by_func (editable,
                                     (gpointer) only_numberi_filter, user_data);
  g_signal_stop_emission_by_name (editable, "insert_text");
  g_free (result);
}


/**
 * This function only allows the insertion of integer and decimal 
 * values in the specified entry ("insert-text" signal)
 * 
 * The "insert-text" signal of GtkEditable (from corresponding GtkEntry)
 * must be hooked to this function.
 *
 * @param: GtkEditable *editable: the object which received the signal.-
 * @param: gchar *new_text: the new text to insert.-
 * @param: gint new_text_length: the length of the new text, in bytes, 
 *         or -1 if new_text is nul-terminated.-
 * @param: gint *position: the position, in characters, at which to insert the new text. 
 *         this is an in-out parameter. After the signal emission is finished,
 *         it should point after the newly inserted text.-
 * @param: gpointer data: user data set when the signal handler was connected.-
 */
void
only_numberd_filter (GtkEditable *editable,
		     gchar *new_text,
		     gint new_text_length,
		     gint *position,
		     gpointer user_data)
{
  gchar *result, *texto_actual;
  
  texto_actual = g_strdup (gtk_entry_get_text (GTK_ENTRY (editable)));
  texto_actual = g_strconcat (texto_actual, new_text, NULL);
  if (!is_numeric (texto_actual))
    {
      new_text_length = new_text_length-1;
      result = g_strndup (new_text, new_text_length);
    }
  else
    result = g_strndup (new_text, new_text_length);

  g_signal_handlers_block_by_func (editable,
				   (gpointer) only_numberd_filter, user_data);
  gtk_editable_insert_text (editable, result, new_text_length, position);
  g_signal_handlers_unblock_by_func (editable,
                                     (gpointer) only_numberd_filter, user_data);
  g_signal_stop_emission_by_name (editable, "insert_text");
  g_free (result);
}

/**
 * This function add a filter on all entry widgets (that meet criteria)
 * from a container specified to accept only number on them.
 *
 * @param container: The container to apply filters
 */
void
only_number_filer_on_container (GtkContainer *container)
{
  GList *list = gtk_container_get_children (container);
  GtkWidget *widget;
  gchar *widget_name = NULL;

  while (list != NULL)
    {
      widget_name = g_strdup (gtk_buildable_get_name (GTK_BUILDABLE (list->data)));
      if (GTK_IS_CONTAINER (list->data) && !GTK_IS_TREE_VIEW (list->data) && !GTK_IS_BUTTON (list->data))
	only_number_filer_on_container (GTK_CONTAINER (list->data));
      else
        {
          widget = GTK_WIDGET (list->data);
          widget_name = g_strdup (gtk_buildable_get_name (GTK_BUILDABLE (widget)));
	  if (widget_name != NULL && !validate_string ("[0-9]+", widget_name) && !validate_string ("Gtk", widget_name))
            {
              if (GTK_IS_ENTRY (widget))
                {
		  if (validate_string ("entry_int", widget_name))
		    g_signal_connect (G_OBJECT (builder_get (builder, widget_name)), "insert-text",
				      G_CALLBACK (only_numberi_filter), NULL);
		  else if (validate_string ("entry_dec", widget_name))
		    g_signal_connect (G_OBJECT (builder_get (builder, widget_name)), "insert-text",
				      G_CALLBACK (only_numberd_filter), NULL);
                }
            }
          g_free (widget_name);
        }

      list = g_list_next (list);
    }

  g_list_free (list);
}


/**
 * Sum all column data from treeview specified
 *
 * @param: Treeview from get the column
 * @param: Column from which data are collected to sum
 * @param: Tipo de columna
 */
gdouble
sum_treeview_column (GtkTreeView *treeview, gint column, GType type)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
 
  gdouble sum;
  gboolean valid;

  gint number_i;
  gdouble number_d;

  void *number;

  model = gtk_tree_view_get_model (treeview);
  valid = gtk_tree_model_get_iter_first (model, &iter);

  number_i = number_d = sum = 0;

  if (type == G_TYPE_INT) number = &number_i;
  else if (type == G_TYPE_DOUBLE) number = &number_d;
  else return 0;

  //Se obtiene el valor de la columna 'column' para sumarlo incrementalmente
  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  column, number, //(type == G_TYPE_INT) ? &number_i : &number_d;
			  -1);
      sum += (type == G_TYPE_INT) ? (gdouble)number_i : number_d;
      valid = gtk_tree_model_iter_next (model, &iter);
    }

  return sum;
}


/**
 * Compare data from colum specified with 'data_to_compare'
 *
 * @param: Treeview from get the column
 * @param: Column number with which 'data_to_compare' will be compared
 * @param: Tipo de columna
 */
gboolean
compare_treeview_column (GtkTreeView *treeview, gint column, GType type, void *data_to_compare)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
 
  gboolean valid;

  gint data_i;
  gdouble data_d;
  gboolean data_g;
  gchar *data_t;
  void *data_get;

  model = gtk_tree_view_get_model (treeview);
  valid = gtk_tree_model_get_iter_first (model, &iter);

  if (type == G_TYPE_INT) data_get = &data_i;
  else if (type == G_TYPE_DOUBLE) data_get = &data_d;
  else if (type == G_TYPE_BOOLEAN) data_get = &data_g;
  else if (type == G_TYPE_STRING) data_get = &data_t;
  else return FALSE;

  //Se obtiene el valor de la columna 'column' para compararlo con 'data_to_compare'
  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  column, (type == G_TYPE_STRING) ? &data_get : data_get,
			  -1);

      if (type == G_TYPE_INT) {
	if ((gint *)data_get == (gint *)data_to_compare)
	  return TRUE;

      } else if (type == G_TYPE_DOUBLE) {
	if ((gdouble *)data_get == (gdouble *)data_to_compare)
	  return TRUE;

      } else if (type == G_TYPE_BOOLEAN) {
	if ((gboolean *)data_get == (gboolean *)data_to_compare)
	  return TRUE;

      } else if (type == G_TYPE_STRING) {
	if (g_str_equal ((gchar *)data_get, (gchar *)data_to_compare))
	  return TRUE;
      }

      valid = gtk_tree_model_iter_next (model, &iter);
    }

  return FALSE;
}


/**
 * Compare data from colum specified with 'data_to_compare'
 * and return the row matched position.
 *
 * @param: Treeview from get the column
 * @param: Column number with which 'data_to_compare' will be compared
 * @param: Tipo de columna
 * @return: Iter matched
 */
gboolean
get_treeview_column_matched (GtkTreeView *treeview, gint column, GType type, void *data_to_compare, GtkTreeIter *iter_ext)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
 
  gboolean valid;

  gint data_i;
  gdouble data_d;
  gchar *data_t;
  void *data_get;

  model = gtk_tree_view_get_model (treeview);
  valid = gtk_tree_model_get_iter_first (model, &iter);

  if (type == G_TYPE_INT) data_get = &data_i;
  else if (type == G_TYPE_DOUBLE) data_get = &data_d;
  else if (type == G_TYPE_STRING) data_get = &data_t;
  else return FALSE;

  //Se obtiene el valor de la columna 'column' para compararlo con 'data_to_compare'
  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  column, (type == G_TYPE_STRING) ? &data_get : data_get,
			  -1);

      if (type == G_TYPE_INT) {
	if ((gint *)data_get == (gint *)data_to_compare)
	  {*iter_ext = iter; return TRUE;}

      } else if (type == G_TYPE_DOUBLE) {
	if ((gdouble *)data_get == (gdouble *)data_to_compare)
	  {*iter_ext = iter; return TRUE;}

      } else if (type == G_TYPE_STRING) {
	if (g_str_equal ((gchar *)data_get, (gchar *)data_to_compare))
	  {*iter_ext = iter; return TRUE;}
      }

      valid = gtk_tree_model_iter_next (model, &iter);
    }

  return FALSE;
}


/**
 * Fill Taxes combobox
 *
 * @param: combo: Combobox to fill
 * @param: id_seleccion: Item to select default
 */
void
fill_combo_impuestos (GtkComboBox *combo, gint id_seleccion)
{
  GtkListStore *store;
  PGresult *res;
  GtkCellRenderer *cell;

  store = GTK_LIST_STORE (gtk_combo_box_get_model (combo));

  /*Combobox impuestos*/
  if (store == NULL)
    {
      store = gtk_list_store_new (3,
				  G_TYPE_INT,    //0 id
				  G_TYPE_STRING, //1 descripcion
				  G_TYPE_DOUBLE);//2 monto

      gtk_combo_box_set_model (combo, GTK_TREE_MODEL (store));

      cell = gtk_cell_renderer_text_new ();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell,
				      "text", 1,
				      NULL);  
    }
  else
    gtk_list_store_clear (store);

  res = EjecutarSQL ("SELECT * FROM select_otros_impuestos() ORDER BY id ASC");
  if (res != NULL)
    {
      GtkTreeIter iter;
      gint i, tuples, active;
	  
      i = tuples = active = 0;
      tuples = PQntuples (res);
      for (i = 0; i < tuples; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, atoi (PQvaluebycol (res, i, "id")),
			      1, PQvaluebycol (res, i, "descripcion"),
			      2, strtod (PUT (PQvaluebycol (res, i, "monto")), (char **)NULL),
			      -1);
	  if (atoi (PQvaluebycol (res, i, "id")) == id_seleccion) active = i;
	}
      
      gtk_combo_box_set_active (combo, active != -1 ? active : 0);
    }
}

/**
 * Fill Unit Combobox
 *
 * @param: combo: Combobox to fill
 * @param: id_seleccion: Item to select default
 */
void
fill_combo_unidad (GtkComboBox *combo, gchar *nombre_unidad)
{
  GtkListStore *store;
  PGresult *res;
  GtkCellRenderer *cell;

  store = GTK_LIST_STORE (gtk_combo_box_get_model (combo));

  /*Combobox Unidad*/
  if (store == NULL)
    {
      store = gtk_list_store_new (2,
				  G_TYPE_INT,
				  G_TYPE_STRING);

      gtk_combo_box_set_model (GTK_COMBO_BOX (combo), GTK_TREE_MODEL (store));

      cell = gtk_cell_renderer_text_new ();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell,
				      "text", 1,
				      NULL);
    }
  else
    gtk_list_store_clear (store);

  res = EjecutarSQL ("SELECT * FROM unidad_producto ORDER BY id ASC");
  if (res != NULL)
    {
      GtkTreeIter iter;
      gint i, tuples, active;

      i = tuples = active = 0;
      tuples = PQntuples (res);

      for (i=0 ; i < tuples ; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, atoi (PQvaluebycol (res, i, "id")),
			      1, PQvaluebycol (res, i, "descripcion"),
			      -1);
	  if (g_str_equal (PQvaluebycol (res, i, "descripcion"), nombre_unidad)) active = i;
	}
      gtk_combo_box_set_active (combo, !g_str_equal (nombre_unidad, "") ? active : 0);
    }
}


/**
 * Fill document type combo - to choose between ('FACTURA', 'GUIA')
 *
 * @param: combo: Combobox to fill
 * @param: id_seleccion: Item to select default
 */
void
fill_combo_tipo_documento (GtkComboBox *combo, gint id_seleccion)
{
  GtkListStore *store;
  GtkCellRenderer *cell;
  GtkTreeIter iter;

  store = GTK_LIST_STORE (gtk_combo_box_get_model (combo));

  /*Combobox familias*/
  if(store == NULL)
    {      
      store = gtk_list_store_new (2,
				  G_TYPE_INT,
				  G_TYPE_STRING);

      gtk_combo_box_set_model (GTK_COMBO_BOX (combo), GTK_TREE_MODEL (store));

      cell = gtk_cell_renderer_text_new();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), cell, TRUE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), cell,
				      "text", 1,
				      NULL);
    }
  else
    gtk_list_store_clear (store);

  // Factura
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
		      0, FACTURA,
		      1, "FACTURA",
		      -1);

  // GUIA
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
		      0, GUIA,
		      1, "GUIA",
		      -1);

  gtk_combo_box_set_active (combo, id_seleccion != -1 ? id_seleccion : 0);
}


/**
 * Fill Family Combobox
 *
 * @param: combo: Combobox to fill
 * @param: id_seleccion: Item to select default
 */
void
fill_combo_familias (GtkComboBox *combo, gint id_seleccion)
{
  GtkListStore *store;
  PGresult *res;
  GtkCellRenderer *cell;

  store = GTK_LIST_STORE (gtk_combo_box_get_model (combo));

  /*Combobox familias*/
  if(store == NULL)
    {      
      store = gtk_list_store_new (2,
				  G_TYPE_INT,
				  G_TYPE_STRING);

      gtk_combo_box_set_model (GTK_COMBO_BOX (combo), GTK_TREE_MODEL (store));

      cell = gtk_cell_renderer_text_new();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), cell, TRUE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), cell,
				      "text", 1,
				      NULL);
    }
  else
    gtk_list_store_clear (store);

  res = EjecutarSQL ("SELECT * FROM familias ORDER BY id ASC");
  if (res != NULL)
    {
      GtkTreeIter iter;
      gint i, tuples, active;
      
      i = tuples = active = 0;
      tuples = PQntuples (res);
      
      for (i=0 ; i < tuples ; i++)
	{
	  gtk_list_store_append (store, &iter);
	  gtk_list_store_set (store, &iter,
			      0, atoi (PQvaluebycol (res, i, "id")),
			      1, PQvaluebycol (res, i, "nombre"),
			      -1);
	  if (atoi (PQvaluebycol (res, i, "id")) == id_seleccion) active = i;
	}
      gtk_combo_box_set_active (combo, active != -1 ? active : 0);
    }
}


/**
 * This function copy the range index of string into
 * other string.
 *
 * @param: String to store result
 * @param: String from copy text
 * @param: Initial index (to copy) of the string
 * @param: End index (to copy) of the string
 */
void
strdup_string_range (gchar *result, gchar *string, gint inicio, gint final)
{
  //Validacion inicial
  if (inicio>final || string == NULL || strlen(string)<final ||
      inicio<0 || final<0)
    return;

  /*El largo total de la cadena más el '\0' del final*/
  gchar range_string[(final-inicio+2)];
  gint i, j; 

  i=j=0;
  while (string[i] != '\0') {
    if (i>=inicio && i<=final)
      range_string[j++]=string[i];
    if (i==final)
      range_string[j]='\0';
    i++;
  }

  strcpy (result, range_string);
}


/**
 * This function copy the range index of string into
 * other string.
 *
 * @param: String to store result
 * @param: String from copy text
 * @param: Initial index (to copy) of the string
 * @param: End index (to copy) of the string
 * @param: Number of decimal point
 */
void
strdup_string_range_with_decimal (gchar *result, gchar *string, gint inicio, gint final, gint decimal)
{
  //Validacion inicial
  if (inicio>final || string == NULL || strlen(string)<final || decimal>=final ||
      inicio<0 || final<1 || decimal<1)
    return;

  /*El largo total de la cadena más el decimal ',' y '\0' del final*/
  gchar range_string[(final-inicio+3)];
  gint i, j; 

  i=j=0;
  while (string[i] != '\0') {
    if (i>=inicio && i<=final)
      range_string[j++]=string[i];
    if (j==decimal)
      range_string[j++]=',';
    if (i==final)
      range_string[j]='\0';
    i++;
  }

  strcpy (result, range_string);
}


void
show_clean_window (GtkWindow *window)
{
  clean_container (GTK_CONTAINER (window));
  gtk_widget_show(window);
}
