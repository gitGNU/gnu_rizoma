-- funciones.sql
-- Copyright (C) 2008 Rizoma Tecnologia Limitada <info@rizoma.cl>

-- This file is part of rizoma.

-- Rizoma is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.

-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.

-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


-- se debe ser superusuario para crear el lenguaje
CREATE LANGUAGE plpgsql;

--helper function for informacion_producto
create or replace function select_vendidos(
       in codigo_barras bigint,
       in shortcode varchar)
returns double precision as $$
declare
prod_vendidos double precision;
begin

if codigo_barras != 0 then
   select vendidos into prod_vendidos from producto where barcode = codigo_barras;
else
   select vendidos into prod_vendidos from producto where codigo_corto = shortcode;
end if;
return prod_vendidos;
end; $$ language plpgsql;

-- return the avg of ventas day
--
create or replace function select_ventas_dia (
       in codbar bigint)
returns double precision as $$
declare
        oldest date;
        last_month date;
        passed_days interval;
	total double precision;
begin

last_month = now() - interval '1 month';

select date_trunc ('day', fecha ) into oldest from venta, venta_detalle where venta.id=id_venta and barcode=codbar and venta.fecha>=last_month order by fecha asc limit 1;

passed_days = date_trunc ('day', now()) - oldest;

IF passed_days < interval '1 days' THEN
   passed_days = interval '1 days';
END IF;

IF passed_days = interval '30 days' THEN
select (sum (cantidad)/30) into total from venta_detalle, venta
       where venta.fecha >= last_month
       and barcode=codbar
       and venta.id=id_venta;
ELSE
select (sum (cantidad)/(date_part ('day', passed_days))) into total from venta_detalle, venta
       where venta.fecha >= last_month
       and barcode=codbar
       and venta.id=id_venta;
END IF;

IF total = 0 THEN
   total = 1;
END IF;

return total;
end; $$ language plpgsql;


-- revisa si hay devoluciones de un producto dado
-- administracion_productos.c:123
create or replace function hay_devolucion(int8)
returns setof record as $$
declare
	prod ALIAS FOR $1;
	query varchar(255);
	list record;
begin
query := 'SELECT id FROM devolucion WHERE barcode_product='
	|| quote_literal(prod)
	|| ' AND devuelto=FALSE';

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END;
$$ LANGUAGE plpgsql;

-- revisa si se puede devolver el producto
-- administracion_productos.c:130
create or replace function puedo_devolver(IN num_prods float8,
       	  	  	   		  IN prod int8)
returns setof record as $$
declare
	list record;
	query varchar(255);

begin
query := 'SELECT cantidad<' || quote_literal(num_prods)
	|| ' as respuesta FROM devolucion '
	|| ' WHERE id=(SELECT id FROM devolucion WHERE barcode_product='
	|| quote_literal(prod)
	|| ' AND devuelto=FALSE)';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; $$ language plpgsql;

-- retorna el maximo de productos que se puede devolver
-- administracion_productos.c:136
create or replace function max_prods_a_devolver(int8)
returns setof record as $$
declare
	prod ALIAS FOR $1;
	list record;
	query varchar(255);

begin
query := 'SELECT cantidad FROM devolucion WHERE id= '
	|| '(SELECT id FROM devolucion WHERE barcode_product='
	|| quote_literal(prod) || ' AND devuelto=FALSE)';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; $$ language plpgsql;

-- retorna los tipos de merma existentes
-- administracion_productos.c:396
create or replace function select_tipo_merma()
returns setof record as $$
declare

	list record;
	query varchar(255);

begin
query := 'SELECT * FROM tipo_merma';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; $$ language plpgsql;

-- inserta un nuevo producto
create or replace function insertar_producto(
		IN prod_barcode bigint,
		IN prod_codigo varchar(10),
		IN prod_marca varchar(35),
		IN prod_descripcion varchar(50),
		IN prod_contenido varchar(10),
		IN prod_unidad varchar(10),
		IN prod_iva boolean,
		IN prod_otros integer,
		IN prod_familia smallint,
		IN prod_perecible boolean,
		IN prod_fraccion boolean)
returns integer as $$
begin
		INSERT INTO producto (barcode, codigo_corto, marca, descripcion, contenido, unidad, impuestos, otros, familia,
				perecibles, fraccion)
				VALUES (prod_barcode, prod_codigo, prod_marca, prod_descripcion,prod_contenido, prod_unidad, prod_iva,
				prod_otros, prod_familia, prod_perecible, prod_fraccion);

		IF FOUND IS TRUE THEN
				RETURN 1;
		ELSE
				RETURN 0;
		END IF;
END; $$ language plpgsql;

-- revisa si existe un producto con el mismo código
-- administracion_productos.c:1354
create or replace function existe_producto(prod_codigo_corto varchar(10))
returns boolean as $$
declare
	list record;
	query varchar(255);
begin
query := 'SELECT count(*) as suma FROM producto WHERE codigo_corto='
      || quote_literal(prod_codigo_corto) ;

FOR list IN EXECUTE query LOOP
	if list.suma > 0 then
		return TRUE;
	end if;
END LOOP;

RETURN FALSE;

END; $$ language plpgsql;

-- revisa si existe un producto con el mismo nombre
-- administracion_productos.c:1356
create or replace function existe_producto(prod_barcode int8)
returns boolean as $$
declare

	list record;
	query varchar(255);

begin
query := 'SELECT count(*) as suma FROM producto WHERE barcode='
      || quote_literal(prod_barcode) ;

FOR list IN EXECUTE query LOOP
	if list.suma > 0 then
		return TRUE;
	end if;
END LOOP;

RETURN FALSE;

END; $$ language plpgsql;

-- retorna TODOS los productos
-- administracion_productos.c:1376
-- NO RETORNA merma_unid
create or replace function select_producto( OUT barcode int8,
					    OUT codigo_corto varchar(10),
					    OUT marca varchar(35),
					    OUT descripcion varchar(50),
					    OUT contenido varchar(10),
					    OUT unidad varchar(10),
					    OUT stock float8,
					    OUT precio int4,
					    OUT costo_promedio int4,
					    OUT vendidos float8,
					    OUT impuestos bool,
					    OUT otros int4,
					    OUT familia int2,
					    OUT perecibles bool,
					    OUT stock_min float8,
					    OUT margen_promedio float8,
					    OUT fraccion bool,
					    OUT canje bool,
					    OUT stock_pro float8,
					    OUT tasa_canje float8,
					    OUT precio_mayor int4,
					    OUT cantidad_mayor int4,
					    OUT mayorista bool)
returns setof record as $$
declare
	list record;
	query text;
begin
query := $S$ SELECT codigo_corto, barcode, descripcion, marca, contenido,
      	     	    unidad, stock, precio, costo_promedio, vendidos, impuestos,
		    otros, familia, perecibles, stock_min, margen_promedio,
		    fraccion, canje, stock_pro, tasa_canje, precio_mayor,
		    cantidad_mayor, mayorista
		    FROM producto ORDER BY descripcion, marca$S$;

FOR list IN EXECUTE query LOOP
    barcode := list.barcode;
    codigo_corto := list.codigo_corto;
    marca := list.marca;
    descripcion := list.descripcion;
    contenido := list.contenido;
    unidad := list.unidad;
    stock := list.stock;
    precio := list.precio;
    costo_promedio := list.costo_promedio;
    vendidos := list.vendidos;
    impuestos := list.impuestos;
    otros := list.otros;
    familia := list.familia;
    perecibles := list.perecibles;
    stock_min := list.stock_min;
    margen_promedio := list.margen_promedio;
    fraccion := list.fraccion;
    canje := list.canje;
    stock_pro := list.stock_pro;
    tasa_canje := list.tasa_canje;
    precio_mayor := list.precio_mayor;
    cantidad_mayor := list.cantidad_mayor;
    mayorista := list.mayorista;
    RETURN NEXT;
END LOOP;

RETURN;

END; $$ language plpgsql;

-- Informacion de los productos para la ventana de Mercaderia
-- administracion_productos.c:1508
CREATE OR REPLACE FUNCTION informacion_producto( IN codigo_barras bigint,
		IN in_codigo_corto varchar(10),
		OUT codigo_corto varchar(10),
                OUT barcode bigint,
		OUT descripcion varchar(50),
		OUT marca varchar(35),
		OUT contenido varchar(10),
		OUT unidad varchar(10),
		OUT stock double precision,
		OUT precio integer,
		OUT costo_promedio integer,
		OUT stock_min double precision,
		OUT margen_promedio double precision,
		OUT contrib_agregada integer,
		OUT ventas_dia double precision,
		OUT vendidos double precision,
		OUT venta_neta double precision,
		OUT canje boolean,
		OUT stock_pro double precision,
		OUT tasa_canje double precision,
		OUT precio_mayor integer,
		OUT cantidad_mayor integer,
		OUT mayorista boolean,
		OUT unidades_merma double precision,
		OUT stock_day double precision,
                OUT total_vendido double precision)
RETURNS SETOF record AS $$
declare
	days double precision;
	datos record;
	query varchar;
	codbar int8;
BEGIN

query := $S$ SELECT *,
      	     	    (SELECT SUM ((cantidad * precio) - (iva + otros + (fifo * cantidad))) FROM venta_detalle WHERE barcode=producto.barcode) as contrib_agregada,
		    (stock::float / select_ventas_dia(producto.barcode)::float) AS stock_day,
		    (SELECT SUM ((cantidad * precio) - (iva + otros)) FROM venta_detalle WHERE barcode=producto.barcode) AS total_vendido,
		    select_merma (producto.barcode) as unidades_merma,
		    select_ventas_dia(producto.barcode) as ventas_dia
		FROM producto WHERE $S$;

-- check if must use the barcode or the short code
IF codigo_barras != 0 THEN
   query := query || $S$ barcode=$S$ || codigo_barras;
ELSE
   query := query || $S$ codigo_corto=$S$ || quote_literal(in_codigo_corto);
END IF;

FOR datos IN EXECUTE query LOOP
    codigo_corto := datos.codigo_corto;
    barcode := datos.barcode;
    descripcion := datos.descripcion;
    marca := datos.marca;
    contenido := datos.contenido;
    unidad := datos.unidad;
    stock := datos.stock;
    precio := datos.precio;
    costo_promedio := datos.costo_promedio;
    stock_min := datos.stock_min;
    stock_day := datos.stock_day;
    margen_promedio := datos.margen_promedio;
    contrib_agregada := round(datos.contrib_agregada);
    unidades_merma := datos.unidades_merma;
    mayorista := datos.mayorista;
    total_vendido := datos.total_vendido;
    precio_mayor := datos.precio_mayor;
    cantidad_mayor := datos.cantidad_mayor;
    ventas_dia := datos.ventas_dia;
    RETURN NEXT;
END LOOP;

RETURN;
END;
$$ LANGUAGE plpgsql;


create or replace function select_merma( IN barcode_in bigint,
		OUT unidades_merma double precision )
returns double precision as $$
declare
   aux int;
BEGIN

  aux := (SELECT count(*) FROM merma WHERE barcode = barcode_in);
  IF aux = 0 THEN
     unidades_merma := 0;
  ELSE
     unidades_merma := (SELECT sum(unidades) FROM merma WHERE barcode = barcode_in);
  END IF;

RETURN;
END;
$$ LANGUAGE plpgsql;


-- retorna el nombre de los proveedores a los que se les ha comprado
-- un producto dado
-- administracion_productos.c:1637
create or replace function select_proveedor_for_product(IN prod_barcode int8,
            	  	  	     			OUT nombre varchar(100))
returns setof varchar(100) as $$
declare
	list record;
	query text;
begin
query := $S$SELECT nombre FROM proveedor
      	 inner join compra on proveedor.rut = compra.rut_proveedor
	 inner join compra_detalle on compra_detalle.id_compra = compra.id
	 and barcode_product= $S$ || prod_barcode
	 || $S$ GROUP BY proveedor.nombre $S$;


FOR list IN EXECUTE query LOOP
	nombre := list.nombre;
	RETURN NEXT;
END LOOP;

RETURN;

END; $$ language plpgsql;

-- busca productos en base un patron con el formate de LIKE
-- administracion_productos.c:1738
-- compras.c:4057
create or replace function buscar_productos(IN expresion varchar(255),
       	  	  	   		    OUT barcode int8,
					    OUT codigo_corto varchar(10),
					    OUT marca varchar(35),
					    OUT descripcion varchar(50),
					    OUT contenido varchar(10),
					    OUT unidad varchar(10),
					    OUT stock float8,
					    OUT precio int4,
					    OUT costo_promedio int4,
					    OUT vendidos float8,
					    OUT impuestos bool,
					    OUT otros int4,
					    OUT familia int2,
					    OUT perecibles bool,
					    OUT stock_min float8,
					    OUT margen_promedio float8,
					    OUT fraccion bool,
					    OUT canje bool,
					    OUT stock_pro float8,
					    OUT tasa_canje float8,
					    OUT precio_mayor int4,
					    OUT cantidad_mayor int4,
					    OUT mayorista bool)
returns setof record as $$
declare
	list record;
	query text;
begin
query := $S$ SELECT barcode, codigo_corto, marca, descripcion, contenido,
      	     	    unidad, stock, precio, costo_promedio, vendidos, impuestos,
		    otros, familia, perecibles, stock_min, margen_promedio,
		    fraccion, canje, stock_pro, tasa_canje, precio_mayor,
		    cantidad_mayor, mayorista
             FROM producto WHERE lower(descripcion) LIKE lower($S$
	|| quote_literal(expresion) || $S$) OR lower(marca) LIKE lower($S$
	|| quote_literal(expresion) || $S$) order by descripcion, marca $S$;

FOR list IN EXECUTE query LOOP
    barcode := list.barcode;
    codigo_corto := list.codigo_corto;
    marca := list.marca;
    descripcion := list.descripcion;
    contenido := list.contenido;
    unidad := list.unidad;
    stock := list.stock;
    precio := list.precio;
    costo_promedio := list.costo_promedio;
    vendidos := list.vendidos;
    impuestos := list.impuestos;
    otros := list.otros;
    familia := list.familia;
    perecibles := list.perecibles;
    stock_min := list.stock_min;
    margen_promedio := list.margen_promedio;
    fraccion := list.fraccion;
    canje := list.canje;
    stock_pro := list.stock_pro;
    tasa_canje := list.tasa_canje;
    precio_mayor := list.precio_mayor;
    cantidad_mayor := list.cantidad_mayor;
    mayorista := list.mayorista;
    RETURN NEXT;
END LOOP;

RETURN;

END; $$ language plpgsql;

-- esta funcion es util para obtener los datos de un barcode dado
-- administracion_productos.c:1803
-- postgres-functions.c:941, 964, 978, 990, 1197, 1432, 1480, 1530, 1652, 1853
-- ventas.c:95, 1504, 3026, 3032,
create or replace function select_producto( IN prod_barcode int8,
       	  	  	   		    OUT barcode int8,
					    OUT codigo_corto varchar(10),
					    OUT marca varchar(35),
					    OUT descripcion varchar(50),
					    OUT contenido varchar(10),
					    OUT unidad varchar(10),
					    OUT stock float8,
					    OUT precio int4,
					    OUT costo_promedio int4,
					    OUT vendidos float8,
					    OUT impuestos bool,
					    OUT otros int4,
					    OUT familia int2,
					    OUT perecibles bool,
					    OUT stock_min float8,
					    OUT margen_promedio float8,
					    OUT fraccion bool,
					    OUT canje bool,
					    OUT stock_pro float8,
					    OUT tasa_canje float8,
					    OUT precio_mayor int4,
					    OUT cantidad_mayor int4,
					    OUT mayorista bool)
returns setof record as $$
declare
	list record;
	query text;
begin
query := $S$ SELECT codigo_corto, barcode, descripcion, marca, contenido,
      	     	    unidad, stock, precio, costo_promedio, vendidos, impuestos,
		    otros, familia, perecibles, stock_min, margen_promedio,
		    fraccion, canje, stock_pro, tasa_canje, precio_mayor,
		    cantidad_mayor, mayorista
             FROM producto WHERE barcode= $S$
	     || quote_literal(prod_barcode);


FOR list IN EXECUTE query LOOP
    barcode := list.barcode;
    codigo_corto := list.codigo_corto;
    marca := list.marca;
    descripcion := list.descripcion;
    contenido := list.contenido;
    unidad := list.unidad;
    stock := list.stock;
    precio := list.precio;
    costo_promedio := list.costo_promedio;
    vendidos := list.vendidos;
    impuestos := list.impuestos;
    otros := list.otros;
    familia := list.familia;
    perecibles := list.perecibles;
    stock_min := list.stock_min;
    margen_promedio := list.margen_promedio;
    fraccion := list.fraccion;
    canje := list.canje;
    stock_pro := list.stock_pro;
    tasa_canje := list.tasa_canje;
    precio_mayor := list.precio_mayor;
    cantidad_mayor := list.cantidad_mayor;
    mayorista := list.mayorista;
    RETURN NEXT;
END LOOP;
RETURN;

END; $$ language plpgsql;

-- retorna todos los impuestos menos el IVA
-- administracion_productos.c:2099
-- compras.c:3170, 3679
create or replace function select_otros_impuestos (OUT id int4,
       	  	  	   			   OUT descripcion varchar(250),
						   OUT monto float8)
returns setof record as $$
declare
	list record;
	query varchar(255);
begin
query := 'SELECT id, descripcion, monto FROM impuesto WHERE id != 1 ORDER BY id';

FOR list IN EXECUTE query LOOP
    id := list.id;
    descripcion := list.descripcion;
    monto := list.monto;
    RETURN NEXT;
END LOOP;

RETURN;

END; $$ language plpgsql;

-- retorna los numeros de boleta
-- boleta.c:36
create or replace function obtener_num_boleta()
returns setof record as $$
declare
	list record;
	query varchar(255);
begin
query := 'SELECT num_boleta FROM numeros_documentos';

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;
END; $$ language plpgsql;

-- retorna los numeros de factura
-- boleta.c:39
create or replace function obtener_num_factura()
returns setof record as $$
declare
	list record;
	query varchar(255);
begin
query := 'SELECT num_factura FROM numeros_documentos';

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; $$ language plpgsql;

-- retorna los numeros de guia
-- boleta.c:42
create or replace function obtener_num_guias()
returns setof record as $$
declare
	list record;
	query varchar(255);
begin
query := 'SELECT num_guias FROM numeros_documentos';

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;
END; $$ language plpgsql;

-- actualiza el numero de boleta
-- boleta.c:62
create or replace function update_num_boleta(int4)
returns void as $$
begin
UPDATE numeros_documentos SET num_boleta=$1;
RETURN;

END; $$ language plpgsql;

-- actualiza el numero de factura
-- boleta.c:65
create or replace function update_num_factura(int4)
returns void as $$
begin
UPDATE numeros_documentos SET num_factura=$1;
RETURN;
END; $$ language plpgsql;

-- actualiza el numero de guias
-- boleta.c:68
create or replace function update_num_guias(int4)
returns void as $$
begin
UPDATE numeros_documentos SET num_guias=$1;
RETURN;
END; $$ language plpgsql;

-- se retornan todas las filas que contengan el numero de factura dado
-- compras.c:725
create or replace function select_factura_compra_by_num_factura
       	  	  	   (IN factura_numero int4,
			   OUT id int4,
			   OUT id_compra int4,
			   OUT rut_proveedor varchar(20),
			   OUT num_factura int4,
			   OUT fecha timestamp,
			   OUT valor_neto int4,
			   OUT valor_iva int4,
			   OUT descuento int4,
			   OUT pagada bool,
			   OUT monto int4,
			   OUT fecha_pago timestamp,
			   OUT forma_pago int4)
returns setof record as $$
declare
	list record;
	query text;
begin
query := $S$ SELECT id, id_compra, rut_proveedor, num_factura, fecha,
      	     	    valor_neto, valor_iva, descuento, pagada, monto,
		    fecha_pago, forma_pago
		    FROM factura_compra
		    WHERE num_factura=$S$ || factura_numero;

FOR list IN EXECUTE query LOOP
    id            := list.id;
    id_compra	  := list.id_compra;
    rut_proveedor := list.rut_proveedor;
    num_factura	  := list.num_factura;
    fecha	  := list.fecha;
    valor_neto	  := list.valor_neto;
    valor_iva	  := list.valor_iva;
    descuento	  := list.descuento;
    pagada	  := list.pagada;
    monto	  := list.monto;
    fecha_pago	  := list.fecha_pago;
    forma_pago	  := list.forma_pago;
    RETURN NEXT;
END LOOP;

RETURN;

END; $$ language plpgsql;


-- retorna el (o los barcode) para un codigo corto dado
-- compras.c:2749
create or replace function codigo_corto_to_barcode
       	  	  	   (IN prod_codigo_corto varchar(10),
			   OUT barcode int8)
returns setof int8 as $$
declare
	contador int;
	list record;
	query varchar(255);
begin
contador := 0;
-- esta sentencia creo que se puede satisfacer con una anterior
query := 'select barcode from producto where codigo_corto = '
      	 	 || quote_literal(prod_codigo_corto);

FOR list IN EXECUTE query LOOP
    barcode := list.barcode;
    IF contador > 0 THEN
       RAISE NOTICE 'Retornando más de un barcode para el codigo corto: %',
       	     	    prod_codigo_corto;
    END IF;
    contador := contador + 1;
    RETURN NEXT;
END LOOP;

RETURN;

END; $$ language plpgsql;

-- retorna todos los datos de un producto
-- compras.c:2760, manejo_productos.c:311
create or replace function obtener_producto(varchar(14))
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := ''SELECT * FROM productos WHERE barcode='' || quote_literal($1);

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- retorna TRUE cuando el codigo corto está libre
-- compras.c:3725
create or replace function codigo_corto_libre(varchar(10))
returns boolean as '
declare

	list record;
	query varchar(255);

begin
query := ''SELECT count(codigo) as suma FROM productos WHERE codigo='' || quote_literal($1);

FOR list IN EXECUTE query LOOP
	if list.suma > 0 then
		return false;
	else
		return true;
	end if;
END LOOP;

RETURN false;

END; ' language plpgsql;

-- ??retorna los datos de los proveedores
-- compras.c:4600
create or replace function select_proveedores()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := ''SELECT * FROM proveedores ORDER BY nombre ASC'';

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??anula una compra
-- compras.c:5186, 5189
create or replace function anular_compra(varchar(10),int4)
returns setof record as '
declare

	list record;
	query varchar(255);

begin
UPDATE products_buy_history SET anulado=TRUE WHERE barcode_product=(SELECT barcode FROM productos WHERE codigo=quote_literal($1)) AND id_compra=$2;
UPDATE compras SET anulada=TRUE WHERE id NOT IN (SELECT id_compra FROM products_buy_history WHERE id_compra=$2 AND anulado=FALSE) AND id=$2;
RETURN;

END; ' language plpgsql;

-- ??buscar un cliente, retorna los datos de este
-- utiliza el patron de LIKE (p.e. '%ab%')
-- credito.c:48
create or replace function buscar_cliente(varchar(255))
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := ''SELECT * FROM clientes WHERE lower(nombre) LIKE lower('' || quote_literal($1)
	|| '') OR lower(apellido_paterno) LIKE lower('' || quote_literal($1)
	|| '') OR lower(apellido_materno) LIKE lower('' || quote_literal($1) || '')'';

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- retorna TODO los clientes
-- credito.c:843
create or replace function select_cliente(
       OUT rut int4,
       OUT dv varchar(1),
       OUT nombre varchar(60),
       OUT apell_p varchar(60),
       OUT apell_m varchar(60),
       OUT giro varchar(255),
       OUT direccion varchar(150),
       OUT telefono varchar(15),
       OUT telefono_movil varchar(15),
       OUT mail varchar(30),
       OUT abonado int4,
       OUT credito int4,
       OUT credito_enable boolean)
returns setof record as $$
declare
	l record;
	query varchar(255);
begin
query := 'select rut, dv, nombre, apell_p, apell_m, giro, direccion, telefono,
      	 	  telefono_movil, mail, abonado, credito, credito_enable
		  FROM cliente';

FOR l IN EXECUTE query LOOP
    rut = l.rut;
    dv = l.dv;
    nombre = l.nombre;
    apell_p = l.apell_p;
    apell_m = l.apell_m;
    giro = l.giro;
    direccion = l.direccion;
    telefono = l.telefono;
    telefono_movil = l.telefono_movil;
    mail = l.mail;
    abonado = l.abonado;
    credito = l.credito;
    credito_enable = l.credito_enable;
    RETURN NEXT;
END LOOP;

RETURN;

END; $$ language plpgsql;

-- actualiza cliente, es necesario revisar que los parametros se pasen correctamente
-- credito.c:1546
create or replace function update_cliente(int4, varchar(60), varchar(60), varchar(60),
					varchar(150), int4, int4, varchar(255))
returns setof record as '
declare

	list record;
	query varchar(255);
begin
UPDATE clientes SET nombre=quote_literal($2),
		apellido_paterno=quote_literal($3),
		apellido_materno=quote_literal($4),
		direccion=quote_literal($5),
		telefono=$6,
		credito=$7,
		giro=quote_literal($8)
		WHERE rut=quote_literal($1);
RETURN;

END; ' language plpgsql;

-- ??retorna TODOS los impuestos
-- impuestos.c:48
create or replace function select_impuesto( OUT id int4,
       	  	  	   		    OUT descripcion varchar(250),
					    OUT monto float8)
returns setof record as $$
declare
	list record;
	query varchar(255);
begin

query := 'SELECT id, descripcion, monto FROM impuesto ORDER BY id';

for list in execute query loop
    id = list.id;
    descripcion := list.descripcion;
    monto := list.monto;
    return next;
end loop;
return;
end; $$ language plpgsql;

-- inserta un nuevo impuesto
-- impuestos.c:76
create or replace function insert_impuesto (IN imp_descripcion varchar(250),
       	  	  	   		    IN imp_monto float8)
returns void as $$
declare
	list record;
	query text;
begin
	INSERT INTO impuesto (id, descripcion, monto, removable) VALUES (DEFAULT, imp_descripcion, imp_monto, DEFAULT);
return;
end; $$ language plpgsql;

-- borra un impuesto
-- impuestos.c:110
-- en la linea impuestos.c:117 se hace alusion a borrar el impuesto en otras tablas,
-- pero se puede eliminar eso usando ON CASCADE
create or replace function delete_impuesto(int4)
returns void as '
begin
DELETE FROM impuestos WHERE id=$1;
return;
end; ' language plpgsql;

-- actualiza un impuesto
-- impuestos.c:149
create or replace function update_impuesto(int4, varchar(250), float8)
returns void as '
begin
UPDATE impuestos SET descripcion=quote_liteal($2), monto=$3 WHERE id=$1;
return;
end; ' language plpgsql;

-- borra un producto
-- postgres-functions.c:181
create or replace function delete_producto(int4)
returns void as '
begin
DELETE FROM productos WHERE codigo=quote_literal($1);
return;
end; ' language plpgsql;

-- inserta un nuevo cliente
-- postgres-functions.c:372
create or replace function insert_cliente(varchar(60), varchar(60), varchar(60), int4,
					varchar(1), varchar(150), int4, int4, varchar(255))
returns void as '

begin
INSERT INTO clientes VALUES(DEFAULT, quote_literal($1),
				quote_literal($2),
				quote_literal($3),
				quote_literal($4),
				quote_literal($5),
				quote_literal($6),
				quote_literal($7),
				0,
				quote_literal($8),
				DEFAULT,
				quote_literal($9));
return;
end; ' language plpgsql;

-- retorna TRUE cuando el rut ya existe
-- postgres-functions.c:388
create or replace function existe_rut(int4)
returns boolean as '
declare
	list record;
	query varchar(255);
begin
query:= ''SELECT count(*) as suma FROM clientes WHERE rut='' || quote_literal($1);

FOR list IN EXECUTE query LOOP
	if list.suma > 0 then
		return TRUE;
	else
		return FALSE;
	end if;
END LOOP;

return TRUE;
end; ' language plpgsql;

-- retorna la deuda total de un cliente
-- postgres-functions.c:415
create or replace function deuda_total_cliente(int4)
returns setof record as '
declare
	list record;
	query text;
begin
query:= ''SELECT SUM (monto) as monto FROM ventas WHERE id IN (SELECT id_venta FROM deudas WHERE rut_cliente='' || quote_literal($1) || '' AND pagada=FALSE)'';

FOR list IN EXECUTE query LOOP
	return next list;
END LOOP;

return;
end; ' language plpgsql;

-- inserta un nuevo abono
-- postgres-functions.c:455
create or replace function insert_abono(int4,int4)
returns void as '
begin
INSERT INTO abonos VALUES (DEFAULT, $1, $2, NOW());
return;
end; ' language plpgsql;

-- cambia la password de un usuario dado
-- postgres-functions.c:637
create or replace function cambiar_password(varchar(30),varchar(400))
returns void as '
begin
update users SET passwd=md5(quote_literal($2))WHERE usuario=quote_literal($1);
return;
end; ' language plpgsql;

-- cambia el estado de credito de un cliente
-- postgres-functions.c:711
create or replace function cambiar_estado_credito(int4, boolean)
returns void as '
begin
UPDATE clientes SET credito_enable=quote_literal($2) WHERE rut=quote_literal($1);

return;
end; ' language plpgsql;

-- borra un cliente dado
-- postgres-functions.c:720
create or replace function delete_cliente(int4)
returns void as '
begin
DELETE FROM clientes WHERE rut=quote_literal($1);

return;
end; ' language plpgsql;

-- inserta una nueva compra, y devuelve el id de la compra
-- postgres-functions.c:778
create or replace function insertar_compra(
		IN proveedor integer,
		IN n_pedido varchar(100),
		IN dias_pago integer,
		OUT id_compra integer)
returns integer as $$
declare
		id_forma_pago integer;
begin
		SELECT * INTO id_forma_pago FROM get_forma_pago_dias( dias_pago );

		INSERT INTO compra(id, fecha, rut_proveedor, pedido, forma_pago, ingresada, anulada ) VALUES (DEFAULT, NOW(), proveedor, n_pedido, id_forma_pago, 'f', 'f' );

		SELECT currval(  'compra_id_seq' ) INTO id_compra;

		return;
end; $$ language plpgsql;

-- inserta el detalle de una compra
-- postgres-functions.c:808, 814
-- SELECT * FROM insertar_detalle_compra(5646, 1.00::double precision, 191.00::double precision, 300, 0::double precision, 0::smallint, 7654321, 20, 36, 23);
create or replace function insertar_detalle_compra(
		IN id_compra_in integer,
		IN cantidad double precision,
		IN precio double precision,
		IN precio_venta integer,
		IN cantidad_ingresada double precision,
		IN descuento smallint,
		IN barcode_product bigint,
		IN margen integer,
		IN iva integer,
		IN otros_impuestos integer)
returns void as $$
declare
	aux int;
	q text;
begin
-- se revisa si existe algun detalle ingresado con el id_compra dado
aux = (select count(*) from compra_detalle where id_compra=id_compra_in);

if aux = 0 then -- si no existen detalles para ese id_compra se usa el id:=0
   aux := 0;
else -- en caso contrario se saca el mayor id ingresado para el id_compra dado y se incrementa
   aux := (select max(id) from compra_detalle where id_compra=id_compra_in);
   aux := aux + 1;
end if;

q := $S$INSERT INTO compra_detalle(id, id_compra, cantidad, precio, precio_venta, cantidad_ingresada, descuento, barcode_product, margen, iva, otros_impuestos) VALUES ($S$
  || aux || $S$,$S$
  || id_compra_in || $S$,$S$
  || cantidad || $S$,$S$
  || precio || $S$,$S$
  || precio_venta || $S$,$S$
  || cantidad_ingresada || $S$,$S$
  || descuento || $S$,$S$
  || barcode_product || $S$,$S$
  || margen || $S$,$S$
  || iva || $S$,$S$
  || otros_impuestos || $S$)$S$;

EXECUTE q;


q := $S$ UPDATE producto SET precio=$S$|| precio_venta ||$S$ WHERE barcode=$S$|| barcode_product;

execute q;

return;
end; $$ language plpgsql;


-- ??paga una factura
-- postgres-functions.c:1784, 1788
create or replace function pagar_factura(int4, varchar(20), text)
returns void as '
begin
UPDATE facturas_compras SET pagada=TRUE WHERE num_factura=$1 AND rut_proveedor=quote_literal($2);

INSERT INTO pagos VALUES ((SELECT id FROM facturas_compras WHERE num_factura=quote_literal($1) AND rut_proveedor=quote_literal($2)), NOW(), FALSE, quote_literal($3));

return;
end; ' language plpgsql;

-- ??inserta una nueva merma
-- postgres-functions.c:1804
create or replace function insert_merma(varchar(14), float8, int2)
returns void as '
begin
INSERT INTO merma VALUES (DEFAULT, quote_literal($1), $2, $3);
UPDATE productos SET merma_unid=merma_unid+$2, stock=$2 WHERE barcode=quote_literal($1);

return;
end; ' language plpgsql;

-- inserta una nueva forma de pago
-- postgres-functions.c:1875
create or replace function insertar_forma_de_pago(
		IN nombre_forma varchar(50),
		IN dias integer,
		OUT id_forma integer)
returns integer as $$
BEGIN
		INSERT INTO formas_pago (id, nombre, days ) VALUES( DEFAULT, nombre_forma, dias );
		SELECT currval( 'formas_pago_id_seq' ) INTO id_forma;

		RETURN;
END; $$ language plpgsql;

create or replace function get_forma_pago_dias(
	IN dias integer,
	OUT id_forma integer)
returns integer as $$
declare
	query varchar( 250 );
begin
	SELECT id INTO id_forma FROM formas_pago WHERE days = dias;
	IF NOT FOUND THEN
	SELECT * INTO id_forma FROM insertar_forma_de_pago( dias || ' Dias', dias );
	END IF;
end; $$ language plpgsql;

-- anula una compra
-- postgres-functions.c:1888
create or replace function anular_compra(int4)
returns void as '
begin
UPDATE compras SET anulada=TRUE WHERE id=$1;
UPDATE products_buy_history SET anulado=TRUE WHERE id_compra=$1;
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1905
create or replace function canjear_producto(prod varchar(14), cant_a_cajear float8)
returns void as '
begin
UPDATE productos SET stock=stock-cant_a_canjear,
		     stock_pro=stock_pro+cant_a_canjear
		WHERE barcode=quote_literal(prod);

INSERT INTO canje VALUES (DEFAULT, NOW (), quote_literal(prod), cant_a_canjear);
return;
end; ' language plpgsql;

-- busca proveedores
-- proveedor.c:71
create or replace function buscar_proveedor(
       in pattern varchar(100),
       out rut integer,
       out dv varchar(1),
       out nombre varchar(100),
       out direccion varchar(100),
       out ciudad varchar(100),
       out comuna varchar(100),
       out telefono integer,
       out email varchar(300),
       out web varchar(300),
       out contacto varchar(100),
       out giro varchar(100))
returns setof record as $$
declare
	q text;
	l record;
begin
q := 'SELECT rut,dv,nombre,direccion,ciudad,comuna,telefono,email,web,contacto,giro
     	     FROM proveedor WHERE lower(nombre) LIKE lower('
	     || quote_literal($1) || ') '
	     || 'OR lower(rut::varchar) LIKE lower(' || quote_literal($1) || ') ORDER BY nombre';

for l in execute q loop
    rut = l.rut;
    dv = l.dv;
    nombre = l.nombre;
    direccion = l.direccion;
    ciudad = l.ciudad;
    comuna = l.comuna;
    telefono = l.telefono;
    email = l.email;
    web = l.web;
    contacto = l.contacto;
    giro = l.giro;
    return next;
end loop;
return;
end; $$ language plpgsql;

-- retorna todos los proveedores
-- proveedores.c:98
create or replace function select_proveedor ( OUT rut int4,
       	  	  	   		      OUT dv varchar(1),
					      OUT nombre varchar(100),
					      OUT direccion varchar(100),
					      OUT ciudad varchar(100),
					      OUT comuna varchar(100),
					      OUT telefono int4,
					      OUT email varchar(300),
					      OUT web varchar(300),
					      OUT contacto varchar(100),
					      OUT giro varchar(100))
returns setof record as $$
declare
	l record;
	q varchar(255);
begin
q := 'SELECT rut, dv, nombre, direccion, ciudad, comuna, telefono,
     email, web, contacto, giro FROM proveedor ORDER BY nombre';

for l in execute q loop
    rut := l.rut;
    dv := l.dv;
    nombre := l.nombre;
    direccion := l.direccion;
    ciudad := l.ciudad;
    comuna := l.comuna;
    telefono := l.telefono;
    email := l.email;
    web := l.web;
    contacto := l.contacto;
    giro := l.giro;
    return next;
end loop;
return;
end; $$ language plpgsql;

-- retorna el proveedor con el rut dado
-- proveedores.c:130, 186
-- ventas_stats.c:118
create or replace function select_proveedor (IN prov_rut int4,
       	  	  	   		    OUT rut int4,
					    OUT dv varchar(1),
					    OUT nombre varchar(100),
					    OUT direccion varchar(100),
  					    OUT ciudad varchar(100),
					    OUT comuna varchar(100),
					    OUT telefono int4,
					    OUT email varchar(300),
					    OUT web varchar(300),
					    OUT contacto varchar(100),
					    OUT giro varchar(100))
returns setof record as $$
declare
	list record;
	query text;
begin
query := 'SELECT rut,dv,nombre, direccion, ciudad, comuna, telefono, email,
      	 	 web, contacto, giro FROM proveedor WHERE rut=' || prov_rut;

for list in execute query loop
    rut       := list.rut;
    dv	      := list.dv;
    nombre    := list.nombre;
    direccion := list.direccion;
    ciudad    := list.ciudad;
    comuna    := list.comuna;
    telefono  := list.telefono;
    email     := list.email;
    web	      := list.web;
    contacto  := list.contacto;
    giro      := list.giro;
    return next;
end loop;

return;

end; $$ language plpgsql;

-- retorna todos los vendedores/usuarios
-- usuario.c:37
create or replace function select_usuario()
returns setof record as $$
declare
	l record;
	q varchar(255);
begin
q := 'SELECT id, rut, dv, usuario, passwd, '
	|| 'nombre,apell_p,apell_m,fecha_ingreso,"level"'
	|| 'FROM users ORDER BY id ASC';

for l in execute q loop
	return next l;
end loop;
return;
end; $$ language plpgsql;

-- retorna la última linea de asistencia
-- usuario.c:75
create or replace function select_asistencia(
       in in_id_user int,
       out entrada_year float8,
       out entrada_month float8,
       out entrada_day float8,
       out entrada_hour float8,
       out entrada_min float8,
       out salida_year float8,
       out salida_month float8,
       out salida_day float8,
       out salida_hour float8,
       out salida_min float8)
returns setof record as $$
declare
	l record;
	q text;
begin
q := $S$SELECT date_part ('year', entrada) as entrada_year,
     	       date_part ('month', entrada) as entrada_month,
	       date_part ('day', entrada) as entrada_day,
	       date_part ('hour', entrada) as entrada_hour,
	       date_part ('minute', entrada) as entrada_min,
	       date_part ('year', salida) as salida_year,
	       date_part ('month', salida) as salida_month,
	       date_part ('day', salida) as salida_day,
	       date_part ('hour', salida) as salida_hour,
	       date_part ('minute', salida) as salida_min
	FROM asistencia WHERE id_user=$S$
	|| quote_literal(in_id_user) || $S$ ORDER BY entrada DESC LIMIT 1$S$;

for l in execute q loop
       entrada_year := l.entrada_year;
       entrada_month := l.entrada_month;
       entrada_day := l.entrada_day;
       entrada_hour := l.entrada_hour;
       entrada_min := l.entrada_min;
       salida_year := l.salida_year;
       salida_month := l.salida_month;
       salida_day := l.salida_day;
       salida_hour := l.salida_hour;
       salida_min := l.salida_min;
       return next;
end loop;
return;
end; $$ language plpgsql;

-- borra un usuario
-- usuario.c:125
create or replace function delete_user(id_usuario int4)
returns void as $$
begin
DELETE FROM users WHERE id=id_usuario;

return;
end; $$ language plpgsql;

-- cancela una venta
-- ventas.c:4020
create or replace function cancelar_venta(integer)
returns integer AS '
declare
	idventa_a_cancelar ALIAS FOR $1;
	list record;
	select_detalle_venta varchar(255);
	update_stocks varchar(255);
begin
	EXECUTE ''UPDATE ventas SET canceled = TRUE WHERE id = ''
		|| quote_literal(idventa_a_cancelar);

	select_detalle_venta := ''SELECT * FROM products_sell_history WHERE id_venta = ''
				|| quote_literal(idventa_a_cancelar)
				|| '' ORDER BY id'';

	FOR list IN EXECUTE select_detalle_venta LOOP
		update_stocks := ''UPDATE productos SET stock = stock + ''
				|| list.cantidad
				|| '' WHERE barcode = ''
				|| list.barcode;
		EXECUTE update_stocks;
	END LOOP;

	RETURN 0;
END;' language plpgsql;

-- Registrar una venta
create or replace function registrar_venta( IN monto integer,
	IN maquina integer,
	IN vendedor integer,
	IN tipo_documento smallint,
	IN tipo_venta smallint,
	IN descuento smallint,
	IN id_documento integer,
	IN canceled boolean,
	OUT inserted_id integer)
returns integer as $$
declare
	query varchar(255);
	fool varchar(10);
BEGIN
	fool := canceled;
	EXECUTE $S$ INSERT INTO venta( id, monto, fecha, maquina, vendedor, tipo_documento, tipo_venta, descuento, id_documento, canceled )
	VALUES ( DEFAULT, $S$|| monto ||$S$, NOW(),$S$|| maquina ||$S$,$S$|| vendedor||$S$,$S$|| tipo_documento ||$S$,$S$||
	tipo_venta||$S$,$S$|| descuento||$S$,$S$|| id_documento||$S$,'$S$|| fool ||$S$')$S$;

	SELECT currval( 'venta_id_seq' ) INTO inserted_id;

	return;
END; $$ language plpgsql;

--registra el detalle de una venta
create or replace function registrar_venta_detalle(
       in in_id_venta int,
       in in_barcode bigint,
       in in_cantidad double precision,
       in in_precio int,
       in in_fifo int,
       in in_iva int,
       in in_otros int)
returns void as $$
declare
aux int;
num_linea int;
begin
	aux := (select count(*) from venta_detalle where id_venta = in_id_venta);

	if aux = 0 then
	   num_linea := 0;
	else
	   num_linea := (select max(id) from venta_detalle where id_venta = in_id_venta);
	   num_linea := num_linea + 1;
	end if;
	INSERT INTO venta_detalle(id,
	       	    		  id_venta,
				  barcode,
				  cantidad,
				  precio,
				  fifo,
				  iva,
				  otros)
	       	    VALUES(num_linea,
		    	   in_id_venta,
			   in_barcode,
			   in_cantidad,
			   in_precio,
			   in_fifo,
			   in_iva,
			   in_otros);

end;$$ language plpgsql;



create or replace function buscar_producto(IN expresion varchar(255),
	IN columnas varchar[],
	IN usar_like boolean,
        IN con_stock boolean,
       	OUT barcode int8,
	OUT codigo_corto varchar(10),
	OUT marca varchar(35),
	OUT descripcion varchar(50),
	OUT contenido varchar(10),
	OUT unidad varchar(10),
	OUT stock float8,
	OUT precio int4,
	OUT costo_promedio int4,
	OUT vendidos float8,
	OUT impuestos bool,
	OUT otros int4,
	OUT familia int2,
	OUT perecibles bool,
	OUT stock_min float8,
	OUT margen_promedio float8,
	OUT fraccion bool,
	OUT canje bool,
	OUT stock_pro float8,
	OUT tasa_canje float8,
	OUT precio_mayor int4,
	OUT cantidad_mayor int4,
	OUT mayorista bool)
returns setof record as $$
declare
	list record;
	query text;
	i integer;
begin
	query := $S$ SELECT barcode, codigo_corto, marca, descripcion, contenido, unidad, stock, precio, costo_promedio,
	vendidos, impuestos, otros, familia, perecibles, stock_min, margen_promedio, fraccion, canje, stock_pro,
	tasa_canje, precio_mayor, cantidad_mayor, mayorista FROM producto WHERE $S$;

        IF con_stock IS TRUE THEN
        query := query || $S$ stock > 0 and $S$;
        END IF;

        query := query || $S$($S$;

	FOR i IN 1..array_upper( columnas, 1) LOOP
	IF usar_like IS TRUE THEN
	IF i > 1 THEN
	query := query || $S$ or  $S$;
	END IF;
	query := query || $S$ upper( $S$ || columnas[i] || $S$::varchar ) $S$ || $S$ like upper( '$S$ || expresion ||$S$%' ) $S$;
	ELSE
	IF i > 1 THEN
	query := query || $S$ and $S$;
	END IF;
	query := query || columnas[i] || $S$ = upper( '$S$ || expresion || $S$' ) $S$;
	END IF;
	END LOOP;

        query := query || $S$) order by descripcion, marca$S$;

	FOR list IN EXECUTE query LOOP
	barcode := list.barcode;
	codigo_corto := list.codigo_corto;
	marca := list.marca;
	descripcion := list.descripcion;
	contenido := list.contenido;
	unidad := list.unidad;
	stock := list.stock;
	precio := list.precio;
	costo_promedio := list.costo_promedio;
	vendidos := list.vendidos;
	impuestos := list.impuestos;
	otros := list.otros;
	familia := list.familia;
	perecibles := list.perecibles;
	stock_min := list.stock_min;
	margen_promedio := list.margen_promedio;
	fraccion := list.fraccion;
	canje := list.canje;
	stock_pro := list.stock_pro;
	tasa_canje := list.tasa_canje;
	precio_mayor := list.precio_mayor;
	cantidad_mayor := list.cantidad_mayor;
	mayorista := list.mayorista;
	RETURN NEXT;
    END LOOP;

	RETURN;

END; $$ language plpgsql;

create or replace function get_compras (
		OUT id_compra integer,
		OUT nombre varchar(100),
		OUT precio double precision,
		OUT dia integer,
		OUT mes integer,
		OUT ano integer)
returns setof record as $$
declare
		list record;
		query text;
begin
		query := $S$ select compra.id,
		      proveedor.nombre,
		      SUM((compra_detalle.cantidad - compra_detalle.cantidad_ingresada) * compra_detalle.precio) as monto,
		      date_part('day', compra.fecha) as dia,
		      date_part ('month', compra.fecha) as mes,
		      date_part('year', compra.fecha) as ano

		      FROM compra
		      	   inner join proveedor on compra.rut_proveedor = proveedor.rut
			   inner join compra_detalle on compra.id = compra_detalle.id_compra

		      WHERE compra_detalle.cantidad_ingresada<compra_detalle.cantidad
			    and compra_detalle.anulado='f'

		      GROUP BY compra.id, proveedor.nombre, compra.fecha
		      ORDER BY fecha DESC $S$;

	FOR list IN EXECUTE query LOOP
	id_compra := list.id;
	nombre := list.nombre;
	precio := list.monto;
	dia := list.dia;
	mes := list.mes;
	ano := list.ano;
	RETURN NEXT;
	END LOOP;

end; $$ language plpgsql;

create or replace function get_detalle_compra(
		IN id_compra integer,
		OUT codigo_corto varchar(10),
		OUT descripcion varchar(50),
		OUT marca varchar(35),
		OUT contenido varchar(10),
		OUT unidad varchar(10),
		OUT precio double precision,
		OUT cantidad double precision,
		OUT cantidad_ingresada double precision,
		OUT costo_ingresado bigint,
		OUT barcode bigint,
		OUT precio_venta integer,
		OUT margen integer)
returns setof record as $$
declare
		list record;
		query text;
begin
		query := $S$ SELECT t2.codigo_corto, t2.descripcion, t2.marca, t2.contenido, t2.unidad, t1.precio, t1.cantidad, t1.cantidad_ingresada, (t1.precio * (t1.cantidad - t1.cantidad_ingresada))::bigint as costo_ingresado, t2.barcode, t1.precio_venta, t1.margen FROM compra_detalle AS t1, producto AS t2 WHERE t1.id_compra=$S$|| id_compra ||$S$ AND t2.barcode=t1.barcode_product AND t1.cantidad_ingresada<t1.cantidad AND t1.anulado='f'$S$;

		FOR list IN EXECUTE query LOOP
		codigo_corto := list.codigo_corto;
		descripcion := list.descripcion;
		marca := list.marca;
		contenido := list.contenido;
		unidad := list.unidad;
		precio := list.precio;
		cantidad := list.cantidad;
		cantidad_ingresada := list.cantidad_ingresada;
		costo_ingresado := list.costo_ingresado;
		barcode := list.barcode;
		precio_venta := list.precio_venta;
		margen := list.margen;
		RETURN NEXT;
		END LOOP;

END; $$ LANGUAGE plpgsql;


create or replace function get_iva(
		IN barcode bigint,
		OUT valor double precision)
returns double precision as $$
begin

		SELECT impuesto.monto INTO valor FROM producto, impuesto WHERE producto.barcode=barcode and producto.impuestos='true' AND impuesto.id=1;

end; $$ language plpgsql;

create or replace function get_otro_impuesto(
		IN barcode bigint,
		OUT valor double precision)
returns double precision as $$
begin
		SELECT impuesto.monto INTO valor FROM producto, impuesto WHERE producto.barcode=barcode AND impuesto.id=producto.otros;
end; $$ language plpgsql;

-- esta funcion es necesario que se probada, porque tiene variaciones en el
-- resultado respecto a la sentencia original
--
create or replace function select_stats_proveedor(
       	  	  IN rut int4,
		  OUT barcode int8,
		  OUT descripcion varchar(50),
		  OUT marca varchar(35),
		  OUT contenido varchar(10),
		  OUT unidad varchar(10),
		  OUT suma_cantingresada double precision,
		  OUT vendidos float8)
returns setof record as $$
declare
	l record;
	q text;
begin
q := $S$ select barcode, descripcion, marca, contenido, unidad,
     	 	sum(cantidad_ingresada) as suma, vendidos
		from compra inner join compra_detalle
		     on compra.id = compra_detalle.id_compra
		     inner join producto
		     on compra_detalle.barcode_product = producto.barcode
		where compra.rut_proveedor = $S$ || quote_literal(rut)
		|| $S$ group by 1,2,3,4,5,7 $S$;

for l in execute q loop
    barcode := l.barcode;
    descripcion := l.descripcion;
    marca := l.marca;
    contenido := l.contenido;
    unidad := l.unidad;
    suma_cantingresada := l.suma;
    vendidos := l.vendidos;
    return next;
end loop;

return;
end; $$ language plpgsql;

-- ??retorna del rut del proveedor para una compra dada
-- revisar si se puede satisfacer con alguna funcion anteriormente definida
-- compras.c:4549, 7023
create or replace function get_proveedor_compra(
		IN id_compra integer,
		OUT proveedor integer)
returns integer as $$
begin

		SELECT rut_proveedor INTO proveedor FROM compra WHERE id = id_compra;

RETURN;

END; $$ language plpgsql;

-- cancela una venta, en la tabla venta, columna canceled lo pone a TRUE
-- y retorna TODAS las cantidades de la venta al stock de producto
create or replace function cancelar_venta(IN idventa_a_cancelar int4)
returns integer AS $$
declare
	list record;
	select_detalle_venta varchar(255);
	update_stocks varchar(255);
begin
EXECUTE 'UPDATE venta SET canceled = TRUE WHERE id = ' || quote_literal(idventa_a_cancelar);

select_detalle_venta:= 'SELECT barcode, cantidad FROM venta_detalle WHERE id_venta = ' || quote_literal(idventa_a_cancelar);

FOR list IN EXECUTE select_detalle_venta LOOP
	update_stocks := 'UPDATE producto SET stock = stock + '
			|| list.cantidad
			|| ' WHERE barcode = '
			|| list.barcode;
	EXECUTE update_stocks;
END LOOP;

RETURN 0;
END;$$ language plpgsql;

-- retorna el fifo o costo_promedio de un producto
create or replace function get_fifo(in barcode_in int8)
returns int as $$
declare
	resultado int;
begin

resultado := (select costo_promedio from producto where barcode=barcode_in);
return resultado;

end; $$ language plpgsql;

create or replace function ranking_ventas(
       in starts date,
       in ends date,
       out descripcion varchar,
       out marca varchar,
       out contenido varchar,
       out unidad varchar,
       out amount double precision,
       out sold_amount double precision,
       out costo double precision,
       out contrib double precision
       )
returns setof record as $$
declare
q text;
l record;
begin

q := $S$ SELECT producto.descripcion as descripcion,
       	      producto.marca as marca,
	      producto.contenido as contenido,
	      producto.unidad as unidad,
	      SUM (venta_detalle.cantidad) as amount,
	      SUM (((venta_detalle.cantidad*venta_detalle.precio)-(venta.descuento*((venta_detalle.cantidad*venta_detalle.precio)/(venta.monto+venta.descuento))))-(venta_detalle.iva+venta_detalle.otros)::integer) as sold_amount,
	      SUM ((venta_detalle.cantidad*venta_detalle.fifo)::integer) as costo,
       	      SUM (((venta_detalle.precio*cantidad)-((iva+venta_detalle.otros)+(fifo*cantidad)))::integer) as contrib
      FROM venta, venta_detalle inner join producto on venta_detalle.barcode = producto.barcode
      where venta_detalle.id_venta=venta.id and fecha>=$S$ || quote_literal(starts) || $S$ AND fecha< $S$ || quote_literal(ends) || $S$
      AND venta.id NOT IN (SELECT id_sale FROM venta_anulada) GROUP BY 1,2,3,4 $S$;

for l in execute q loop
    descripcion := l.descripcion;
    marca := l.marca;
    contenido := l.contenido;
    unidad := l.unidad;
    amount := l.amount;
    sold_amount := l.sold_amount;
    costo := l.costo;
    contrib := l.contrib;
    return next;
end loop;

return;
end; $$ language plpgsql;

create or replace function get_guide_detail(
		IN id_guia integer,
                IN rut_proveedor integer,
		OUT codigo_corto integer,
		OUT descripcion varchar(50),
		OUT marca varchar(35),
		OUT contenido varchar(10),
		OUT unidad varchar(10),
		OUT precio integer,
		OUT cantidad double precision,
		OUT precio_compra bigint,
		OUT barcode bigint)
returns setof record as $$
declare
        list record;
        query text;
begin
        query := $S$ SELECT t2.codigo_corto, t2.descripcion, t2.marca, t2.contenido, t2.unidad, t2.precio as precio_venta, t1.precio as precio_compra, t1.cantidad, t2.barcode FROM guias_compra_detalle AS t1, producto AS t2, guias_compra as t3 WHERE t1.barcode=t2.barcode and t1.id_guias_compra=t3.id and t3.numero=$S$|| id_guia ||$S$ and t3.rut_proveedor=$S$|| rut_proveedor;

		FOR list IN EXECUTE query LOOP
		codigo_corto := list.codigo_corto;
		descripcion := list.descripcion;
		marca := list.marca;
		contenido := list.contenido;
		unidad := list.unidad;
		precio := list.precio_venta;
		cantidad := list.cantidad;
		precio_compra := list.precio_compra;
		barcode := list.barcode;
		RETURN NEXT;
		END LOOP;
END; $$ LANGUAGE plpgsql;

create or replace function guide_invoice ( IN id_invoice int,
        IN guides int[] )
returns integer as $$
DECLARE
        list record;
        query text;
        i integer;
BEGIN

        FOR i IN 1..array_upper( guides, 1) LOOP
        UPDATE guias_compra SET id_factura=id_invoice WHERE id=guides[i];
                IF FOUND IS TRUE THEN
                INSERT INTO factura_compra_detalle SELECT nextval('factura_compra_detalle_id_seq'::regclass), id_invoice, barcode, cantidad, precio, iva, otros FROM guias_compra_detalle WHERE id_guias_compra=guides[i];
                ELSE
                RETURN 0;
                END IF;
        END LOOP;

RETURN 1;

END; $$ language plpgsql;

create or replace function get_invoice_detail(
		IN id_invoice integer,
		OUT codigo_corto varchar(10),
		OUT descripcion varchar(50),
		OUT marca varchar(35),
		OUT contenido varchar(10),
		OUT unidad varchar(10),
		OUT precio integer,
		OUT cantidad double precision,
		OUT precio_compra bigint,
		OUT barcode bigint)
returns setof record as $$
declare
        list record;
        query text;
begin
        query := $S$ SELECT t2.codigo_corto, t2.descripcion, t2.marca, t2.contenido, t2.unidad, t2.precio as precio_venta, t1.precio as precio_compra, t1.cantidad, t2.barcode FROM factura_compra_detalle AS t1, producto AS t2, factura_compra as t3 WHERE t1.barcode=t2.barcode and t1.id_factura_compra=t3.id and t3.id=$S$|| id_invoice;

		FOR list IN EXECUTE query LOOP
		codigo_corto := list.codigo_corto;
		descripcion := list.descripcion;
		marca := list.marca;
		contenido := list.contenido;
		unidad := list.unidad;
		precio := list.precio_venta;
		cantidad := list.cantidad;
		precio_compra := list.precio_compra;
		barcode := list.barcode;
		RETURN NEXT;
		END LOOP;
END; $$ LANGUAGE plpgsql;

create or replace function insert_egreso (
       in in_monto int,
       in in_tipo int,
       in in_usuario int)
returns int as $$
declare
	current_caja int;
begin
select max(id) into current_caja from caja;

if (select fecha_termino from caja where id=current_caja) IS NOT NULL then
   raise notice 'Adding an egreso to caja that is closed (%)', current_caja;
end if;

insert into egreso (monto, tipo, fecha, usuario, id_caja)
       values (in_monto, in_tipo, now(), in_usuario, current_caja);

return 0;
end; $$ language plpgsql;

create or replace function insert_ingreso (
       in in_monto int,
       in in_tipo int,
       in in_usuario int)
returns int as $$
declare
	current_caja int;
begin

select max(id) into current_caja from caja;

if (select fecha_termino from caja where id=current_caja) IS NOT NULL then
   raise notice 'Adding an ingreso to caja that is closed (%)', current_caja;
end if;

insert into ingreso (monto, tipo, fecha, usuario, id_caja)
       values (in_monto, in_tipo, now(), in_usuario, current_caja);

return 0;
end; $$ language plpgsql;

create or replace function is_caja_abierta ()
returns boolean as $$
declare
	current_caja int;
	fecha_ter timestamp;
begin

select max(id) into current_caja from caja;

if current_caja IS NULL then
   return FALSE;
end if;

select fecha_termino into fecha_ter from caja where id=current_caja;

if (fecha_ter is NULL) then
   return TRUE;
else
   return FALSE;
end if;

end; $$ language plpgsql;

create or replace function trg_insert_caja()
returns trigger as $$
declare
	last_sale bigint;
begin
select last_value into last_sale from venta_id_seq;

new.id_venta_inicio = last_sale;

return new;
end; $$ language plpgsql;

create trigger trigger_insert_caja before insert
       on caja for each row
       execute procedure trg_insert_caja();

create or replace function trg_update_caja()
returns trigger as $$
declare
	last_sale bigint;
begin
select last_value into last_sale from venta_id_seq;

new.id_venta_termino = last_sale;

return new;
end; $$ language plpgsql;

create trigger trigger_update_caja before update
       on caja for each row
       execute procedure trg_update_caja();

--
-- returns the amount of money that must have a given 'caja'
-- if is passed -1 then checks the current caja
create or replace function get_arqueo_caja(
       in id_caja int)
returns int as $$
declare
	id_inicio int;
	id_termino int;
	last_caja int;
	arqueo bigint;
	egresos bigint;
	ingresos bigint;
	monto_apertura bigint;
	q varchar;
	l record;
begin

if id_caja = -1 then
    select last_value into last_caja from caja_id_seq;
else
    last_caja := id_caja;
end if;

select inicio into monto_apertura
       from caja where id = last_caja;

if monto_apertura is null then
   monto_apertura := 0;
end if;

select id_venta_inicio, id_venta_termino into id_inicio, id_termino
       from caja where id = last_caja;

if id_termino is null then
   select last_value into id_termino from venta_id_seq;
end if;

select sum(monto) into arqueo
       from venta
       where id > id_inicio and id <= id_termino
       and tipo_documento = 0;

if arqueo is null then
   arqueo := 0;
end if;

egresos := 0;
q := $S$ select monto from egreso where id_caja = $S$ || last_caja;
for l in execute q loop
    egresos := egresos + l.monto;
end loop;

ingresos := 0;

q := $S$ select monto from ingreso where id_caja = $S$ || last_caja;

for l in execute q loop
    ingresos := ingresos + l.monto;
end loop;

return (monto_apertura + arqueo + ingresos - egresos);
end; $$ language plpgsql;


create or replace function select_merma( IN barcode_in bigint,
		OUT unidades_merma double precision )
returns double precision as $$
declare
   aux int;
BEGIN

  aux := (SELECT count(*) FROM merma WHERE barcode = barcode_in);
  IF aux = 0 THEN
     unidades_merma := 0;
  ELSE
     unidades_merma := (SELECT sum(unidades) FROM merma WHERE barcode = barcode_in);
  END IF;

RETURN;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION informacion_producto_venta( IN codigo_barras bigint,
		IN in_codigo_corto varchar(10),
		OUT codigo_corto varchar(10),
                OUT barcode bigint,
		OUT descripcion varchar(50),
		OUT marca varchar(35),
		OUT contenido varchar(10),
		OUT unidad varchar(10),
		OUT stock double precision,
		OUT precio integer,
		OUT precio_mayor integer,
		OUT cantidad_mayor integer,
		OUT mayorista boolean,
		OUT stock_day double precision)
RETURNS SETOF record AS $$
declare
	days double precision;
	datos record;
	query varchar;
	codbar int8;
BEGIN

query := $S$ SELECT *,
		    (stock::float / select_ventas_dia(producto.barcode)::float) AS stock_day
		FROM producto WHERE $S$;

-- check if must use the barcode or the short code
IF codigo_barras != 0 THEN
   query := query || $S$ barcode=$S$ || codigo_barras;
ELSE
   query := query || $S$ codigo_corto=$S$ || quote_literal(in_codigo_corto);
END IF;

FOR datos IN EXECUTE query LOOP
    codigo_corto := datos.codigo_corto;
    barcode := datos.barcode;
    descripcion := datos.descripcion;
    marca := datos.marca;
    contenido := datos.contenido;
    unidad := datos.unidad;
    stock := datos.stock;
    precio := datos.precio;
    stock_day := datos.stock_day;
    mayorista := datos.mayorista;
    precio_mayor := datos.precio_mayor;
    cantidad_mayor := datos.cantidad_mayor;
    RETURN NEXT;
END LOOP;

RETURN;
END;
$$ LANGUAGE plpgsql;

----
-- nullify sale
create or replace function nullify_sale (
       in salesman_id int,
       in sale_id int)
returns boolean as $$
declare
        sale_amount integer;
        id_tipo_egreso integer;
        query text;
	l record;
begin

        select monto into sale_amount from venta where id=sale_id;
        select id into id_tipo_egreso from tipo_egreso where descrip='Nulidad de Venta';

        perform insert_egreso (sale_amount, id_tipo_egreso, salesman_id);

        insert into venta_anulada(id_sale, vendedor)
                values (sale_id, salesman_id);

        query := $S$ select * from get_sale_detail ($S$||sale_id||$S$)$S$;

        for l in execute query loop
                execute $S$update producto set stock = stock+$S$||l.amount||$S$ where barcode = $S$||l.barcode;
        end loop;

return 't';
end; $$ language plpgsql;

-- Cash Box Report

create or replace function cash_box_report (
        in cash_box_id integer,
        out open_date timestamp,
        out close_date timestamp,
        out cash_box_start integer,
        out cash_box_end integer,
        out cash_sells integer,
        out cash_outcome integer,
        out cash_income integer,
        out cash_payed_money integer,
	out cash_loss_money integer)
returns setof record as $$
declare
        query varchar;
        dat record;
        sell_first_id integer;
        sell_last_id integer;
        first_cash_box_id integer;
        last_cash_box_id integer;
begin

        select id_venta_inicio, fecha_inicio, inicio, id_venta_termino, fecha_termino, termino, perdida
        into sell_first_id, open_date, cash_box_start, sell_last_id, close_date, cash_box_end, cash_loss_money
        from caja
        where id = cash_box_id;

        -- select id_venta_inicio into sell_first_id, fecha_inicio into open_date, inicio into cintoh_box_start, id_venta_termino into sell_lintot_id, fecha_termino into close_date,
        --         termino into cintoh_box_end
        -- from caja
        -- where id = cintoh_box_id;

        if close_date is null then
                close_date := now();
        end if;

        -- To avoid problems with the first sell
        if sell_first_id = 1 then
                sell_first_id := 0;
        end if;

        if sell_last_id = 0 or sell_last_id is null then
                select sum (monto) into cash_sells
                from venta
                where id > sell_first_id and tipo_venta = 0;
        else
                select sum (monto) into cash_sells
                from venta
                where id > sell_first_id and id <= sell_last_id and tipo_venta = 0;
        end if;

        if cash_sells is null then
                cash_sells := 0;
        end if;

        select sum (monto) into cash_outcome
        from egreso
        where id_caja = cash_box_id;

        if cash_outcome is null then
                cash_outcome := 0;
        end if;

        select sum (monto) into cash_income
        from ingreso
        where id_caja = cash_box_id;

        if cash_income is null then
                cash_income := 0;
        end if;

        select sum (monto_abonado) into cash_payed_money
        from abono
        where fecha_abono >= open_date and fecha_abono <= close_date;

        if cash_payed_money is null then
                cash_payed_money := 0;
        end if;

return next;
return;

end; $$ language plpgsql;

create or replace function trg_tasks_delete()
returns trigger as $$
begin

if old.removable = 'f' then
return NULL;
else
return old;
end if;

end; $$ language plpgsql;

create trigger trigger_tasks_delete before delete
       on impuesto for each row
       execute procedure trg_tasks_delete();

create or replace function trg_egress_delete()
returns trigger as $$
begin

if old.removable = 'f' then
return NULL;
else
return old;
end if;

end; $$ language plpgsql;

create trigger trigger_egress_delete before delete
       on tipo_egreso for each row
       execute procedure trg_egress_delete();

create or replace function get_sale_detail (
        in sale_id int,
        out barcode bigint,
        out price int,
        out amount double precision
        )
returns setof record as $$
declare
        list record;
        query varchar (255);
begin

query := $S$ select barcode, precio, cantidad from venta_detalle where id_venta=$S$ || sale_id;

for list in execute query loop
        barcode := list.barcode;
        price := list.precio;
        amount := list.cantidad;
        return next;
end loop;

return;
end; $$ language plpgsql;

create or replace function calculate_fifo (
        in product_barcode bigint,
        in compra_id int)
returns integer as $$
declare
        current_fifo double precision;
        current_stock double precision;
        costo double precision;
        stock_add double precision;
        suma double precision;
        fifo integer;
begin

        select costo_promedio, stock into current_fifo, current_stock from producto where barcode=product_barcode;
        select precio, cantidad_ingresada into costo, stock_add from compra_detalle where barcode_product=product_barcode and id_compra=compra_id;

        if current_fifo is null then
            current_fifo = 0;
        end if;

        suma = current_stock * current_fifo;
        suma = suma + (stock_add * costo);

        current_stock = current_stock + stock_add;

        fifo = (suma / current_stock)::integer;

        return fifo;
end; $$ language plpgsql;

create or replace function sells_get_totals (
        in starts timestamp,
        in ends timestamp,
        out total_cash_sell integer,
        out total_cash integer,
        out total_cash_discount integer,
        out total_discount integer,
        out total_credit_sell integer,
        out total_credit integer,
        out total_ventas integer
        )
returns setof record as $$
declare
       q text;
       l record;
begin
        select SUM (descuento) into total_cash_discount from venta where fecha >= starts and fecha < ends and descuento!=0
                and (select forma_pago FROM documentos_emitidos where id=id_documento)=0 and venta.id not in (select id_sale from venta_anulada);

        select count(*) into total_discount from venta where fecha >= starts and fecha < ends and descuento!=0
                and (select forma_pago FROM documentos_emitidos where id=id_documento)=0 and venta.id not in (select id_sale from venta_anulada);


return next;
return;
end; $$ language plpgsql;

--Funcion incr_fecha
--Esta funcion incrementa la fecha en un dia mas
create or replace function incr_fecha(
	in pfecha date
	)
returns date as $$
begin
	return pfecha + integer '1';
end;
 $$ language plpgsql;

-- Registrar una devolucion
create or replace function registrar_devolucion( 
	IN monto integer,
	IN proveedor integer,
	OUT inserted_id integer)
returns integer as $$
BEGIN
	EXECUTE $S$ INSERT INTO devolucion( id, monto, fecha, proveedor)
	VALUES ( DEFAULT, $S$|| monto ||$S$, NOW(),$S$|| proveedor ||$S$ )$S$;

	SELECT currval( 'devolucion_id_seq' ) INTO inserted_id;
	return;
END; $$ language plpgsql;


--registra el detalle de una devolucion
create or replace function registrar_devolucion_detalle(
       in in_id_devolucion int,
       in in_barcode bigint,
       in in_cantidad double precision,
       in in_precio int,
       in in_precio_compra int )
returns void as $$
declare
aux int;
num_linea int;
begin
	aux := (select count(*) from devolucion_detalle where id_devolucion = in_id_devolucion);

	if aux = 0 then
	   num_linea := 0;
	else
	   num_linea := (select max(id) from devolucion_detalle where id_devolucion = in_id_devolucion);
	   num_linea := num_linea + 1;
	end if;
	INSERT INTO devolucion_detalle (id,
					id_devolucion,
					barcode,
					cantidad,
					precio,
					precio_compra)
	       	    VALUES(num_linea,
		    	   in_id_devolucion,
			   in_barcode,
			   in_cantidad,
			   in_precio,
			   in_precio_compra);

end;$$ language plpgsql;



