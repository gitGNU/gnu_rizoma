/*encriptar.c
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
#include<string.h>
 
#include<rpc/des_crypt.h>

#include"tipos.h"

#include<rizoma_errors.h>
#include"postgres-functions.h"

/*gchar *
EncriptarPasswd (gchar *passwd)
{  
  des_setparity (key);

  ecb_crypt (key, passwd, strlen (passwd), DES_ENCRYPT);

  return passwd;
}
*/
gboolean
CompararPassword (gchar *passwd_db, gchar *passwd)
{
  /*  des_setparity (llave);

  ecb_crypt (llave, passwd, strlen (passwd), DES_ENCRYPT);
  */
  if (strcmp (passwd_db, passwd) == 0)
    return TRUE;
  else
    return FALSE;
}

gboolean 
AcceptPassword (gchar *passwd, gchar *user)
{
  /*gchar *passwd_db = ReturnPasswd (user);
  gchar *llave = ReturnLlave (user);

  if (passwd_db == NULL)
    return FALSE;
  
  if (CompararPassword (passwd_db, passwd) == FALSE)
    return FALSE;
  else
  return TRUE;*/

  PGresult *res;
  
  /*  res = EjecutarSQL (g_strdup_printf 
		     ("SELECT * FROM users WHERE passwd=md5('%s') AND usuario='%s' AND (SELECT date_part ('year', asistencia.salida) "
		      "FROM asistencia WHERE id_user=users.id ORDER BY entrada DESC LIMIT 1)!=0; ", passwd, user));
  */
  res = EjecutarSQL (g_strdup_printf ("SELECT * FROM users WHERE passwd=md5('%s') AND usuario='%s'", passwd, user));

  if (res != NULL && PQntuples (res) == 0)
    rizoma_errors_set ("El usuario o la contraseña no son correctos", "AcceptPassword ()", ALERT);
    
  if (res != NULL && PQntuples (res) != 0)
    return TRUE;
  else
    return FALSE;

}
