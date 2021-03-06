/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
   c-indentation-style: gnu -*- */
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

/**
 * Open the rizoma configuration file
 *
 * The returned value must be freed when no longer needed.
 *
 * @return the pointer that contains the rizoma configuration,
 */
GKeyFile*
rizoma_open_config()
{
  gchar *rizoma_path;
  GKeyFile *file;
  gboolean res;

  if (g_getenv("RIZOMA_CONF") != NULL)
      rizoma_path = g_strdup(g_getenv("RIZOMA_CONF"));
  else
      rizoma_path = g_strconcat(g_getenv("HOME"), "/.rizoma", NULL);

  file = g_key_file_new();

  res = g_key_file_load_from_file(file, rizoma_path, G_KEY_FILE_NONE, NULL);

  if (!res)
    {
      g_printerr("\n*** funcion %s: no pudo ser cargado el archivo de configuracion", G_STRFUNC);
      g_printerr("\npath: %s", rizoma_path);

      return NULL;
    }
  return file;
}

/**
 * Esta funcion se utiliza para obtener el valor de una clave que se
 * almacena el archivo de configuracion de rizoma
 * @param var_name un string con el nombre de la clave que se quiere
 * obtener el valor
 *
 * @return un string con el valor de la clave solicitada, en caso de
 * que no se haya encontrado la clave retorna NULL
 */
gchar *
rizoma_get_value (gchar *var_name)
{
  gchar *value = NULL;
  GKeyFile *file;

  file = rizoma_open_config();

  if (!g_key_file_has_key(file, config_profile, var_name, NULL))
    {
#ifdef DEBUG
      g_printerr("\n*** funcion %s: el archivo de configuracion no tiene la clave %s\n", G_STRFUNC, var_name);
#endif
      return NULL;
    }

  value = g_key_file_get_string(file, config_profile, var_name, NULL);

  g_key_file_free(file);

  return(value);
}

/**
 * This function returns the int value associated to a key at the
 * configuration profile used to start the application.
 *
 * @param var_name a key that has a int value associated
 *
 * @return the int value, if the function fails will return G_MININT
 */
int
rizoma_get_value_int (gchar *var_name)
{
  gint value;
  GKeyFile *file;

  file = rizoma_open_config();

  if (!g_key_file_has_key(file, config_profile, var_name, NULL))
    {
#ifdef DEBUG
      g_printerr("\n*** funcion %s: el archivo de configuracion no tiene la clave %s\n", G_STRFUNC, var_name);
#endif
      return G_MININT;
    }

  value = g_key_file_get_integer(file, config_profile, var_name, NULL);
  g_key_file_free(file);

  return(value);
}

/**
 * Read a boolean value associated to the given key
 *
 * If the value contained in the rizoma configuration file cannot be
 * interpreted as a boolean value this function will return FALSE
 *
 * @param var_name the key
 *
 * @return the boolean value associated to the key
 */
gboolean
rizoma_get_value_boolean (gchar *var_name)
{
  gboolean value;
  GKeyFile *file;
  GError *err=NULL;

  file = rizoma_open_config();

  if (!g_key_file_has_key(file, config_profile, var_name, NULL))
    {
#ifdef DEBUG
      g_printerr("\n*** funcion %s: el archivo de configuracion no tiene la clave %s\n", G_STRFUNC, var_name);
#endif
      return FALSE;
    }

  value = g_key_file_get_boolean(file, config_profile, var_name, &err);

  g_key_file_free(file);

  return(value);
}

/**
 * Write the contents of the key into the rizoma configuration file
 *
 * @param file the key file to write
 */
void
rizoma_write_config (GKeyFile *file)
{
  FILE *fp;

  //TODO: usar glib para manipular el archivo, si bien esto funciona
  //la idea es dejarlo consistente para que todo use glib
  fp = fopen (g_strdup_printf ("%s/.rizoma", g_getenv ("HOME")), "w");

  fprintf(fp, "%s", g_key_file_to_data(file, NULL, NULL));

  fflush(fp);
  fclose(fp);
}

/**
 * Cambia el valor de una clave existente, si la clave no existe la
 * crea automaticamente, los valores son escritos directamente al
 * archivo de configuracion
 * @param var_name es un string que contiene el nombre de la clave
 * @param new_value es un string que contiene el nuevo valor que debe
 * tener la clave
 *
 * @return return 0 when the value was saved with success
 */
int
rizoma_set_value (const gchar *var_name, const gchar *new_value)
{
  GKeyFile *file;

  file = rizoma_open_config();

  g_key_file_set_string(file, config_profile, var_name, new_value);

  rizoma_write_config(file);

  return (0);
}

/**
 * Set the key var_name to a list of double values
 *
 * @param var_name the name of the key
 * @param list the list of double values
 * @param length the length of the list
 */
void
rizoma_set_double_list (gchar *var_name, gdouble list[], gsize length)
{
  GKeyFile *file;

  file = rizoma_open_config();

  g_key_file_set_double_list (file, config_profile, var_name, list, length);

  rizoma_write_config (file);
}

/**
 * Returns a list of double values
 *
 * @param var_name the key name to retrieve
 * @param length the amount of double values that must contain the list
 *
 * @return the list of double values of length
 */
gdouble*
rizoma_get_double_list (gchar *var_name, gsize length)
{
  gdouble *value;
  GKeyFile *file;
  GError *err=NULL;

  file = rizoma_open_config();

  if (!g_key_file_has_key(file, config_profile, var_name, NULL))
    {
#ifdef DEBUG
      g_printerr("\n*** funcion %s: el archivo de configuracion no tiene la clave %s\n", G_STRFUNC, var_name);
#endif
      return FALSE;
    }

  value = g_key_file_get_double_list(file, config_profile, var_name, &length, &err);

  g_key_file_free(file);

  return(value);
}

/**
 * Set the name of the profile that must use the application
 *
 * @param group_name the name of the profile that will be used
 */
void
rizoma_set_profile (gchar *group_name)
{
  config_profile = group_name;
}
