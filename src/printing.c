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
#include"tiempo.h"
#include"main.h"

int
LaunchApp (gchar *file)
{
  pid_t pid;

  pid = fork ();
  if (pid == 0)
    {
      system (g_strdup_printf ("LANG=C gnumeric \"%s\"", file));
      exit (0);
    }
  
  return 0;
}

void
PrintTree (GtkWidget *widget, gpointer data)
{
  Print *print = (Print *) data;
  GtkTreeModel *model = gtk_tree_view_get_model (print->tree);
  GtkTreeIter iter;
  GtkTreeIter son;
  GType column_type;
  gint columns = gtk_tree_model_get_n_columns (model);
  gint i;
  gint cols = 0;
  gchar *temp_directory = rizoma_get_value ("TEMP_FILES");
  gchar *file;
  FILE *fp;
  
  if (gtk_tree_model_get_iter_first (model, &iter) == FALSE)
    return;

  while (print->cols[cols].name != NULL)
    cols++;

  /* Si es NULL */
  if (print->date_string == NULL)
    print->date_string = CurrentDate (); /* Asumismo la fecha actual */
  
  file = g_strdup_printf ("%s/informe-%s.csv", temp_directory, print->date_string);

  fp = fopen (file, "w");
  
  if (fp == NULL)
    {
      perror (g_strdup_printf ("Opening %s", file));
      return;
    }
  else
    printf ("Working on %s\n", file);

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
		  }	  column_type = gtk_tree_model_get_column_type (model, i);
		
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
    print->date_string = CurrentDate (); /* Asumismo la fecha actual */
  
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
  
  for (i = 0; i < father_cols; i++)
    {
      fprintf (fp, "\"%s\",", print->cols[i].name);
    }
  
  fprintf (fp, ",,,,\n");
  
  do
    {
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
      
      fprintf (fp, "\n");
      
      gtk_tree_selection_select_iter (gtk_tree_view_get_selection (print->tree), &iter_father);

      gtk_tree_model_get_iter_first (model_son, &iter_son);

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
      		 
    } while ((gtk_tree_model_iter_next (model_father, &iter_father)) != FALSE);

  fclose (fp);
  
  LaunchApp (file);
}
