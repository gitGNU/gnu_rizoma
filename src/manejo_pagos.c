/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4;
       c-indentation-style: gnu -*- */
/*manejo_pagos.c
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
#include<string.h>
#include<stdlib.h>
#include<math.h>

#include"tipos.h"
#include"postgres-functions.h"
#include"utils.h"

/**
 * Crea un nuevo cheque de restaurant
 */
ChequesRestaurant *
create_new_restaurant_check (gchar *codigo, gchar *fecha_venc, gint monto)
{
  ChequesRestaurant *new = NULL;

  new = (ChequesRestaurant *) g_malloc (sizeof (ChequesRestaurant));
  new->cheque = (ChequeRest *) g_malloc (sizeof (ChequeRest));
  new->cheque->codigo = codigo;
  new->cheque->fecha_vencimiento = fecha_venc;
  new->cheque->monto = monto;

  return new;
}

/**
 *
 * Libera la memoria de un cheque eliminado desde la lista de cheques.
 *
 */
void
free_chk_rest (ChequesRestaurant *cheques_restaurant)
{
    g_free (cheques_restaurant->cheque);
    g_free (cheques_restaurant);
}

/**
 * Esta funcion agrega cheques a una cadena "circular"
 * (después del último cheque viene el primero).
 * 
 * LLena la estructura ChequesRestaurant * (un trio), que contiene 
 * el trio anterior, un ChequeRest * (el cheque en si) 
 * y el trio siguiente.
 */
gint
add_chk_rest_to_list (gchar *codigo, gchar *fecha_venc, gint monto)
{
  //Se inicializa un trio
  ChequesRestaurant *new = NULL;

  //Si no existen cheques anteriores, se inicializa con uno
  if (pago_chk_rest->header == NULL)
    {
      new = create_new_restaurant_check (codigo, fecha_venc, monto);
      pago_chk_rest->header = new;

      pago_chk_rest->cheques = new;

      new->back = NULL;
      new->next = pago_chk_rest->header;

      new->cheque->lugar = 1;
    }
  else //De lo contrario se agrega un cheque a la lista
    {
      //Se obtiene el primer trio de la lista
      ChequesRestaurant *end = pago_chk_rest->header;

      //Se crea un nuevo trio
      new = create_new_restaurant_check (codigo, fecha_venc, monto);

      //Se obtiene el último trio de la lista (a su vez es el anterior al primero)
      while (end->next != pago_chk_rest->header)
        end = end->next;

      //El nuevo tiene como antecesor al último
      new->back = end;

      //El que era último tiene de sucesor al nuevo (antes tenía al header de sucesor)
      end->next = new;

      //El nuevo (que ahora es el último) tiene al header de sucesor
      new->next = pago_chk_rest->header;

      //el nuevo ahora es el último trio en pagos
      pago_chk_rest->cheques = new;

      //El lugar del cheque nuevo es igual al lugar del anterior + 1
      new->cheque->lugar = new->back->cheque->lugar+1;
    }

  return 0;
}


/**
 *
 *
 *
 */
gint
del_chk_rest_from_list (gchar *codigo)
{
  ChequesRestaurant *find = pago_chk_rest->header;
  ChequesRestaurant *end = pago_chk_rest->header;

  //Busca el trio cuyo código coincida con el ingresado
  while (!g_str_equal (find->cheque->codigo, codigo))
    find = find->next;

  printf ("%s == %s\n", find->cheque->codigo, codigo);

  //Obtiene el último trio
  while (end->next != pago_chk_rest->header)
    end = end->next;

  //Remueve el trío de la cadena según corresponda

  //Si el código corresponde al header, y solo queda el header
  if (find == pago_chk_rest->header && pago_chk_rest->header->next == pago_chk_rest->header)
    {
      free_chk_rest (find);

      pago_chk_rest->header = NULL;
      pago_chk_rest->cheques = NULL;
    }
  else if (find->back == NULL) //Si es el primero y no es el único
    {
      //El siguiente será el nuevo header
      pago_chk_rest->header = find->next;
      pago_chk_rest->header->back = NULL;

      //El que le continúe al último será el nuevo header
      end->next = pago_chk_rest->header;

      free_chk_rest (find);
    }
  else if (find == end) //Si se eligió el último y hay más de uno
    {
      //La continuación de mi antecesor será el header
      find->back->next = pago_chk_rest->header;

      free_chk_rest (find);
    }
  else //Si se eligió uno de al medio y hay más de uno
    {
      //Mi sucesor actual, será el sucesor de mi antecesor
      find->back->next = find->next;
      //Mi antecesor actual, será el antecesor de mi sucesor
      find->next->back = find->back;

      free_chk_rest (find);
    }

  //El cheque será igual al header (por qué?...)
  pago_chk_rest->cheques = pago_chk_rest->header;

  return 0;
}

gdouble
calcular_total_cheques (ChequesRestaurant *header)
{
  ChequesRestaurant *cal = header;
  gdouble total = 0;

  if (cal == NULL)
    return total;

  do
    {
      total += (gdouble)(cal->cheque->monto);
      cal = cal->next;
    }
  while (cal != header);

  return total;
}

gint
cantidad_cheques (ChequesRestaurant *header)
{
  ChequesRestaurant *cal = header;
  gint total = 0;

  do
    {
      total += 1;

      cal = cal->next;
    }
  while (cal != header);

  return total;
}

gint
limpiar_lista (void)
{
  ChequesRestaurant *alter = pago_chk_rest->header;
  ChequesRestaurant *tofree;
  gint total;
  gint i;

  if (pago_chk_rest->header == NULL) return 0;

  total = cantidad_cheques (pago_chk_rest->header);

  for (i = 0; i < total; i++)
    {
      tofree = alter;
      alter = tofree->next;
      free_chk_rest (tofree);
    }

  pago_chk_rest->header = NULL;
  pago_chk_rest->cheques = NULL;

  return 0;
}

Productos *
buscar_por_codigo (ChequesRestaurant *header, gchar *code)
{
  ChequesRestaurant *find = header;

  if (header == NULL)
    return NULL;

  do
    {
      if (strcmp (find->cheque->codigo, code) == 0)
        return find;
      else
        find = find->next;
    }
  while (find != header);

  return NULL;
}

