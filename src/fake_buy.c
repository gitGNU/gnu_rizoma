#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<tipos.h>
#include<config_file.h>
#include<postgres-functions.h>

gchar *
PutPoints (gchar *number)
{
  gchar *alt = g_malloc0 (15), *alt3 = g_malloc0 (15);
  int len = strlen (number);
  gint points;
  gint i, unidad = 0, point = 0;

  if (len <= 3)
	return number;

  if ((len % 3) != 0)
	points = len / 3;
  else
	points = (len / 3) - 1;

  for (i = len; i >= 0; i--)
    {
	  if (unidad == 3 && point < points && number[i] != '.')
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
SendCursorTo (GtkWidget *widget, gpointer data)
{
  GtkWindow *window = GTK_WINDOW (gtk_widget_get_toplevel ((GtkWidget *)data));
  GtkWidget *destiny = (GtkWidget *) data;

  gtk_window_set_focus (window, destiny);
}


char *
get_line (FILE *fd)
{
  int c;
  int i = 0;
  char *line = (char *) malloc (255);

  memset (line, '\0', 255);

  for (c = fgetc (fd); c != '#' && c != '\n' && c != EOF; c = fgetc (fd))
    {
	  *line++ = c;
	  i++;
    }


  for (; c != '\n' && c != EOF; c = fgetc (fd));


  if (i > 0 || c != EOF)
    {
	  for (; i != 0; i--)
		*line--;

	  return line;
    }
  else
	return NULL;
}

void
IngresarFakeCompra (void)
{
  Productos *products = compra->header_compra;
  gint id, doc;
  gchar *rut_proveedor;

  PGresult *res;

  res = EjecutarSQL ("SELECT id FROM compra ORDER BY id DESC LIMIT 1");

  id = atoi (PQgetvalue (res, 0, 0));

  if (products != NULL)
    {
	  do {

		IngresarProducto (products->product, id);

		IngresarDetalleDocumento (products->product, id,
								  -1,
								  -1);

		products = products->next;
	  }
	  while (products != compra->header_compra);

	  //      SetProductosIngresados ();

    }

  rut_proveedor = GetDataByOne (g_strdup_printf ("SELECT rut_proveedor FROM compra WHERE id=%d",
												 id));


  doc = IngresarFactura ("-1", id, rut_proveedor, -1, "25", "3", "06", 0);


  CompraIngresada ();

}

int main (int argc, char **argv)
{
  FILE *fp;
  char *line = NULL;
  char *value;

  char *barcode;
  double cant = 100.0;
  double pcomp = 0.0;
  int precio = 1000;
  int margen = 20;
  char * pEnd;
  int config;
  char *config_file;

  fp = fopen (argv[1], "r");

  compra = (Compra *) g_malloc (sizeof (Compra));
  compra->header = NULL;
  compra->products_list = NULL;
  compra->header_compra = NULL;
  compra->products_compra = NULL;
  compra->current = NULL;

  config_file = g_strdup_printf ("%s/.rizoma", getenv ("HOME"));

  rizoma_config = rizoma_read_conf (config_file);

  if (rizoma_config == NULL)

  config_file = g_strconcat(g_getenv("HOME"),"/.rizoma");
  if (!g_file_test(config_file, 
		   G_FILE_TEST_EXISTS|G_FILE_TEST_IS_REGULAR))
    {
	  perror (g_strdup_printf ("Opening %s", config_file));
	  printf ("Para configurar su sistema debe ejecutar rizoma-config usando gksu(o similar) con la opcion -k");
	  exit (-1);
    }

  do {

	line = get_line (fp);

	if (line != NULL)
	  {
		/*
		  Codigo de Barras, Cantidad, Precio, Precio Compra
		*/
		barcode = strtok (line, ",");
		cant = strtod( strtok( NULL, "," ), &pEnd );
		precio = atoi( strtok( NULL, "," ) );
		pcomp = strtod( strtok (NULL, ","), &pEnd );
		//printf( "Barcode: %s\nCant: %f\nPrecio: %d\nPrecio Compra: %f\n", barcode, cant, precio, pcomp );

		if( cant == 0 ) continue;

		CompraAgregarALista (barcode, cant, precio, pcomp, margen, FALSE);
	  }

  } while (line != NULL);

  AgregarCompra ("161736979", "", 0);

  // IngresarFakeCompra ();

  fclose (fp);

  return 0;
}
