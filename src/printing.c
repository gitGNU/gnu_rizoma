/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
/*printing.c
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

#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>

#include"tipos.h"

#include"config_file.h"
#include"postgres-functions.h"
#include"printing.h"

#include"utils.h"

int
LaunchApp (const gchar *file)
{
  gchar *spreadsheet_app;
  spreadsheet_app = rizoma_get_value ("SPREADSHEET_APP");
  system (g_strdup_printf ("LANG=es_CL.UTF-8 %s \"%s\" &", spreadsheet_app, file));
  return 0;
}

void
PrintTree (GtkWidget *widget, gpointer data)
{
  Print *print;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreeIter son;
  GType column_type;
  gint columns;
  gint i;
  gint cols = 0;
  gchar *temp_directory;
  gchar *file;
  FILE *fp;

  print = (Print *) data;
  model = gtk_tree_view_get_model (print->tree);
  columns = gtk_tree_model_get_n_columns (model);
  temp_directory = rizoma_get_value ("TEMP_FILES");


  if (!(gtk_tree_model_get_iter_first (model, &iter)))
    {
      g_printerr("%s: could not be obtained the iter for the first element", G_STRFUNC);
      return;
    }

  //what for is this?
  while (print->cols[cols].name != NULL)
    cols++;

  /* Si es NULL */
  if (print->date_string == NULL)
    print->date_string = CurrentDate(0); /* Asumimos la fecha actual */

  file = g_strdup_printf ("%s/informe-%s.csv", temp_directory, print->date_string);

  fp = fopen (file, "w");

  if (fp == NULL)
    {
      perror (g_strdup_printf ("Opening %s", file));
      return;
    }
  else
    g_printerr ("Working on %s\n", file);

  fprintf (fp, "%s,,,,\n%s\n", print->title, print->date_string);

  for (i = 0; i < cols; i++)
    {
      fprintf (fp, "\"%s\",", print->cols[i].name);
    }

  fprintf (fp, ",,,,\n");

  do
    {
      for (i = 0; i < columns; i++)
        {
          column_type = gtk_tree_model_get_column_type (model, i);

          switch (column_type)
            {
            case G_TYPE_STRING:
              {
                gchar *value_char;
                gtk_tree_model_get (model, &iter,
                                    i, &value_char,
                                    -1);
                if (value_char != NULL)
                  fprintf (fp, "\"%s\",", value_char);
                else
                  fprintf (fp, ",");
              }
              break;
            case G_TYPE_INT:
              {
                gint value_int;
                gtk_tree_model_get (model, &iter,
                                    i, &value_int,
                                    -1);

                fprintf (fp, "\"%d\",", value_int);
              }
              break;
            case G_TYPE_DOUBLE:
              {
                gdouble value_double;
                gtk_tree_model_get (model, &iter,
                                    i, &value_double,
                                    -1);
                fprintf (fp, "\"%s\",", PUT (g_strdup_printf ("%.2f", value_double)));
              }
              break;
            }
        }
      fprintf (fp, "\n");

      if (gtk_tree_model_iter_has_child (model, &iter) == TRUE)
        {
          gtk_tree_model_iter_children (model, &son, &iter);

          do {

            for (i = 0; i < columns; i++)
              {
                column_type = gtk_tree_model_get_column_type (model, i);

                switch (column_type)
                  {
                  case G_TYPE_STRING:
                    {
                      gchar *value_char;
                      gtk_tree_model_get (model, &son,
                                          i, &value_char,
                                          -1);
                      if (value_char != NULL)
                        fprintf (fp, "\"%s\",", value_char);
                      else
                        fprintf (fp, ",");
                    }
                    break;
                  case G_TYPE_INT:
                    {
                      gint value_int;
                      gtk_tree_model_get (model, &son,
                                          i, &value_int,
                                          -1);
                      fprintf (fp, "\"%d\",", value_int);
                    }
                    break;
                  case G_TYPE_DOUBLE:
                    {
                      gdouble value_double;
                      gtk_tree_model_get (model, &son,
                                          i, &value_double,
                                          -1);
                      fprintf (fp, "\"%s\",", PUT (g_strdup_printf ("%.2f", value_double)));
                    }
                    break;
                  }  column_type = gtk_tree_model_get_column_type (model, i);

              }
            fprintf (fp, "\n");
          } while ((gtk_tree_model_iter_next (model, &son)) != FALSE);
        }
      fprintf (fp, "\n");
    } while ((gtk_tree_model_iter_next (model, &iter)) != FALSE);

  fclose (fp);

  LaunchApp (file);
}

/*
  This function will print two tree view, one will contain the
  father and the other will contain the son
*/
void
PrintTwoTree (GtkWidget *widget, gpointer data)
{
  Print *print = (Print *) data;
  GtkTreeModel *model_father = gtk_tree_view_get_model (print->tree);
  GtkTreeModel *model_son = gtk_tree_view_get_model (print->son->tree);
  GtkTreeIter iter_father, iter_son;
  GType column_type;
  gint i, j;
  gint father_cols = 0, son_cols = 0;
  gchar *temp_directory = rizoma_get_value ("TEMP_FILES");
  gchar *file;
  FILE *fp;

  if (gtk_tree_model_get_iter_first (model_father, &iter_father) == FALSE)
    return;

  while (print->cols[father_cols].name != NULL)
    father_cols++;

  while (print->son->cols[son_cols].name != NULL)
    son_cols++;

  /* Si es NULL */
  if (print->date_string == NULL)
    print->date_string = CurrentDate(0); /* Asumismos la fecha actual */

  file = g_strdup_printf ("%s/informe-%s-%s.csv", temp_directory,
                          print->name, print->date_string);

  fp = fopen (file, "w");

  if (fp == NULL)
    {
      perror (g_strdup_printf ("Opening %s", file));
      return;
    }
  else
    printf ("Working on %s\n", file);

  fprintf (fp, "%s\n%s\n\n", print->title, print->date_string);

  do
    {
      //Se agrega la cabecera de las columnas del treeview padre
      for (i = 0; i < father_cols; i++)
	fprintf (fp, "\"%s\",", print->cols[i].name);
      fprintf (fp, ",,,,\n");

      for (i = 0; i < father_cols; i++)
        {
          column_type = gtk_tree_model_get_column_type (model_father, print->cols[i].num);

          switch (column_type)
            {
            case G_TYPE_STRING:
              {
                gchar *value_char;
                gtk_tree_model_get (model_father, &iter_father,
                                    print->cols[i].num, &value_char,
                                    -1);
                fprintf (fp, "%s,", CutPoints (value_char));
              }
              break;
            case G_TYPE_INT:
              {
                gint value_int;
                gtk_tree_model_get (model_father, &iter_father,
                                    print->cols[i].num, &value_int,
                                    -1);
                fprintf (fp, "%d,", value_int);
              }
              break;
            case G_TYPE_DOUBLE:
              {
                gdouble value_double;
                gtk_tree_model_get (model_father, &iter_father,
                                    print->cols[i].num, &value_double,
                                    -1);
                fprintf (fp, "\"%s\",", PUT (g_strdup_printf ("%.2f", value_double)));
              }
              break;
            }
        }

      fprintf (fp, "\n\n");

      gtk_tree_selection_select_iter (gtk_tree_view_get_selection (print->tree), &iter_father);
      gtk_tree_view_row_activated (print->tree, gtk_tree_model_get_path (model_father, &iter_father), 
				   gtk_tree_view_get_column (print->tree, 0));

      if (gtk_tree_model_get_iter_first (model_son, &iter_son) == FALSE)
	{
	  printf ("No se seleccionó una fila del treeview hijo, puede ser que no haya información\n");
	  return;
	}

      //Se agrega la cabecera de las columnas del treeview hijo
      fprintf (fp, ",");
      for (j = 0; j < son_cols; j++)
      	fprintf (fp, "\"%s\",", print->son->cols[j].name);
      fprintf (fp, ",,,,\n");

      //Se insertan los datos del treeview hijo
      do
        {
          fprintf (fp, ",");

          for (j = 0; j < son_cols; j++)
            {
              column_type = gtk_tree_model_get_column_type
                (model_son, print->son->cols[j].num);

              switch (column_type)
                {
                case G_TYPE_STRING:
                  {
                    gchar *value_char = NULL;

                    gtk_tree_model_get (model_son, &iter_son,
                                        print->son->cols[j].num, &value_char,
                                        -1);

                    fprintf (fp, "\"%s\",", value_char);
                  }
                  break;
                case G_TYPE_INT:
                  {
                    gint value_int;
                    gtk_tree_model_get (model_son, &iter_son,
                                        print->son->cols[j].num, &value_int,
                                        -1);
                    fprintf (fp, "%d,", value_int);
                  }
                  break;
                case G_TYPE_DOUBLE:
                  {
                    gdouble value_double;
                    gtk_tree_model_get (model_son, &iter_son,
                                        print->son->cols[j].num, &value_double,
                                        -1);
                    fprintf (fp, "\"%s\",", PUT (g_strdup_printf ("%.2f", value_double)));
                  }
                  break;
                }
            }

          fprintf (fp, "\n");
        } while ((gtk_tree_model_iter_next (model_son, &iter_son)) != FALSE);

      fprintf (fp, "\n\n");
    } while ((gtk_tree_model_iter_next (model_father, &iter_father)) != FALSE);

  fclose (fp);

  LaunchApp (file);
}


void
PrintThreeTree (GtkWidget *widget, gpointer data)
{
  Print *print = (Print *) data;
  GtkTreeModel *model_father = gtk_tree_view_get_model (print->tree);
  GtkTreeModel *model_son = gtk_tree_view_get_model (print->son->tree);
  GtkTreeModel *model_grandson = gtk_tree_view_get_model (print->son->son->tree);
  GtkTreeIter iter_father, iter_son, iter_grandson;
  GType column_type;
  gint i, j, k;
  gint father_cols = 0, son_cols = 0, grandson_cols = 0;
  gchar *temp_directory = rizoma_get_value ("TEMP_FILES");
  gchar *file;
  FILE *fp;

  if (gtk_tree_model_get_iter_first (model_father, &iter_father) == FALSE)
    return;

  while (print->cols[father_cols].name != NULL)
    father_cols++;

  while (print->son->cols[son_cols].name != NULL)
    son_cols++;

  while (print->son->son->cols[grandson_cols].name != NULL)
    grandson_cols++;

  /* Si es NULL */
  if (print->date_string == NULL)
    print->date_string = CurrentDate(0); /* Asumismos la fecha actual */

  file = g_strdup_printf ("%s/informe-%s-%s.csv", temp_directory,
                          print->name, print->date_string);

  fp = fopen (file, "w");

  if (fp == NULL)
    {
      perror (g_strdup_printf ("Opening %s", file));
      return;
    }
  else
    printf ("Working on %s\n", file);

  fprintf (fp, "%s\n%s\n\n", print->title, print->date_string);

  do    
    {
      //Se agrega la cabecera de las columnas del treeview padre
      for (i = 0; i < father_cols; i++)
	fprintf (fp, "\"%s\",", print->cols[i].name);
      fprintf (fp, "\n");

      for (i = 0; i < father_cols; i++)
        {
          column_type = gtk_tree_model_get_column_type (model_father, print->cols[i].num);

          switch (column_type)
            {
            case G_TYPE_STRING:
              {
                gchar *value_char;
                gtk_tree_model_get (model_father, &iter_father,
                                    print->cols[i].num, &value_char,
                                    -1);
                fprintf (fp, "%s,", CutPoints (value_char));
              }
              break;
            case G_TYPE_INT:
              {
                gint value_int;
                gtk_tree_model_get (model_father, &iter_father,
                                    print->cols[i].num, &value_int,
                                    -1);
                fprintf (fp, "%d,", value_int);
              }
              break;
            case G_TYPE_DOUBLE:
              {
                gdouble value_double;
                gtk_tree_model_get (model_father, &iter_father,
                                    print->cols[i].num, &value_double,
                                    -1);
                fprintf (fp, "\"%s\",", PUT (g_strdup_printf ("%.2f", value_double)));
              }
              break;
            }
        }

      fprintf (fp, "\n\n");

      gtk_tree_selection_select_iter (gtk_tree_view_get_selection (print->tree), &iter_father);
      gtk_tree_view_row_activated (print->tree, gtk_tree_model_get_path (model_father, &iter_father), 
				   gtk_tree_view_get_column (print->tree, 0));

      if (gtk_tree_model_get_iter_first (model_son, &iter_son) == FALSE)
	{
	  printf ("No se seleccionó una fila del treeview hijo, puede ser que no haya información\n");
	  return;
	}

      //Se agrega la cabecera de las columnas del treeview hijo
      //fprintf (fp, ",");
      for (j = 0; j < son_cols; j++)
      	fprintf (fp, "\"%s\",", print->son->cols[j].name);
      fprintf (fp, "\n");

      //Se insertan los datos del treeview hijo
      do
        {
          //fprintf (fp, ",");

          for (j = 0; j < son_cols; j++)
            {
              column_type = gtk_tree_model_get_column_type
                (model_son, print->son->cols[j].num);

              switch (column_type)
                {
                case G_TYPE_STRING:
                  {
                    gchar *value_char = NULL;

                    gtk_tree_model_get (model_son, &iter_son,
                                        print->son->cols[j].num, &value_char,
                                        -1);

                    fprintf (fp, "\"%s\",", value_char);
                  }
                  break;
                case G_TYPE_INT:
                  {
                    gint value_int;
                    gtk_tree_model_get (model_son, &iter_son,
                                        print->son->cols[j].num, &value_int,
                                        -1);
                    fprintf (fp, "%d,", value_int);
                  }
                  break;
                case G_TYPE_DOUBLE:
                  {
                    gdouble value_double;
                    gtk_tree_model_get (model_son, &iter_son,
                                        print->son->cols[j].num, &value_double,
                                        -1);
                    fprintf (fp, "\"%s\",", PUT (g_strdup_printf ("%.2f", value_double)));
                  }
                  break;
                }
            }

	  fprintf (fp, "\n\n");
	  gtk_tree_selection_select_iter (gtk_tree_view_get_selection (print->son->tree), &iter_son);
	  gtk_tree_view_row_activated (print->son->tree, gtk_tree_model_get_path (model_son, &iter_son), 
				       gtk_tree_view_get_column (print->son->tree, 0));

	  if (gtk_tree_model_get_iter_first (model_grandson, &iter_grandson) == FALSE)
	    {
	      printf ("No se seleccionó una fila del treeview nieto, puede ser que no haya información\n");
	      return;
	    }

	  //Se agrega la cabecera de las columnas del treeview hijo
	  //fprintf (fp, ",,");
	  for (k = 0; k < grandson_cols; k++)
	    fprintf (fp, "\"%s\",", print->son->son->cols[k].name);
	  fprintf (fp, "\n");

	  //Se insertan los datos del treeview hijo
	  do
	    {
	      //fprintf (fp, ",,");

	      for (k = 0; k < grandson_cols; k++)
		{
		  column_type = gtk_tree_model_get_column_type
		    (model_grandson, print->son->son->cols[k].num);

		  switch (column_type)
		    {
		    case G_TYPE_STRING:
		      {
			gchar *value_char = NULL;

			gtk_tree_model_get (model_grandson, &iter_grandson,
					    print->son->son->cols[k].num, &value_char,
					    -1);

			fprintf (fp, "\"%s\",", value_char);
		      }
		      break;
		    case G_TYPE_INT:
		      {
			gint value_int;
			gtk_tree_model_get (model_grandson, &iter_grandson,
					    print->son->son->cols[k].num, &value_int,
					    -1);
			fprintf (fp, "%d,", value_int);
		      }
		      break;
		    case G_TYPE_DOUBLE:
		      {
			gdouble value_double;
			gtk_tree_model_get (model_grandson, &iter_grandson,
					    print->son->son->cols[k].num, &value_double,
					    -1);
			fprintf (fp, "\"%s\",", PUT (g_strdup_printf ("%.2f", value_double)));
		      }
		      break;
		    }
		}
	      fprintf (fp, "\n");
	    } while ((gtk_tree_model_iter_next (model_grandson, &iter_grandson)) != FALSE);

          fprintf (fp, "\n");
        } while ((gtk_tree_model_iter_next (model_son, &iter_son)) != FALSE);

      fprintf (fp, "\n\n");
    } while ((gtk_tree_model_iter_next (model_father, &iter_father)) != FALSE);

  fclose (fp);

  LaunchApp (file);
}



/*
  This function will print the selcted rows from the two tree view, 
  one will contain the father and the other will contain the son
*/
void
SelectivePrintTwoTree (GtkWidget *widget, gpointer data)
{
  Print *print = (Print *) data;
  GtkTreeSelection *selection_father = gtk_tree_view_get_selection (print->tree);
  GtkTreeModel *model_father = gtk_tree_view_get_model (print->tree);
  GtkTreeModel *model_son = gtk_tree_view_get_model (print->son->tree);
  GtkTreeIter iter_father, iter_son;
  GType column_type;
  gint i, j;
  gint boolean_column_num;
  gboolean boolean_column = FALSE;
  gint father_cols = 0, son_cols = 0;
  gchar *temp_directory = rizoma_get_value ("TEMP_FILES");
  gchar *file;
  FILE *fp;

  if (gtk_tree_selection_get_selected (selection_father, NULL, &iter_father) == FALSE)
    return;

  while (print->cols[father_cols].name != NULL)
    father_cols++;

  while (print->son->cols[son_cols].name != NULL)
    son_cols++;

  /* Si es NULL */
  if (print->date_string == NULL)
    print->date_string = CurrentDate(0); /* Asumismos la fecha actual */

  file = g_strdup_printf ("%s/informe-%s-%s.csv", temp_directory,
                          print->name, print->date_string);

  fp = fopen (file, "w");

  if (fp == NULL)
    {
      perror (g_strdup_printf ("Opening %s", file));
      return;
    }
  else
    printf ("Working on %s\n", file);

  fprintf (fp, "%s\n%s\n\n", print->title, print->date_string);

  //Se agrega la cabecera de las columnas del treeview padre
  for (i = 0; i < father_cols; i++)
    fprintf (fp, "\"%s\",", print->cols[i].name);
  fprintf (fp, ",,,,\n");

  for (i = 0; i < father_cols; i++)
    {
      column_type = gtk_tree_model_get_column_type (model_father, print->cols[i].num);

      switch (column_type)
	{
	case G_TYPE_STRING:
	  {
	    gchar *value_char;
	    gtk_tree_model_get (model_father, &iter_father,
				print->cols[i].num, &value_char,
				-1);
	    fprintf (fp, "%s,", CutPoints (value_char));
	  }
	  break;

	case G_TYPE_INT:
	  {
	    gint value_int;
	    gtk_tree_model_get (model_father, &iter_father,
				print->cols[i].num, &value_int,
				-1);
	    fprintf (fp, "%d,", value_int);
	  }
	  break;

	case G_TYPE_DOUBLE:
	  {
	    gdouble value_double;
	    gtk_tree_model_get (model_father, &iter_father,
				print->cols[i].num, &value_double,
				-1);
	    fprintf (fp, "\"%s\",", PUT (g_strdup_printf ("%.2f", value_double)));
	  }
	  break;
	}
    }

  fprintf (fp, "\n\n");

  gtk_tree_selection_select_iter (gtk_tree_view_get_selection (print->tree), &iter_father);
  //gtk_tree_view_row_activated (print->tree, gtk_tree_model_get_path (model_father, &iter_father), 
  //                                                    gtk_tree_view_get_column (print->tree, 0));

  gtk_tree_model_get_iter_first (model_son, &iter_son);

  //Se agrega la cabecera de las columnas del treeview hijo
  fprintf (fp, ",");
      
  for (i = 0; i < son_cols; i++)
    {
      column_type = gtk_tree_model_get_column_type (model_son, print->son->cols[i].num);
      if (column_type == G_TYPE_BOOLEAN)
	{
	  boolean_column_num = i;
	  boolean_column = TRUE;
	}
      else
	fprintf (fp, "\"%s\",", print->son->cols[i].name);
    }
      
  fprintf (fp, ",,,,\n");
  
  if (boolean_column == FALSE)
    return;
              
  gboolean enabled;
  gint largo = get_treeview_length (print->son->tree);

  if (largo == 0)
    return;
  
  //Se insertan los datos del treeview hijo
  for (i=0; i<largo; i++)
    {
      gtk_tree_model_get (model_son, &iter_son,
			  boolean_column_num, &enabled,
			  -1);

      if (enabled == TRUE)
	{	  
	  fprintf (fp, ",");

	  for (j = 0; j < son_cols; j++)
	    {
	      column_type = gtk_tree_model_get_column_type (model_son, print->son->cols[j].num);

	      switch (column_type)
		{
		case G_TYPE_STRING:
		  {
		    gchar *value_char = NULL;
		    gtk_tree_model_get (model_son, &iter_son,
					print->son->cols[j].num, &value_char,
					-1);
		    fprintf (fp, "\"%s\",", value_char);
		  }
		  break;
		  
		case G_TYPE_INT:
		  {
		    gint value_int;
		    gtk_tree_model_get (model_son, &iter_son,
					print->son->cols[j].num, &value_int,
					-1);
		    fprintf (fp, "%d,", value_int);
		  }
		  break;

		case G_TYPE_DOUBLE:
		  {
		    gdouble value_double;
		    gtk_tree_model_get (model_son, &iter_son,
					print->son->cols[j].num, &value_double,
					-1);
		    fprintf (fp, "\"%s\",", PUT (g_strdup_printf ("%.2f", value_double)));
		  }
		  break;

		} // END SWITCH
	    } // END FOR

	  fprintf (fp, "\n");

	} // END IF
      gtk_tree_model_iter_next (model_son, &iter_son);
    } // END FOR

  fprintf (fp, "\n\n");

  fclose (fp);

  LaunchApp (file);
}


void 
print_cash_box_selected ( void )
{
  PGresult *res;
  gchar *query;
  gchar *total_ingreso;
  gchar *total_egreso;
  gchar *monto_caja;
  gint cash_id;

  gchar *temp_directory = rizoma_get_value ("TEMP_FILES");
  gchar *file;
  FILE *fp;

  GtkTreeView *tree = GTK_TREE_VIEW (builder_get (builder, "tree_view_cash_box_lists"));
  GtkTreeModel *model = gtk_tree_view_get_model (tree);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter) == FALSE)
    return;
  
  //Se obtiene el id seleccionado
  gtk_tree_model_get (model, &iter,
		      0, &cash_id,
		      -1);

  //Se obtiene informacion    
  query = g_strdup_printf ("select to_char (open_date, 'DD-MM-YYYY HH24:MI') as open_date_formatted, "
			   "to_char (close_date, 'DD-MM-YYYY HH24:MI') as close_date_formatted, * from cash_box_report (%d)", cash_id);
  res = EjecutarSQL (query);
  g_free (query);

  //Obtiene el ingreso total
  total_ingreso = PutPoints (g_strdup_printf ("%d", atoi (PQvaluebycol (res, 0, "cash_sells"))
					      + atoi (PQvaluebycol (res, 0, "cash_income"))
					      + atoi (PQvaluebycol (res, 0, "cash_payed_money"))
					      + atoi (PQvaluebycol (res, 0, "cash_box_start"))
					      + atoi (PQvaluebycol (res, 0, "bottle_deposit"))
					      ));
  //Obtiene el egreso total
  total_egreso = PutPoints (g_strdup_printf ("%d", atoi (PQvaluebycol (res, 0, "cash_outcome"))
					     + atoi (PQvaluebycol (res, 0, "cash_loss_money"))
					     + atoi (PQvaluebycol (res, 0, "nullify_sell"))
					     + atoi (PQvaluebycol (res, 0, "current_expenses"))
					     + atoi (PQvaluebycol (res, 0, "bottle_return"))
					     ));

  //Obtiene el monto en caja
  monto_caja = PutPoints (g_strdup_printf ("%d", atoi (PQvaluebycol (res, 0, "cash_box_start"))
					   + atoi (PQvaluebycol (res, 0, "cash_sells"))
					   + atoi (PQvaluebycol (res, 0, "cash_payed_money"))
					   + atoi (PQvaluebycol (res, 0, "cash_income"))
					   + atoi (PQvaluebycol (res, 0, "bottle_deposit"))
					   - atoi (PQvaluebycol (res, 0, "cash_loss_money"))
					   - atoi (PQvaluebycol (res, 0, "cash_outcome"))
					   - atoi (PQvaluebycol (res, 0, "nullify_sell"))
					   - atoi (PQvaluebycol (res, 0, "current_expenses"))
					   - atoi (PQvaluebycol (res, 0, "bottle_return"))
					   ));

  //Se crea el archivo
  file = g_strdup_printf ("%s/informe-caja-%s.csv", temp_directory,
                          CurrentDate(0));

  fp = fopen (file, "w");

  if (fp == NULL)
    {
      perror (g_strdup_printf ("Opening %s", file));
      return;
    }
  else
    printf ("Working on %s\n", file);

  //Cabecera archivo
  fprintf (fp, "Informe de Caja\n%s\n\n", CurrentDate(0));
  fprintf (fp, ",,,,\n");

  //Cuerpo archivo
  //fprintf (fp, "  Nombre usuario: %s \n", user_name);
  fprintf (fp, "  ID caja: %d \n", cash_id);
  fprintf (fp, "  ------------------ \n");

  fprintf (fp, "  Fecha apertura: %s \n", PQvaluebycol (res, 0, "open_date_formatted"));
  fprintf (fp, "  Fecha cierre: %s \n\n", PQvaluebycol (res, 0, "close_date_formatted"));

  fprintf (fp, "  INGRESOS: \n");
  fprintf (fp, "  \"Inicio\", \"Ventas Efectivo\", \"Abonos Credito\", \"Deposito Envase\", \"Ingresos Efectivo\", \"Sub-total ingresos\" \n");
  fprintf (fp, "  %s,", PutPoints (PQvaluebycol (res, 0, "cash_box_start")));
  fprintf (fp, "  %s,", PutPoints (PQvaluebycol (res, 0, "cash_sells")));
  fprintf (fp, "  %s,", PutPoints (PQvaluebycol (res, 0, "cash_payed_money")));
  fprintf (fp, "  %s,", PutPoints (PQvaluebycol (res, 0, "bottle_deposit")));
  fprintf (fp, "  %s,", PutPoints (PQvaluebycol (res, 0, "cash_income")));
  fprintf (fp, "  %s \n\n", total_ingreso);

  fprintf (fp, "  EGRESOS: \n");
  fprintf (fp, "  \"Retiros\", \"Ventas Anuladas\", \"Gastos Corrientes\", \"Devolucion Envase\", \"Perdida\", \"Sub-total egresos\" \n");
  fprintf (fp, "  %s,", PutPoints (PQvaluebycol (res, 0, "cash_outcome")));
  fprintf (fp, "  %s,", PutPoints (PQvaluebycol (res, 0, "nullify_sell")));
  fprintf (fp, "  %s,", PutPoints (PQvaluebycol (res, 0, "current_expenses")));
  fprintf (fp, "  %s,", PutPoints (PQvaluebycol (res, 0, "bottle_return")));
  fprintf (fp, "  %s,", PutPoints (PQvaluebycol (res, 0, "cash_loss_money")));
  fprintf (fp, "  %s \n\n", total_egreso);

  fprintf (fp, "  ,,,,,Saldo en caja:\n ,,,,, %s \n", monto_caja);

  //Finaliza el archivo
  fclose (fp);
  LaunchApp (file);
}
