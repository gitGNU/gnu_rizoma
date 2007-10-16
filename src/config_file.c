/*config_file.c
 *
 *    Copyright 2004 Rizoma Tecnologia Limitada <jonathan@rizoma.cl>
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
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
 * We read the config file
 */
int
read_conf (char *file, Parms parms[], int total)
{
  /* The file descriptor to use */
  FILE *fd;
  /* We save the line returned bye get_clean_line ()*/
  char *line;
  /* A silly buffer */
  char *buf;
  /* The counter to run through the vars to get */
  int i;
  /* The lenght of the silly buffer ;)*/
  int len;

  /* We open the config file */
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
				/*
				 * We get the len of the buffer + 1, because
				 * we don't want to lose a character ;)
				 */
				len = strlen (buf) + 1;

				*parms[i].value = (gchar *) g_malloc (sizeof (len));

				g_snprintf (*parms[i].value, len, "%s", buf);

				break;
			  }

		  g_free (buf);

		  g_free (line);
		}
    } while (line != NULL);

  /* We close the config file */
  fclose (fd);

  /* We return success */
  return 0;
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


/* New */

RizomaConf *
rizoma_conf_new (void)
{
  RizomaConf *new;

  new = (RizomaConf *)malloc (sizeof (RizomaConf));

  new->back = NULL;

  new->var_name = NULL;
  new->valor = NULL;

  new->next = NULL;

  return new;
}

RizomaConf *
rizoma_read_conf (char *file)
{
  FILE *fd;
  char *line;
  RizomaConf *header = NULL;
  RizomaConf *current;
  RizomaConf *new;

  fd = fopen (file, "r");

  if (fd == NULL)
	return NULL;

  do
    {
	  line = get_clean_line (fd);

	  if (line != NULL && *line != '\0' && *line != ' ')
		{

		  if (header == NULL)
			{
			  header = rizoma_conf_new ();

			  header->var_name = get_var_name (line);
			  header->valor = get_value_var (line);
			  header->next = header;
			}
		  else
			{
			  current = header;
			  while (current->next != header)
				current = current->next;

			  new = rizoma_conf_new ();

			  new->var_name = get_var_name (line);
			  new->valor = get_value_var (line);

			  current->next = new;
			  new->back = current;
			  new->next = header;
			}
		}
    } while (line != NULL);

  return header;
}

char *
rizoma_get_value (RizomaConf *header, char *var_name)
{
  RizomaConf *current = header;

  do
    {
	  if (strcasecmp (current->var_name, var_name) == 0)
		return current->valor;

	  current = current->next;
    } while (current != header);

  return NULL;
}

int
rizoma_set_value (RizomaConf *header, char *var_name, char *new_value)
{
  RizomaConf *current = header;

  do
    {
	  if (strcasecmp (current->var_name, var_name) == 0)
		{
		  g_free (current->valor);
		  current->valor = g_strdup (new_value);
		  return 0;
		}

	  current = current->next;
    } while (current != header);

  return -1;
}

int
rizoma_save_file (RizomaConf *header)
{
  FILE *fp;
  RizomaConf *current = header;

  fp = fopen (g_strdup_printf ("%s/.rizoma", getenv ("HOME")), "w+");

  fprintf (fp, "# Rizoma Version %s\n", VERSION);

  do
    {
	  fprintf (fp, "%s = %s\n", current->var_name, current->valor);

	  current = current->next;
    } while (current != header);

  fclose (fp);

  return 0;
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
