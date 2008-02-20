 /*config_file.c
 *
 *    Copyright (C) 2004,2008 Rizoma Tecnologia Limitada <info@rizoma.cl>
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

#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include<config.h>

#include"tipos.h"
#include"config_file.h"

#define MAXLINE 256 /* The max size in one line */

/* We */
#define COEF_PULGADA 0.3528


/*
  We get a clean line, this means that the line doesn't start with #
  or isn't a clean line only with white space
*/

char *
get_clean_line (FILE *fd)
{
  char c;
  int i = 0;
  char *line = (char *) g_malloc (MAXLINE);
  //  char *buf = NULL;
  char *special = NULL;

  /* We clean the line, we don't want any ugly character */
  memset (line, '\0', 255);

  /* We put in the line everything after a # or a newline (\n) */
  for (c = fgetc (fd); c != '#' && c != '\n' && c != EOF; c = fgetc (fd))
    {
	  /* If the character isn't a whitespace or an EOF */
	  if (c != ' ' && c != '\t')
		{
		  /* We save it */
		  *line++ = c;
		  i++;
		}
    }

  /* We found the end of this line in the file */
  for (; c != '\n' && c != EOF; c = fgetc (fd));

  /*
   * We back in the line to the begging
   * Only if is necessary
   */
  if (i > 0 || c != EOF)
    {
	  for (; i != 0; i--)
		*line--;

	  /* We look for somethin like a '=' */
	  special = strchr (line, '=');

	  if (special == NULL)
		{
		  g_free (line);
		  line = " ";
		}

	  return line;
    }
  else
	return NULL;
}

char *
get_var_name (char *line)
{
  /* return buffer */
  char *buf = NULL;
  /* Lenght of the var name */
  int len = 0;

  /* We suppose that the get_clean_line return a line without space */

  /* We get the len of the var name, everything before the '=' */
  for (; line[len] != '='; len++);

  /* We save the var name in the return buffer */
  buf = g_strndup (line, len);

  return buf;
}

char *
get_value_var (char *line)
{
  /* Return buffer */
  char *buf = NULL;
  /* Lenght of the value var */
  int len = 0;
  /* Characters before the '=' */
  //  int before = 0;

  /* Again we suppose that get_clean_line return a line without spaces */

  /* We put the pointer in the '=' */
  for (; *line != '='; *line++);

  /* We jump the '=' */
  *line++;

  /* We get the len of the value */
  for (; line[len] != '\0'; len++);

  buf = g_strndup (line, len);

  return buf;
}

/*
  We extract the x and y values from buf, the format to take these values
  it's:

  x,y
*/
int
extract_xy (char *values, Position *pos)
{
  char *buf;

  buf = strtok (values, ",");

  /* We save it*/
  pos->x = (float)(atoi (buf) / COEF_PULGADA);

  //  g_free (buf);

  buf = strtok (NULL, ",");

  pos->y = (float)(atoi (buf) / COEF_PULGADA);

  //  g_free (buf);

  return 0;
}

/*
  We read the dimensions from the 'dimension_file', this contain the
  follow format:

  field = x,y

  Always the x value should come before the y value
*/
int
read_dimensions (char *file, ParmsDimensions parms[], int total)
{
  FILE *fd;

  char *line;
  char *buf;
  int i;

  fd = fopen (file, "r");

  if (fd == NULL)
	return -1;

  do
    {
	  line = get_clean_line (fd);

	  if (line != NULL && *line != '\0' && *line != ' ')
		{
		  buf = get_var_name (line);

		  for (i = 0; i < total; i++)
			if (strcmp (parms[i].var_name, buf) == 0)
			  {
				g_free (buf);

				buf = get_value_var (line);

				*parms[i].values = (Position *) malloc (sizeof (Position));

				extract_xy (buf, *parms[i].values);

				break;
			  }

		  g_free (buf);
		  g_free (line);
		}
    } while (line != NULL);

  fclose (fd);

  return 0;
}

/**
 * Esta funcion se utiliza para obtener el valor de una clave que se
 * almacena el archivo de configuracion de rizoma
 * @param var_name un string con el nombre de la clave que se quiere
 * obtener el valor
 * @return un string con el valor de la clave solicitada, en caso de
 * que no se haya encontrado la clave retorna NULL
 */
char *
rizoma_get_value (char *var_name)
{
  gchar *value = NULL;
  GKeyFile *file;
  gchar *rizoma_path;
  gboolean res;

  rizoma_path = g_strconcat(g_getenv("HOME"), "/.rizoma", NULL);
  file = g_key_file_new();

  res = g_key_file_load_from_file(file, rizoma_path, G_KEY_FILE_NONE, NULL);

  if (!res)
    {
      g_printerr("\n*** funcion %s: no pudo ser cargado el "
		 "archivo de configuracion", G_STRFUNC);
      g_printerr("\npath: %s", rizoma_path);

      return NULL;
    }

  if (!g_key_file_has_key(file, config_profile, var_name, NULL))
    {
      g_printerr("\n*** funcion %s: el archivo de configuracion no tiene la clave %s\n", G_STRFUNC, var_name);
      return NULL;
    }

  value = g_key_file_get_string(file, config_profile, var_name, NULL);

  g_key_file_free(file);

  return(value);
}

/**
 * Cambia el valor de una clave existente, si la clave no existe la
 * crea automaticamente, los valores son escritos directamente al
 * archivo de configuracion
 * @param var_name es un string que contiene el nombre de la clave
 * @param new_value es un string que contiene el nuevo valor que debe
 * tener la clave
 */
int
rizoma_set_value (char *var_name, char *new_value)
{
  GKeyFile *file;
  gchar *rizoma_path;
  gboolean res;
  FILE *fp;

  rizoma_path = g_strconcat(g_getenv("HOME"), "/.rizoma", NULL);
  file = g_key_file_new();

  res = g_key_file_load_from_file(file, rizoma_path,
				  G_KEY_FILE_KEEP_COMMENTS, NULL);

  if (!res)
    {
      g_printerr("\n*** funcion %s: no pudo ser cargado el archivo de configuracion", G_STRFUNC);
      return NULL;
    }

  g_key_file_set_string(file, config_profile, var_name, new_value);
  //TODO: usar glib para manipular el archivo, si bien esto funciona
  //la idea es dejarlo consistente para que todo use glib
  fp = fopen (g_strdup_printf ("%s/.rizoma", g_getenv ("HOME")), "w");

  fprintf(fp, "%s", g_key_file_to_data(file, NULL, NULL));

  fflush(fp);
  fclose(fp);
  return (0);
}

void
rizoma_set_profile (gchar *group_name)
{
  config_profile = group_name;
}

int
rizoma_extract_xy (char *value, float *x, float *y)
{
  char *buf;

  if (value == NULL)
	return -1;
  printf ("%s\n", value);
  buf = strtok (value, ",");

  *x = (float)(atoi (buf) / COEF_PULGADA);

  buf = strtok (NULL, ",");

  *y = (float)(atoi (buf) / COEF_PULGADA);

  return 0;
}
