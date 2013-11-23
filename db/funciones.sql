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
--CREATE LANGUAGE plpgsql;

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

---
-- return the avg of ventas day
-- (si no_cero es TRUE devuelve como valor mínimo 1)
---
CREATE OR REPLACE FUNCTION select_ventas_dia (IN codbar bigint,
       	  	  	   		      IN no_cero boolean)
RETURNS double precision AS $$
DECLARE
  oldest date;
  oldest_vd date;
  oldest_vmcd date;
  last_month date;
  passed_days interval;
  total double precision;
  subtotal double precision;

BEGIN
  last_month = now() - interval '1 month';

  -- Se selecciona la fecha de venta (en venta_detalle) más antigua dentro del último mes
  SELECT date_trunc ('day', fecha) INTO oldest_vd
  FROM venta, venta_detalle
  WHERE venta.id=id_venta
        AND barcode=codbar
        AND venta.fecha>=last_month
  ORDER BY fecha ASC LIMIT 1;
  -- Se asegura que la fecha no sea null (si es que no se ha vendido el producto)
  IF oldest_vd IS NULL THEN
     oldest_vd := date_trunc ('day', now());
  END IF;

  -- Se selecciona la fecha de venta (en venta_mc_detalle) más antigua dentro del último mes
  SELECT date_trunc ('day', fecha) INTO oldest_vmcd
  FROM venta, venta_mc_detalle
  WHERE venta.id=id_venta_vd
        AND barcode_componente=codbar
        AND venta.fecha>=last_month
  ORDER BY fecha ASC LIMIT 1;
  -- Se asegura que la fecha no sea null (si es que no se ha vendido el producto)
  IF oldest_vmcd IS NULL THEN
     oldest_vmcd := date_trunc ('day', now());
  END IF;

  -- Se elije la fecha más antigua
  IF oldest_vd < oldest_vmcd THEN
     oldest := oldest_vd;
  ELSE
     oldest := oldest_vmcd;
  END IF;

  passed_days = date_trunc ('day', now()) - oldest;

  IF passed_days < interval '1 days' THEN
     passed_days = interval '1 days';
  END IF;

  -- Cantidad en venta_detalle
  SELECT (SUM (cantidad)) INTO subtotal
  FROM venta_detalle, venta
  WHERE venta.fecha >= last_month
       AND barcode=codbar
       AND venta.id=id_venta;

  total := COALESCE (subtotal, 0);
  --RAISE NOTICE '----> subt1: %, tot1: %', subtotal, total;

  -- Cantidad en venta_mc_detalle
  SELECT (SUM (cantidad)) INTO subtotal
  FROM venta_mc_detalle, venta
  WHERE venta.fecha >= last_month
       AND barcode_componente=codbar
       AND venta.id=id_venta_vd;

  total := total + COALESCE (subtotal,0);
  --RAISE NOTICE '----> subt2: %, tot2: %', subtotal, total;

  -- Se calcula el promedio de acuerdo a los días pasados
  IF passed_days = interval '30 days' THEN
     total := (total / 30);
  ELSE
     total := (total / date_part('day', passed_days));
  END IF;

  IF total = 0 AND no_cero = TRUE THEN
     total := 1;
  END IF;

RETURN total;
END; $$ LANGUAGE plpgsql;


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
		IN prod_fraccion boolean,
		IN prod_tipo integer)
returns integer as $$
begin

	IF prod_barcode = 0 THEN
	     INSERT INTO producto (codigo_corto, marca, descripcion, contenido, unidad, impuestos, otros, familia,
	     	    	           perecibles, fraccion, tipo)
	     VALUES (prod_codigo, prod_marca, prod_descripcion,prod_contenido, prod_unidad, prod_iva,
		     prod_otros, prod_familia, prod_perecible, prod_fraccion, prod_tipo);
	ELSE
	     INSERT INTO producto (barcode, codigo_corto, marca, descripcion, contenido, unidad, impuestos, otros, familia,
	     	    	           perecibles, fraccion, tipo)
	     VALUES (prod_barcode, prod_codigo, prod_marca, prod_descripcion,prod_contenido, prod_unidad, prod_iva,
		     prod_otros, prod_familia, prod_perecible, prod_fraccion, prod_tipo);
        END IF;

	IF FOUND IS TRUE THEN
			RETURN 1;
	ELSE
			RETURN 0;
	END IF;

END; $$ language plpgsql;

-- revisa si existe un producto con el mismo código
-- administracion_productos.c:1354
create or replace function existe_producto(prod_codigo_corto varchar(16))
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


-- Si la mercadería es compuesta:
-- calcula el máximo stock posible de la mercadería compuesta a partir del stock de sus componentes
-- y la cantidad que usa de cada uno de ellos.
--
-- Si es de otro tipo, simplemente retorna el stock que corresponde
--
CREATE OR REPLACE FUNCTION obtener_stock_desde_barcode ( IN codigo_barras bigint,
       	  	  	   		   	         OUT disponible double precision)
RETURNS double precision AS $$
DECLARE
	list record;
	query text;

	menor_l double precision;
	contador_l int4;
	unidades_l int4;

	tipo_l int4;
	derivada_l int4;
	compuesta_l int4;
	corriente_l int4;
	materia_prima_l int4;

	cant_mud_l double precision;
	stock_l double precision;
	disponible_l double precision;
BEGIN
	SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';
	SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
	SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';
	SELECT id INTO materia_prima_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA';
	SELECT tipo INTO tipo_l FROM producto WHERE barcode = codigo_barras;

	disponible := 0;
	contador_l := 0;

	-- SI LA MERCADERÍA ES COMPUESTA
	IF (tipo_l = compuesta_l OR tipo_l = derivada_l) THEN
	   -- SE OBTIENE LA CANTIDAD QUE REQUERIDA DE TODOS LOS PRODUCTOS SIMPLES (CORRIENTES) O MATERIAS PRIMAS
	   query := $S$
	   	    WITH RECURSIVE compuesta (barcode_complejo, barcode_componente, tipo_componente, cant_mud) AS
		    (
		     SELECT barcode_complejo, barcode_componente, tipo_componente, cant_mud
		     FROM componente_mc WHERE barcode_complejo = $S$ || codigo_barras || $S$
	 	     UNION ALL
	 	     SELECT componente_mc.barcode_complejo, componente_mc.barcode_componente,
		     	    componente_mc.tipo_componente,
		    	    componente_mc.cant_mud * compuesta.cant_mud
	             FROM componente_mc, compuesta
		     WHERE componente_mc.barcode_complejo = compuesta.barcode_componente
		    )
		    SELECT barcode_componente, tipo_componente, SUM(compuesta.cant_mud) AS cantidad
		    FROM compuesta
		    WHERE tipo_componente != $S$ || compuesta_l || $S$ AND tipo_componente != $S$ || derivada_l || $S$
		    GROUP BY barcode_componente, tipo_componente$S$;

	   -- OBTENER EL STOCK DE SUS COMPONENTES Y VER PARA CUANTOS COMPUESTOS ALCANZAN
	   FOR list IN EXECUTE query LOOP
	       -- Calcula las unidades disponibles de la mercadería para este compuesto
	       IF (list.tipo_componente = corriente_l OR list.tipo_componente = materia_prima_l) THEN
	       	  stock_l := (SELECT * FROM obtener_stock_desde_barcode (list.barcode_componente));
	       	  unidades_l := TRUNC (stock_l / list.cantidad);
	       END IF;

	       -- Elige la cantidad menor como su stock
	       IF (contador_l = 0 OR menor_l > unidades_l) THEN
	       	  menor_l := unidades_l;
	       END IF;

	       contador_l := contador_l + 1;
	   END LOOP;

	   IF (menor_l >= 0) THEN
	      disponible_l := menor_l;
           END IF;

	-- SI ES UNA MERCADERÍA CORRIENTE
	ELSIF (tipo_l = corriente_l  OR tipo_l = materia_prima_l) THEN
	   disponible_l := (SELECT stock FROM producto WHERE barcode = codigo_barras);
	END IF;

	disponible := disponible_l;

RETURN; -- Retorna el valor de "disponible"
END; $$ language plpgsql;


-- retorna TODOS los productos
-- administracion_productos.c:1376
-- NO RETORNA merma_unid
create or replace function select_producto (OUT barcode int8,
					    OUT codigo_corto varchar(16),
					    OUT marca varchar(35),
					    OUT descripcion varchar(50),
					    OUT contenido varchar(10),
					    OUT unidad varchar(10),
					    OUT stock float8,
					    OUT precio double precision,
					    OUT precio_neto double precision,
					    OUT costo_promedio float8,
					    OUT vendidos float8,
					    OUT impuestos bool,
					    OUT otros int4,
					    OUT familia int2,
					    OUT perecibles bool,
					    OUT stock_min float8,
					    OUT dias_stock float8,
					    OUT margen_promedio float8,
					    OUT fraccion bool,
					    OUT canje bool,
					    OUT stock_pro float8,
					    OUT tasa_canje float8,
					    OUT precio_mayor double precision,
					    OUT cantidad_mayor double precision,
					    OUT mayorista bool,
					    OUT tipo int4)
returns setof record as $$
declare
	list record;
	query text;
	corriente int4;
	materia_prima int4;
begin
query := $S$ SELECT codigo_corto, barcode, descripcion, marca, contenido,
      	     	    unidad, stock, precio, precio_neto, costo_promedio, vendidos, impuestos,
		    otros, familia, perecibles, margen_promedio, dias_stock,
		    COALESCE ((dias_stock * select_ventas_dia(producto.barcode, TRUE)::float), 0) AS stock_min,
		    fraccion, canje, stock_pro, tasa_canje, precio_mayor,
		    cantidad_mayor, mayorista, tipo
		    FROM producto ORDER BY descripcion, marca$S$;

corriente := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'CORRIENTE');
materia_prima := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'MATERIA PRIMA');

FOR list IN EXECUTE query LOOP
    barcode := list.barcode;
    codigo_corto := list.codigo_corto;
    marca := list.marca;
    descripcion := list.descripcion;
    contenido := list.contenido;
    unidad := list.unidad;

    -- Su la mercadería es derivada, calcula su stock de acuerdo a sus componentes
    IF list.tipo != corriente AND list.tipo != materia_prima THEN
	stock := (SELECT disponible FROM obtener_stock_desde_barcode (list.barcode));
    ELSE
	stock := list.stock;
    END IF;

    precio := list.precio;
    precio_neto := list.precio_neto;

    -- Si la mercadería es derivada, calcula su costo promedio a partir del costo de sus componentes
    IF list.tipo != corriente AND list.tipo != materia_prima THEN
        costo_promedio := (SELECT costo FROM obtener_costo_promedio_desde_barcode (list.barcode));
    ELSE
	costo_promedio := list.costo_promedio;
    END IF;

    vendidos := list.vendidos;
    impuestos := list.impuestos;
    otros := list.otros;
    familia := list.familia;
    perecibles := list.perecibles;
    stock_min := list.stock_min;
    dias_stock := list.dias_stock;
    margen_promedio := list.margen_promedio;
    fraccion := list.fraccion;
    canje := list.canje;
    stock_pro := list.stock_pro;
    tasa_canje := list.tasa_canje;
    precio_mayor := list.precio_mayor;
    cantidad_mayor := list.cantidad_mayor;
    mayorista := list.mayorista;
    tipo := list.tipo;
    RETURN NEXT;
END LOOP;

RETURN;
END; $$ language plpgsql;


-- Calcula el costo del producto compuesto a partir del costo de todos los productos
-- que lo componen.
CREATE OR REPLACE FUNCTION obtener_costo_promedio_desde_barcode ( IN codigo_barras bigint,
       	  	  	   			      		  OUT costo double precision)
RETURNS double precision AS $$
DECLARE
	list record;
	sub_list record;
	query text;

	corriente_l int4;
	compuesta_l int4;
	derivada_l int4;
	materia_prima_l int4;
	tipo_l int4;
	costo_l double precision;
BEGIN
	SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';
	SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
	SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';
	SELECT id INTO materia_prima_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA';
	SELECT tipo INTO tipo_l FROM producto WHERE barcode = codigo_barras;

	costo := 0;

	-- PROCEDIMIENTOS SEGÚN EL TIPO DE LA MERCADERIA
	-- SI ES COMPUESTA O DERIVADA
	IF (tipo_l = compuesta_l OR tipo_l = derivada_l) THEN
	   WITH RECURSIVE compuesta (barcode_complejo, barcode_componente, tipo_componente, cant_mud) AS
		(
		  SELECT barcode_complejo, barcode_componente, tipo_componente, cant_mud
		  FROM componente_mc WHERE barcode_complejo = codigo_barras
	 	  UNION ALL
	 	  SELECT componente_mc.barcode_complejo, componente_mc.barcode_componente,
		         componente_mc.tipo_componente,
		      	 componente_mc.cant_mud * compuesta.cant_mud
	          FROM componente_mc, compuesta
		  WHERE componente_mc.barcode_complejo = compuesta.barcode_componente
		)
	   SELECT SUM((SELECT * FROM obtener_costo_promedio_desde_barcode (barcode_componente))
	   	      	      	     					  * compuesta.cant_mud) INTO costo_l
	   FROM compuesta
	   WHERE tipo_componente != compuesta_l
	   AND tipo_componente != derivada_l;

	-- SI ES CORRIENTE
	ELSIF (tipo_l = corriente_l OR tipo_l = materia_prima_l) THEN
	   SELECT costo_promedio INTO costo_l FROM producto WHERE barcode = codigo_barras;
	END IF;

	costo := costo_l;

RETURN; -- Retorna el costo del producto
END; $$ language plpgsql;


-- Informacion de los productos para la ventana de Mercaderia
-- administracion_productos.c:1508
CREATE OR REPLACE FUNCTION informacion_producto( IN codigo_barras bigint,
		IN in_codigo_corto varchar(16),
		OUT codigo_corto varchar(16),
                OUT barcode bigint,
		OUT descripcion varchar(50),
		OUT marca varchar(35),
		OUT contenido varchar(10),
		OUT unidad varchar(10),
		OUT stock double precision,
		OUT precio double precision,
		OUT precio_neto double precision,
		OUT costo_promedio double precision,
		OUT stock_min double precision,
		OUT dias_stock double precision,
		OUT margen_promedio double precision,
		OUT contrib_agregada double precision,
		OUT ventas_dia double precision,
		OUT vendidos double precision,
		OUT venta_neta double precision,
		OUT canje boolean,
		OUT stock_pro double precision,
		OUT tasa_canje double precision,
		OUT precio_mayor double precision,
		OUT cantidad_mayor double precision,
		OUT mayorista boolean,
		OUT unidades_merma double precision,
		OUT stock_day double precision,
                OUT total_vendido double precision,
		OUT estado boolean,
		OUT tipo int4)
RETURNS SETOF record AS $$
declare
	days double precision;
	datos record;
	query varchar;
	codbar int8;
	corriente int4;
	materia_prima int4;
BEGIN

query := $S$ SELECT *,
      	     	    (SELECT SUM ((cantidad * precio) - (iva + otros + (fifo * cantidad))) FROM venta_detalle WHERE barcode=producto.barcode) as contrib_agregada,
		    (stock::float / select_ventas_dia(producto.barcode, TRUE)::float) AS stock_day,
		    COALESCE ((dias_stock * select_ventas_dia(producto.barcode, TRUE)::float), 0) AS stock_min,
		    (SELECT SUM ((cantidad * precio) - (iva + otros)) FROM venta_detalle WHERE barcode=producto.barcode) AS total_vendido,
		    select_merma (producto.barcode) AS unidades_merma, dias_stock,
		    select_ventas_dia(producto.barcode, FALSE) AS ventas_dia
		FROM producto WHERE $S$;

-- check if must use the barcode or the short code
IF codigo_barras != 0 THEN
   query := query || $S$ barcode=$S$ || codigo_barras;
ELSE
   query := query || $S$ codigo_corto=$S$ || quote_literal(in_codigo_corto);
END IF;

corriente := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'CORRIENTE');
materia_prima := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'MATERIA PRIMA');

FOR datos IN EXECUTE query LOOP
    codigo_corto := datos.codigo_corto;
    barcode := datos.barcode;
    descripcion := datos.descripcion;
    marca := datos.marca;
    contenido := datos.contenido;
    unidad := datos.unidad;

    IF datos.tipo = corriente OR datos.tipo = materia_prima THEN
       stock := datos.stock;
    ELSE
       stock := (SELECT disponible FROM obtener_stock_desde_barcode (datos.barcode));
    END IF;

    precio := datos.precio;
    precio_neto := datos.precio;

    -- Costo_promedio de los compuestos debería estar en el producto mismo?
    IF datos.tipo != corriente AND datos.tipo != materia_prima THEN
        costo_promedio := (SELECT costo FROM obtener_costo_promedio_desde_barcode (datos.barcode));
    ELSE
	costo_promedio := datos.costo_promedio;
    END IF;

    stock_min := datos.stock_min;
    dias_stock := datos.dias_stock;
    stock_day := datos.stock_day;
    margen_promedio := datos.margen_promedio;
    contrib_agregada := round(datos.contrib_agregada);
    unidades_merma := datos.unidades_merma;
    mayorista := datos.mayorista;
    total_vendido := datos.total_vendido;
    precio_mayor := datos.precio_mayor;
    cantidad_mayor := datos.cantidad_mayor;
    ventas_dia := datos.ventas_dia;
    estado := datos.estado;
    tipo := datos.tipo;
    RETURN NEXT;
END LOOP;

RETURN;
END;
$$ LANGUAGE plpgsql;


create or replace function select_merma(IN barcode_in bigint,
		  	   		OUT unidades_merma double precision)
returns double precision as $$
declare
   aux int;
   derivada_l int4;
BEGIN
  aux := (SELECT COALESCE (count(*), 0) FROM merma WHERE barcode = barcode_in);
  aux := aux + (SELECT COALESCE (count(*), 0) FROM merma_mc_detalle WHERE barcode_componente = barcode_in);
  IF aux = 0 THEN
     unidades_merma := 0;
  ELSE
     unidades_merma := (SELECT COALESCE (sum(unidades), 0) FROM merma WHERE barcode = barcode_in);
     unidades_merma := unidades_merma + (SELECT COALESCE (sum(cantidad), 0) FROM merma_mc_detalle WHERE barcode_componente = barcode_in);
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
					    OUT codigo_corto varchar(16),
					    OUT marca varchar(35),
					    OUT descripcion varchar(50),
					    OUT contenido varchar(10),
					    OUT unidad varchar(10),
					    OUT stock float8,
					    OUT precio float8,
					    OUT precio_neto float8,
					    OUT costo_promedio float8,
					    OUT vendidos float8,
					    OUT impuestos bool,
					    OUT otros int4,
					    OUT familia int2,
					    OUT perecibles bool,
					    OUT stock_min float8,
					    OUT dias_stock float8,
					    OUT margen_promedio float8,
					    OUT fraccion bool,
					    OUT canje bool,
					    OUT stock_pro float8,
					    OUT tasa_canje float8,
					    OUT precio_mayor float8,
					    OUT cantidad_mayor float8,
					    OUT mayorista bool,
					    OUT tipo int4)
returns setof record as $$
declare
	list record;
	query text;
	corriente int4;
	materia_prima int4;
begin
query := $S$ SELECT barcode, codigo_corto, marca, descripcion, contenido,
      	     	    unidad, stock, precio, precio_neto, costo_promedio, vendidos, impuestos,
		    otros, familia, perecibles, margen_promedio,
		    COALESCE ((dias_stock * select_ventas_dia(producto.barcode, TRUE)::float), 0) AS stock_min,
		    fraccion, canje, stock_pro, tasa_canje, precio_mayor, dias_stock,
		    cantidad_mayor, mayorista, tipo
             FROM producto WHERE estado = true AND (lower(descripcion) LIKE lower($S$
	|| quote_literal(expresion) || $S$) OR lower(marca) LIKE lower($S$
	|| quote_literal(expresion) || $S$) OR upper(codigo_corto) LIKE upper($S$
	|| quote_literal(expresion) || $S$)) ORDER BY descripcion, marca $S$;

corriente := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'CORRIENTE');
materia_prima := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'MATERIA PRIMA');

FOR list IN EXECUTE query LOOP
    barcode := list.barcode;
    codigo_corto := list.codigo_corto;
    marca := list.marca;
    descripcion := list.descripcion;
    contenido := list.contenido;
    unidad := list.unidad;

    -- Su la mercadería es derivada, calcula su stock de acuerdo a sus componentes
    IF list.tipo != corriente AND list.tipo != materia_prima THEN
	stock := (SELECT disponible FROM obtener_stock_desde_barcode (list.barcode));
    ELSE
	stock := list.stock;
    END IF;

    precio := list.precio;
    precio_neto := list.precio_neto;

    -- Si la mercadería es derivada, calcula su costo promedio a partir del costo de sus componentes
    IF list.tipo != corriente AND list.tipo != materia_prima THEN
        costo_promedio := (SELECT costo FROM obtener_costo_promedio_desde_barcode (list.barcode));
    ELSE
	costo_promedio := list.costo_promedio;
    END IF;

    vendidos := list.vendidos;
    impuestos := list.impuestos;
    otros := list.otros;
    familia := list.familia;
    perecibles := list.perecibles;
    stock_min := list.stock_min;
    dias_stock := list.dias_stock;
    margen_promedio := list.margen_promedio;
    fraccion := list.fraccion;
    canje := list.canje;
    stock_pro := list.stock_pro;
    tasa_canje := list.tasa_canje;
    precio_mayor := list.precio_mayor;
    cantidad_mayor := list.cantidad_mayor;
    mayorista := list.mayorista;
    tipo := list.tipo;
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
					    OUT codigo_corto varchar(16),
					    OUT marca varchar(35),
					    OUT descripcion varchar(50),
					    OUT contenido varchar(10),
					    OUT unidad varchar(10),
					    OUT stock float8,
					    OUT precio double precision,
					    OUT precio_neto double precision,
					    OUT costo_promedio float8,
					    OUT vendidos float8,
					    OUT impuestos bool,
					    OUT otros int4,
					    OUT familia int2,
					    OUT perecibles bool,
					    OUT stock_min float8,
					    OUT dias_stock float8,
					    OUT margen_promedio float8,
					    OUT fraccion bool,
					    OUT canje bool,
					    OUT stock_pro float8,
					    OUT tasa_canje float8,
					    OUT precio_mayor double precision,
					    OUT cantidad_mayor double precision,
					    OUT mayorista bool,
					    OUT tipo int4)
returns setof record as $$
declare
	list record;
	query text;
	corriente int4;
	materia_prima int4;
begin
query := $S$ SELECT codigo_corto, barcode, descripcion, marca, contenido,
      	     	    unidad, stock, precio, precio_neto, costo_promedio, vendidos, impuestos,
		    otros, familia, perecibles, margen_promedio,
		    COALESCE ((dias_stock * select_ventas_dia(producto.barcode, TRUE)::float), 0) AS stock_min,
		    fraccion, canje, stock_pro, tasa_canje, precio_mayor, dias_stock,
		    cantidad_mayor, mayorista, tipo
             FROM producto WHERE barcode= $S$
	     || quote_literal(prod_barcode);

corriente := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'CORRIENTE');
materia_prima := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'MATERIA PRIMA');

FOR list IN EXECUTE query LOOP
    barcode := list.barcode;
    codigo_corto := list.codigo_corto;
    marca := list.marca;
    descripcion := list.descripcion;
    contenido := list.contenido;
    unidad := list.unidad;

    -- Su la mercadería es derivada, calcula su stock de acuerdo a sus componentes
    IF list.tipo != corriente AND list.tipo != materia_prima THEN
	stock := (SELECT disponible FROM obtener_stock_desde_barcode (list.barcode));
    ELSE
	stock := list.stock;
    END IF;

    precio := list.precio;
    precio_neto := list.precio_neto;

    -- Si la mercadería es derivada, calcula su costo promedio a partir del costo de sus componentes
    IF list.tipo != corriente AND list.tipo != materia_prima THEN
        costo_promedio := (SELECT costo FROM obtener_costo_promedio_desde_barcode (list.barcode));
    ELSE
	costo_promedio := list.costo_promedio;
    END IF;

    vendidos := list.vendidos;
    impuestos := list.impuestos;
    otros := list.otros;
    familia := list.familia;
    perecibles := list.perecibles;
    stock_min := list.stock_min;
    dias_stock := list.dias_stock;
    margen_promedio := list.margen_promedio;
    fraccion := list.fraccion;
    canje := list.canje;
    stock_pro := list.stock_pro;
    tasa_canje := list.tasa_canje;
    precio_mayor := list.precio_mayor;
    cantidad_mayor := list.cantidad_mayor;
    mayorista := list.mayorista;
    tipo := list.tipo;
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


-- This function returns the total contribution
-- of the merchandise on stock
CREATE OR REPLACE FUNCTION contribucion_total_stock (OUT monto_contribucion float8)
RETURNS float8 AS $$
DECLARE
	query varchar(255);
	monto_pci float8; -- monto productos con impuestos
	monto_psi float8; -- monto productos sin impuestos
	--------------------
	corriente int4; -- id de mercadería del tipo discreta (normal)
	derivada int4; -- id de mercaderia derivada
	--------------------
BEGIN

   corriente := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'CORRIENTE');
   derivada := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'DERIVADA');

   -- Productos con impuestos
   SELECT SUM (( (precio / (SELECT (SUM(monto)/100)+1 FROM impuesto WHERE id = otros OR id = 1)) -
   	         (SELECT costo FROM obtener_costo_promedio_desde_barcode (barcode)) ) *
               (SELECT disponible FROM obtener_stock_desde_barcode (barcode)))
   INTO monto_pci
   FROM producto WHERE impuestos = TRUE
   AND tipo = corriente
   OR tipo = derivada;

   -- Productos sin impuestos   -- costo_promedio
   SELECT (SELECT SUM( (precio - (SELECT costo FROM obtener_costo_promedio_desde_barcode (barcode))) *
   	  	       (SELECT disponible FROM obtener_stock_desde_barcode (barcode))))
   INTO monto_psi
   FROM producto WHERE impuestos = FALSE
   AND tipo = corriente
   OR tipo = derivada;

   -- Contribucion total stock
   monto_contribucion := COALESCE(monto_pci,0) + COALESCE(monto_psi,0);

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
       	  	  	   (IN prod_codigo_corto varchar(16),
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
create or replace function codigo_corto_libre(varchar(16))
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
       OUT credito_enable boolean,
       OUT activo boolean,
       OUT tipo tc)
returns setof record as $$
declare
	l record;
	query varchar(255);
begin
query := 'select rut, dv, nombre, apell_p, apell_m, giro, direccion, telefono,
      	 	  telefono_movil, mail, abonado, credito, credito_enable, activo, tipo
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
    activo = l.activo;
    tipo = l.tipo;
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

-- retorna la deuda total de un cliente a partir de su rut
-- postgres-functions.c:415
CREATE OR REPLACE FUNCTION deuda_total_cliente (IN rut_cliente int4)
RETURNS INTEGER AS $$
DECLARE
	resultado INTEGER;
BEGIN

resultado := (SELECT SUM (monto) FROM search_deudas_cliente (rut_cliente, true));

RETURN resultado;
END; $$ LANGUAGE plpgsql;

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

		INSERT INTO compra(id, fecha, rut_proveedor, pedido, forma_pago, ingresada, anulada ) VALUES (DEFAULT, NOW(), proveedor, n_pedido, id_forma_pago, 'f', 'f');

		SELECT currval(  'compra_id_seq' ) INTO id_compra;

		return;
end; $$ language plpgsql;

-- inserta el detalle de una compra
-- postgres-functions.c:808, 814
-- SELECT * FROM insertar_detalle_compra(5646, 1.00::double precision, 191.00::double precision, 300, 0::double precision, 0::smallint, 7654321, 20, 36, 23);
CREATE OR REPLACE FUNCTION insertar_detalle_compra (IN id_compra_in integer,
		  	   			    IN cantidad double precision,
						    IN costo double precision,
						    IN precio_venta double precision,
						    IN precio_neto double precision,
						    IN cantidad_ingresada double precision,
						    IN descuento smallint,
						    IN barcode_product bigint,
						    IN margen double precision,
						    IN iva double precision,
						    IN otros_impuestos double precision)
RETURNS void AS $$
DECLARE
	aux int;
	q text;
BEGIN
-- se revisa si existe algun detalle ingresado con el id_compra dado
aux = (select count(*) from compra_detalle where id_compra=id_compra_in);

if aux = 0 then -- si no existen detalles para ese id_compra se usa el id:=0
   aux := 0;
else -- en caso contrario se saca el mayor id ingresado para el id_compra dado y se incrementa
   aux := (select max(id) from compra_detalle where id_compra=id_compra_in);
   aux := aux + 1;
end if;

q := $S$INSERT INTO compra_detalle(id, id_compra, cantidad, precio, precio_venta, precio_neto, cantidad_ingresada, descuento, barcode_product, margen, iva, otros_impuestos) VALUES ($S$
  || aux || $S$,$S$
  || id_compra_in || $S$,$S$
  || cantidad || $S$,$S$
  || costo || $S$,$S$
  || precio_venta || $S$,$S$
  || precio_neto || $S$,$S$
  || cantidad_ingresada || $S$,$S$
  || descuento || $S$,$S$
  || barcode_product || $S$,$S$
  || margen || $S$,$S$
  || iva || $S$,$S$
  || otros_impuestos || $S$)$S$;

EXECUTE q;


q := $S$ UPDATE producto SET precio=$S$|| precio_venta ||$S$ WHERE barcode=$S$|| barcode_product;

execute q;

RETURN;
END; $$ LANGUAGE plpgsql;


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
       out telefono varchar(100),
       out email varchar(300),
       out web varchar(300),
       out contacto varchar(100),
       out giro varchar(100),
       out lapso_reposicion integer)
returns setof record as $$
declare
	q text;
	l record;
begin
q := 'SELECT rut,dv,nombre,direccion,ciudad,comuna,telefono,email,web,contacto,giro,lapso_reposicion
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
    lapso_reposicion = l.lapso_reposicion;
    return next;
end loop;
return;
end; $$ language plpgsql;

-- busca emisores
-- proveedor.c:71
create or replace function buscar_emisor(
       in pattern varchar(100),
       out id integer,
       out rut integer,
       out dv varchar(1),
       out razon_social varchar(100),
       out telefono varchar(100),
       out direccion varchar(100),
       out comuna varchar(100),
       out ciudad varchar(100),
       out giro varchar(100))
returns setof record as $$
declare
	q text;
	l record;
begin
q := 'SELECT id, rut, dv, razon_social, telefono, direccion, comuna, ciudad, giro
     	     FROM emisor_cheque WHERE lower(razon_social) LIKE lower('
	     || quote_literal($1) || ') '
	     || 'OR lower(rut::varchar) LIKE lower(' || quote_literal($1) || ') ORDER BY razon_social';

for l in execute q loop
    id = l.id;
    rut = l.rut;
    dv = l.dv;
    razon_social = l.razon_social;
    telefono = l.telefono;
    direccion = l.direccion;
    comuna = l.comuna;
    ciudad = l.ciudad;
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
					      OUT telefono varchar(100),
					      OUT email varchar(300),
					      OUT web varchar(300),
					      OUT contacto varchar(100),
					      OUT giro varchar(100),
					      OUT lapso_reposicion integer)
returns setof record as $$
declare
	l record;
	q varchar(255);
begin
q := 'SELECT rut, dv, nombre, direccion, ciudad, comuna, telefono,
     email, web, contacto, giro, lapso_reposicion FROM proveedor ORDER BY nombre';

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
    lapso_reposicion := l.lapso_reposicion;
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
					    OUT telefono varchar(100),
					    OUT email varchar(300),
					    OUT web varchar(300),
					    OUT contacto varchar(100),
					    OUT giro varchar(100),
					    OUT lapso_reposicion integer)
returns setof record as $$
declare
	list record;
	query text;
begin
query := 'SELECT rut,dv,nombre, direccion, ciudad, comuna, telefono, email,
      	 	 web, contacto, giro, lapso_reposicion FROM proveedor WHERE rut=' || prov_rut;

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
    lapso_reposicion := list.lapso_reposicion;
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

---
-- Aumenta el stock de la mercadería en la cantidad especificada, según su tipo
-- (si es compuesta aumenta el stock de sus componentes, si es derivada aumenta el de su materia prima)
---
CREATE OR replace function aumentar_stock_desde_barcode (barcode_in bigint,
       	  	  	   			         cantidad_in double precision)
RETURNS void AS $$
DECLARE
   -----------------
   cantidad_mp double precision;
   -----------------
   derivada_l int4;
   compuesta_l int4;
   corriente_l int4;
   materia_prima_l int4;
   -----------------
   tipo_l int4;
   -----------------
   q text;
   l record;
   -----------------
BEGIN
   -- Se obtienen los id de los tipo de mercaderías
   SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
   SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';
   SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';
   SELECT id INTO materia_prima_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA';

   -- Se obtiene el tipo de producto
   SELECT tipo INTO tipo_l FROM producto WHERE barcode = barcode_in;

   -- De acuerdo al tipo de mercadería
   -- Si es compuesta
   IF (tipo_l = compuesta_l OR tipo_l = derivada_l) THEN
      CREATE TEMPORARY TABLE componentes_compuesto AS
      WITH RECURSIVE compuesta (barcode_complejo, tipo_complejo, barcode_componente, tipo_componente, cant_mud) AS
         (
	  SELECT barcode_complejo, tipo_complejo, barcode_componente, tipo_componente, cant_mud
       	  FROM componente_mc WHERE barcode_complejo = barcode_in
          UNION ALL
          SELECT componente_mc.barcode_complejo, componente_mc.tipo_complejo,
	         componente_mc.barcode_componente, componente_mc.tipo_componente,
           	 componente_mc.cant_mud * compuesta.cant_mud
          FROM componente_mc, compuesta
       	  WHERE componente_mc.barcode_complejo = compuesta.barcode_componente
      	 )
      	 SELECT c.barcode_componente AS barcode, c.tipo_componente AS tipo, c.cant_mud AS cantidad
	 FROM compuesta c
	 WHERE c.tipo_componente != compuesta_l
	 AND c.tipo_componente != derivada_l;

	 q := $S$SELECT * FROM componentes_compuesto$S$;

	 -- Se aumenta el stock de los componentes de la mercadería compuesta
	 FOR l IN EXECUTE q LOOP
	     IF (l.tipo = materia_prima_l OR l.tipo = corriente_l) THEN
      	        UPDATE producto SET stock = stock + (l.cantidad * cantidad_in) WHERE barcode = l.barcode;
	     END IF;
	 END LOOP;
	 -- Se elimina la tabla temporal
  	 DROP TABLE componentes_compuesto;
   -- Si es corriente o materia prima
   ELSEIF (tipo_l = materia_prima_l OR tipo_l = corriente_l) THEN
      UPDATE producto SET stock = stock + cantidad_in WHERE barcode = barcode_in;
   END IF;

RETURN;
END; $$ language plpgsql;


---
-- Disminuye el stock de la mercadería en la cantidad especificada, según su tipo
-- (si es compuesta disminuye el stock de sus componentes, si es derivada disminuye
--  el de su materia prima)
---
CREATE OR replace function disminuir_stock_desde_barcode (barcode_in bigint,
       	  	  	   				  cantidad_in double precision)
RETURNS void AS $$
DECLARE
   -----------------
   cantidad_mp double precision;
   -----------------
   derivada_l int4;
   compuesta_l int4;
   corriente_l int4;
   materia_prima_l int4;
   -----------------
   tipo_l int4;
   -----------------
   q text;
   l record;
   -----------------
BEGIN
   -- Se obtienen los id de los tipo de mercaderías
   SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
   SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';
   SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';
   SELECT id INTO materia_prima_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA';

   -- Se obtiene el tipo de producto
   SELECT tipo INTO tipo_l FROM producto WHERE barcode = barcode_in;

   -- De acuerdo al tipo de mercadería
   -- Si es compuesta
   IF (tipo_l = compuesta_l OR tipo_l = derivada_l) THEN
      CREATE TEMPORARY TABLE componentes_compuesto AS
      WITH RECURSIVE compuesta (barcode_complejo, tipo_complejo, barcode_componente, tipo_componente, cant_mud) AS
         (
	  SELECT barcode_complejo, tipo_complejo, barcode_componente, tipo_componente, cant_mud
       	  FROM componente_mc WHERE barcode_complejo = barcode_in
          UNION ALL
          SELECT componente_mc.barcode_complejo, componente_mc.tipo_complejo,
	         componente_mc.barcode_componente, componente_mc.tipo_componente,
           	 componente_mc.cant_mud * compuesta.cant_mud
          FROM componente_mc, compuesta
       	  WHERE componente_mc.barcode_complejo = compuesta.barcode_componente
      	 )
      	 SELECT c.barcode_componente AS barcode, c.tipo_componente AS tipo, c.cant_mud AS cantidad
	 FROM compuesta c
	 WHERE c.tipo_componente != compuesta_l
	 AND c.tipo_componente != derivada_l;

	 q := $S$SELECT * FROM componentes_compuesto$S$;

	 -- Se disminuye el stock de los componentes de la mercadería compuesta
	 FOR l IN EXECUTE q LOOP
	     IF (l.tipo = materia_prima_l OR l.tipo = corriente_l) THEN
      	        UPDATE producto SET stock = stock - (l.cantidad * cantidad_in) WHERE barcode = l.barcode;
	     END IF;
	 END LOOP;
	 -- Se elimina la tabla temporal
  	 DROP TABLE componentes_compuesto;
   -- Si es corriente o materia prima
   ELSEIF (tipo_l = materia_prima_l OR tipo_l = corriente_l) THEN
      UPDATE producto SET stock = stock - cantidad_in WHERE barcode = barcode_in;
   END IF;

RETURN;
END; $$ language plpgsql;


--registra el detalle de una venta
create or replace function registrar_venta_detalle(
       in in_id_venta int,
       in in_barcode bigint,
       in in_cantidad double precision,
       in in_precio double precision, --precio de venta (proporcional en caso de descuento)
       in in_precio_neto double precision, --precio de venta neto (proporcional en caso de descuento)
       in in_fifo double precision,
       in in_iva double precision,
       in in_otros double precision,
       in in_iva_residual double precision,
       in in_otros_residual double precision,
       in in_ganancia double precision,
       in in_tipo int4,
       in in_impuestos boolean,
       in in_proporcion_iva double precision,
       in in_proporcion_otros double precision)
returns void as $$
declare
   aux int;
   num_linea int;
   id_venta_detalle_l int4;
   ----
   compuesta_l int4;
   derivada_l int4;
   materia_prima_l int4;
   corriente_l int4;
   ----
   barcode_mp bigint;
   precio_mp int4;
   costo_mp double precision;
   cantidad_mp double precision;
   ----
begin
	SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
	SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';
	SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';

	-- Es necesario esto?, mejor usar DEFAULT como id_venta_detalle
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
				  precio_neto,
				  fifo,
				  iva,
				  otros,
				  iva_residual,
				  otros_residual,
				  ganancia,
				  tipo,
				  impuestos,
				  proporcion_iva,
				  proporcion_otros)
	       	    VALUES(num_linea,
		    	   in_id_venta,
			   in_barcode,
			   in_cantidad,
			   in_precio,
			   in_precio_neto,
			   in_fifo,
			   in_iva,
			   in_otros,
			   in_iva_residual,
			   in_otros_residual,
			   in_ganancia,
			   in_tipo,
			   in_impuestos,
			   in_proporcion_iva,
			   in_proporcion_otros) returning id into id_venta_detalle_l;

	-- Si es una mercadería compuesta, se registrará todo su detalle en venta_mc_detalle
	IF (in_tipo = compuesta_l OR in_tipo = derivada_l) THEN -- NOTA:registrar_detalle_compuesto actualiza el stock de sus componentes por si solo
	   PERFORM registrar_detalle_compuesto (in_id_venta::int4, num_linea::int4, in_barcode::bigint,
			                        ARRAY[0,0]::int[], 0::double precision, in_precio::double precision, in_cantidad::double precision,
						in_proporcion_iva::double precision, in_proporcion_otros::double precision);
	-- Si es mercadería corriente
	ELSIF (in_tipo = corriente_l OR in_tipo = materia_prima_l) THEN -- La materia prima no se vende directamente, pero se coloca aquí por si acaso
	   -- Actualiza el stock de la mercadería corriente (lo disminuye segun lo vendido)
	   PERFORM disminuir_stock_desde_barcode (in_barcode, in_cantidad);
	END IF;
RETURN;
END;$$ LANGUAGE plpgsql;


-- busca productos en base un patron con el formate de LIKE
-- Si variable de entrada con_stock > 0, Productos para la venta
-- Si no, son para visualizar en mercaderia
create or replace function buscar_producto (IN expresion varchar(255),
	  	  	   		    IN columnas varchar[],
					    IN usar_like boolean,
        				    IN con_stock boolean,
       					    OUT barcode int8,
					    OUT codigo_corto varchar(16),
					    OUT marca varchar(35),
					    OUT descripcion varchar(50),
					    OUT contenido varchar(10),
					    OUT unidad varchar(10),
					    OUT stock float8,
					    OUT precio float8,
					    OUT precio_neto float8,
					    OUT costo_promedio float8,
					    OUT vendidos float8,
					    OUT impuestos bool,
					    OUT otros int4,
					    OUT familia int2,
					    OUT perecibles bool,
					    OUT stock_min float8,
					    OUT dias_stock float8,
					    OUT margen_promedio float8,
					    OUT fraccion bool,
					    OUT canje bool,
					    OUT stock_pro float8,
					    OUT tasa_canje float8,
					    OUT precio_mayor float8,
					    OUT cantidad_mayor float8,
					    OUT mayorista bool,
					    OUT tipo_id int4,
					    OUT tipo_mer varchar(20))
returns setof record as $$
declare
	list record;
	query text;
	i integer;
	corriente int4;
	materia_prima int4;
begin
	corriente := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'CORRIENTE');
	materia_prima := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'MATERIA PRIMA');

	query := $S$ SELECT barcode, codigo_corto, marca, descripcion, contenido, unidad, stock, costo_promedio,
	      	     	    precio, precio_neto, vendidos, impuestos, otros, familia, perecibles,
			    (SELECT nombre FROM tipo_mercaderia WHERE id = tipo) AS tipo_mercaderia,
			    COALESCE ((dias_stock * select_ventas_dia(producto.barcode, TRUE)::float), 0) AS stock_min,
			    margen_promedio, fraccion, canje, stock_pro, dias_stock,
			    tasa_canje, precio_mayor, cantidad_mayor, mayorista, tipo FROM producto WHERE $S$;

        IF con_stock IS TRUE THEN
           query := query || $S$ (SELECT disponible FROM obtener_stock_desde_barcode (barcode)) > 0 and $S$;
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

        query := query || $S$) and estado=true order by descripcion, marca$S$;

	FOR list IN EXECUTE query LOOP
	    barcode := list.barcode;
	    codigo_corto := list.codigo_corto;
	    marca := list.marca;
	    descripcion := list.descripcion;
	    contenido := list.contenido;
	    unidad := list.unidad;

	    IF list.tipo != corriente AND list.tipo != materia_prima THEN
		stock := (SELECT disponible FROM obtener_stock_desde_barcode (barcode));
	    ELSE
		stock := list.stock;
            END IF;

	    precio := list.precio;
	    precio_neto := list.precio_neto;

	    IF list.tipo != corriente AND list.tipo != materia_prima THEN
		costo_promedio := (SELECT costo FROM obtener_costo_promedio_desde_barcode (barcode));
	    ELSE
		costo_promedio := list.costo_promedio;
	    END IF;

	    vendidos := list.vendidos;
	    impuestos := list.impuestos;
	    otros := list.otros;
	    familia := list.familia;
	    perecibles := list.perecibles;
	    stock_min := list.stock_min;
	    dias_stock := list.dias_stock;
	    margen_promedio := list.margen_promedio;
	    fraccion := list.fraccion;
	    canje := list.canje;
	    stock_pro := list.stock_pro;
	    tasa_canje := list.tasa_canje;
	    precio_mayor := list.precio_mayor;
	    cantidad_mayor := list.cantidad_mayor;
	    mayorista := list.mayorista;
	    tipo_id := list.tipo;
	    tipo_mer := list.tipo_mercaderia;
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
		      	    and compra.anulada='f'
			    and compra.anulada_pi='f'
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

--retorna el detalle de una compra
--
create or replace function get_detalle_compra (IN id_compra integer,
		                               OUT codigo_corto varchar(16),
					       OUT descripcion varchar(50),
					       OUT marca varchar(35),
					       OUT contenido varchar(10),
					       OUT unidad varchar(10),
					       OUT precio double precision,
					       OUT cantidad double precision,
					       OUT cantidad_ingresada double precision,
					       OUT costo_ingresado bigint,
					       OUT barcode bigint,
					       OUT precio_venta double precision,
					       OUT margen double precision)
RETURNS setof record AS $$
DECLARE
	list record;
	query text;
BEGIN
	query := $S$ SELECT t2.barcode, t2.codigo_corto, t2.descripcion, t2.marca, t2.contenido, t2.unidad,
	                    t1.precio, t1.cantidad, t1.cantidad_ingresada, t1.precio_venta, t1.margen,
			    (t1.precio * (t1.cantidad - t1.cantidad_ingresada))::bigint AS costo_ingresado
		     FROM compra_detalle AS t1,
		          producto AS t2
		     WHERE t1.id_compra = $S$||id_compra||$S$
		     AND t2.barcode = t1.barcode_product
		     AND t1.cantidad_ingresada < t1.cantidad
		     AND t1.anulado ='f' $S$;

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


--
-- retorna el porcentaje del iva de un producto (de no tener IVA retorna -1)
--
CREATE OR REPLACE FUNCTION get_iva (IN barcode_in bigint,
		  	   	    OUT valor double precision)
RETURNS double precision AS $$
BEGIN
		SELECT impuesto.monto INTO valor
		FROM producto, impuesto
		WHERE producto.barcode=barcode_in
		AND producto.impuestos='true'
		AND impuesto.id=1;

                IF valor IS NULL THEN
                   valor := 0;
                END if;

END; $$ LANGUAGE plpgsql;


--
-- retorna el porcentaje de sus otros impuestos (aparte del iva) del producto
--
CREATE OR REPLACE FUNCTION get_otro_impuesto (IN barcode_in bigint,
		  	   		      OUT valor double precision)
RETURNS double precision AS $$
BEGIN
		SELECT impuesto.monto INTO valor
		FROM producto, impuesto
		WHERE producto.barcode = barcode_in
		AND impuesto.id = producto.otros;

		IF valor IS NULL THEN
                   valor := 0;
                END if;

END; $$ LANGUAGE plpgsql;


--
-- Obtiene los impuestos totales en cifra monetaria (no porcentaje) de una mercadería compuesta
-- sumando los impuestos de sus componentes (para registrarlo en venta)
--
CREATE OR REPLACE FUNCTION get_impuestos_compuesto (IN in_barcode_product bigint,
       	  	  	   			    OUT iva_out double precision,
						    OUT otros_out double precision,
						    OUT ganancia_out double precision,
       	  	  	   			    OUT iva_percent_out double precision,
						    OUT otros_percent_out double precision,
						    OUT ganancia_percent_out double precision)
RETURNS setof record AS $$
DECLARE
	l record;
	query varchar;

	compuesta_l int4;

	-- Datos para la mercadería madre
	costo_madre double precision;
	precio_madre double precision;

	precio_gm double precision;
	costo_gm double precision;

	-- Datos para mercadería
	-- Precio proporcional
	precio_prop double precision;
BEGIN
	-- Id tipo de mercadería (materia prima)
	SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';

	-- Se inicializan algunas variables
	SELECT costo INTO costo_gm FROM obtener_costo_promedio_desde_barcode (in_barcode_product);
	SELECT precio INTO precio_gm FROM producto WHERE barcode = in_barcode_product;
	precio_madre := precio_gm;
	costo_madre := costo_gm;
	iva_out := 0;
	otros_out := 0;
	ganancia_out := 0;

	-- Consulta recursiva, obtiene todos los componentes y sub componentes de la mercadería compuesta o derivada
	query :=$S$ WITH RECURSIVE compuesta (barcode_complejo, tipo_complejo, barcode_componente, tipo_componente, cant_mud) AS
      	      	    (
		      SELECT barcode_complejo, tipo_complejo, barcode_componente, tipo_componente, cant_mud
       		      FROM componente_mc WHERE barcode_complejo = $S$ || in_barcode_product || $S$
        	      UNION ALL
        	      SELECT componente_mc.barcode_complejo, componente_mc.tipo_complejo,
		      	     componente_mc.barcode_componente, componente_mc.tipo_componente,
           	      	     componente_mc.cant_mud * compuesta.cant_mud
              	      FROM componente_mc, compuesta
       		      WHERE componente_mc.barcode_complejo = compuesta.barcode_componente
      		    )
      		    SELECT c.barcode_complejo, c.tipo_complejo, c.barcode_componente, c.tipo_componente, c.cant_mud,
		    	   (SELECT precio FROM producto WHERE barcode = c.barcode_componente) AS precio_componente,
			   (SELECT costo FROM obtener_costo_promedio_desde_barcode (c.barcode_componente)) AS costo_componente,
			   (SELECT valor FROM get_iva (c.barcode_componente))/100 AS iva,
			   (SELECT valor FROM get_otro_impuesto (c.barcode_componente))/100 AS otros
      		    FROM compuesta c $S$;

	FOR l IN EXECUTE query LOOP	-- NOTA: cuidado con las materias primas vendibles como madre
	    -- PRECIO PROPORCIONAL (proporcion del COSTO componente con respecto a madre) * PRECIO madre
	    precio_prop := (l.costo_componente/costo_madre) * precio_madre;

	    -- Si el componente actual es compuesto (sera la proxima madre)
	    IF (l.tipo_componente = compuesta_l OR l.tipo_componente = derivada_l) THEN
	       precio_madre := precio_prop;
	       SELECT costo INTO costo_madre FROM obtener_costo_promedio_desde_barcode (l.barcode_componente);
	    ELSE
	       -- GANANCIA ((PRECIO NETO -> precio de venta sin impuestos) - costo del producto) * cantidad_requerida
	       ganancia_out := ganancia_out + ((precio_prop / (l.iva + l.otros + 1)) - l.costo_componente) * l.cant_mud;
	       -- IVA ((PRECIO NETO -> precio de venta sin impuestos) * iva)  * cantidad_requerida
	       iva_out := iva_out + ((precio_prop / (l.iva + l.otros + 1)) * l.iva) * l.cant_mud;
	       -- OTROS ((PRECIO NETO -> precio de venta sin impuestos) * otros) * cantidad_requerida
	       otros_out := otros_out + ((precio_prop / (l.iva + l.otros + 1)) * l.otros) * l.cant_mud;
	    END IF;
	END LOOP;

	--                 ((iva madre) / (Precio neto madre)) * 100 = %IVA
	iva_percent_out := (iva_out / (precio_gm - (iva_out+otros_out))) * 100;
	--                 ((otros madre) / (Precio neto madre)) * 100 = %OTROS
	otros_percent_out := (otros_out / (precio_gm - (iva_out+otros_out))) * 100;
	--                 (ganancia_madre / costo_madre) * 100 = % ganancia
	ganancia_percent_out := (ganancia_out / costo_gm) * 100;

	-- retorna el total acumulado
	RETURN NEXT;
RETURN;
END; $$ LANGUAGE plpgsql;


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

-- retorna del rut del proveedor para una compra dada
-- revisar si se puede satisfacer con alguna funcion anteriormente definida
-- compras.c:4549, 7023
create or replace function get_proveedor_compra(
		IN id_compra integer,
		OUT nombre_out varchar(100),
		OUT rut_out integer,
		OUT dv_out varchar(1))
RETURNS SETOF RECORD AS $$
DECLARE
   l record;
   q varchar;
BEGIN

   -- Se obtienen los datos del proveedor a quien de le compro
   q := $S$ SELECT rut, dv, nombre
      	    FROM proveedor
   	    WHERE rut = (SELECT rut_proveedor FROM compra WHERE id = $S$||id_compra||$S$)$S$;

   -- Se supone que siempre devolvera una fila...
   FOR l IN EXECUTE q LOOP
       nombre_out := l.nombre;
       rut_out := l.rut;
       dv_out := l.dv;
       RETURN NEXT;
   END LOOP;

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
returns double precision as $$
declare
	resultado double precision;
begin

resultado := (select costo_promedio from producto where barcode=barcode_in);
return resultado;

end; $$ language plpgsql;


---
-- Se obtienen los detalles de cada venta
-- (agrupados por producto) para el ranking de ventas.
---
CREATE OR REPLACE FUNCTION ranking_ventas (IN starts date,
       	  	  	   		   IN ends date,
       					   OUT barcode varchar,
       					   OUT descripcion varchar,
       					   OUT marca varchar,
       					   OUT contenido varchar,
       					   OUT unidad varchar,
       					   OUT familia integer,
       					   OUT amount double precision,
       					   OUT sold_amount double precision,
       					   OUT costo double precision,
       					   OUT contrib double precision,
       					   OUT impuestos boolean)
RETURNS SETOF RECORD AS $$
DECLARE
  q text;
  l record;

  corriente_l int4;
  derivada_l int4;
BEGIN

  SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';
  SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';

  -- Se crea una tabla temporal con el detalle de la venta simple
  CREATE TEMPORARY TABLE venta_detalle_completa AS
  SELECT vd.barcode AS barcode_l,
  	 SUM (vd.cantidad) AS amount_l,
	 SUM (vd.cantidad * vd.precio) AS sold_amount_l, -- SubTotal
	 SUM (vd.cantidad * vd.fifo) AS costo_l,
	 SUM ((vd.precio * vd.cantidad) - ((vd.iva+vd.otros) + (vd.fifo * vd.cantidad))) AS contrib_l
	 --SUM (vd.ganancia) AS contrib -- habilitar cuando este la modificación de facturas funcione perfectamente
  FROM venta v
       INNER JOIN venta_detalle vd
       ON vd.id_venta = v.id
  WHERE (vd.tipo = corriente_l OR vd.tipo = derivada_l)
  	AND v.fecha>=quote_literal(starts)::timestamp AND v.fecha<quote_literal(ends)::timestamp
  	AND v.id NOT IN (SELECT id_sale FROM venta_anulada)
  GROUP BY vd.barcode;

  -- Incluye en la tabla temporal venta_detalle_completa el detalle de los componentes de una merc. compleja
  INSERT INTO venta_detalle_completa
  SELECT vmcd.barcode_componente AS barcode_l,
  	 SUM (vmcd.cantidad) AS amount_l,
	 SUM (vmcd.cantidad * vmcd.precio_proporcional) AS sold_amount_l, -- SubTotal
	 SUM (vmcd.cantidad * vmcd.costo_promedio) AS costo_l,
	 SUM ((vmcd.precio_proporcional * vmcd.cantidad) - ((vmcd.iva+vmcd.otros) + (vmcd.costo_promedio * vmcd.cantidad))) AS contrib_l
  FROM venta v
       INNER JOIN venta_mc_detalle vmcd
       ON vmcd.id_venta_vd = v.id
  WHERE (vmcd.tipo_componente = corriente_l OR vmcd.tipo_componente = derivada_l)
  	AND v.fecha>=quote_literal(starts)::timestamp AND v.fecha<quote_literal(ends)::timestamp
  	AND v.id NOT IN (SELECT id_sale FROM venta_anulada)
  GROUP BY vmcd.barcode_componente;

  q := $S$
     SELECT producto.barcode AS barcode,
     	    producto.descripcion AS descripcion,
       	    producto.marca AS marca,
	    producto.contenido AS contenido,
	    producto.unidad AS unidad,
	    producto.familia AS familia,
	    producto.impuestos AS impuestos,
	    SUM (vdc.amount_l) AS amount,
	    SUM (vdc.sold_amount_l) AS sold_amount,
	    SUM (vdc.costo_l) AS costo,
       	    SUM (vdc.contrib_l) AS contrib
      FROM venta_detalle_completa vdc
      	   INNER JOIN producto
	   ON vdc.barcode_l = producto.barcode
      GROUP BY vdc.barcode_l,1,2,3,4,5,6,7
      ORDER BY producto.descripcion ASC $S$;

  FOR l IN EXECUTE q LOOP
      barcode := l.barcode;
      descripcion := l.descripcion;
      marca := l.marca;
      contenido := l.contenido;
      unidad := l.unidad;
      familia := l.familia;
      amount := l.amount;
      sold_amount := l.sold_amount;
      costo := l.costo;
      contrib := l.contrib;
      impuestos := l.impuestos;
      RETURN NEXT;
  END LOOP;

  -- Se elimina la tabla temporal
  DROP TABLE venta_detalle_completa;

RETURN;
END; $$ language plpgsql;


--- TODOO : REVISAR
-- Ranking de ventas de las materias primas
-- (ventas indirectas).
---
CREATE OR REPLACE FUNCTION ranking_ventas_mp (IN starts date,
       	  	  	   		      IN ends date,
					      OUT barcode varchar,
					      OUT descripcion varchar,
					      OUT marca varchar,
					      OUT contenido varchar,
					      OUT unidad varchar,
					      OUT cantidad double precision,
					      OUT monto_vendido double precision,
					      OUT costo double precision,
					      OUT contribucion double precision,
					      OUT familia integer)
RETURNS SETOF RECORD AS $$
DECLARE
	q text;
	l record;
	materia_prima_l int4;
BEGIN
	SELECT id INTO materia_prima_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA';
	q := $S$ SELECT --CASE WHEN vmcd.tipo_componente = $S$||materia_prima_l||$S$ THEN vmcd.barcode_componente
	     	 	--     WHEN vmcd.tipo_complejo = $S$||materia_prima_l||$S$ THEN vmcd.barcode_complejo
			--END AS barcode,
			vmcd.barcode_componente AS barcode, --Ahora la materia prima es un componente base
	     	 	p.descripcion, p.marca, p.contenido, p.unidad, p.familia,
		        SUM (vmcd.cantidad) AS cantidad,
			SUM (vmcd.cantidad*vmcd.precio_proporcional) AS monto_vendido,
			SUM (vmcd.cantidad*vmcd.costo_promedio) AS costo,
       			SUM ((vmcd.precio_proporcional*vmcd.cantidad)-((vmcd.iva+vmcd.otros)+(vmcd.costo_promedio*vmcd.cantidad))) AS contribucion
		 FROM venta_mc_detalle vmcd
		      INNER JOIN venta v
		      	    ON v.id = vmcd.id_venta_vd
		      INNER JOIN producto p
		      	    ON p.barcode = vmcd.barcode_componente
			       		   --CASE WHEN vmcd.tipo_hijo = $S$||materia_prima_l||$S$ THEN vmcd.barcode_hijo
	     	 	       		   --	WHEN vmcd.tipo_madre = $S$||materia_prima_l||$S$ THEN vmcd.barcode_madre
					   --END
		 WHERE fecha>=$S$ || quote_literal(starts) || $S$ AND fecha< $S$ || quote_literal(ends) || $S$
		       AND vmcd.tipo_componente = $S$||materia_prima_l||$S$ -- OR vmcd.tipo_complejo = $S$||materia_prima_l||$S$
		       AND v.id NOT IN (SELECT id_sale FROM venta_anulada)
		 GROUP BY 1,2,3,4,5,6 ORDER BY p.descripcion ASC $S$;

      	FOR l IN EXECUTE q loop
	    barcode := l.barcode;
            descripcion := l.descripcion;
	    marca := l.marca;
    	    contenido := l.contenido;
    	    unidad := l.unidad;
	    familia := l.familia;
    	    cantidad := l.cantidad;
    	    monto_vendido := l.monto_vendido;
    	    costo := l.costo;
    	    contribucion := l.contribucion;
    	    RETURN NEXT;
        END LOOP;

RETURN;
END; $$ LANGUAGE plpgsql;

--
-- Ranking de ventas de los productos derivados
-- (ventas indirectas).
--
CREATE OR REPLACE FUNCTION ranking_ventas_deriv (IN starts date,
       	  	  	   		      	 IN ends date,
						 IN barcode_mp varchar,
					      	 OUT barcode varchar,
					      	 OUT descripcion varchar,
					      	 OUT marca varchar,
					      	 OUT contenido varchar,
					      	 OUT unidad varchar,
					      	 OUT cantidad double precision,
					      	 OUT monto_vendido double precision,
					      	 OUT costo double precision,
					      	 OUT contribucion double precision)
RETURNS SETOF RECORD AS $$
DECLARE
  q text;
  l record;
  --------------
  derivada_l int4;  -- id tipo derivado
  --------------
BEGIN
  SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';

  -- Se crea una tabla temporal con el detalle de la venta simple
  CREATE TEMPORARY TABLE venta_derivados_completa AS
  SELECT vd.barcode AS barcode_l,
  SUM (vd.cantidad) AS amount_l,
  SUM (vd.cantidad * vd.precio) AS sold_amount_l, -- SubTotal
  SUM (vd.cantidad * vd.fifo) AS costo_l,
  SUM ((vd.precio * vd.cantidad) - ((vd.iva+vd.otros) + (vd.fifo * vd.cantidad))) AS contrib_l
  --SUM (vd.ganancia) AS contrib -- TODO: habilitar cuando este la modificación de facturas funcione perfectamente
  FROM venta v
  INNER JOIN venta_detalle vd
  ON vd.id_venta = v.id
  WHERE vd.tipo = derivada_l
  AND v.fecha>=quote_literal(starts)::timestamp AND v.fecha<quote_literal(ends)::timestamp
  AND v.id NOT IN (SELECT id_sale FROM venta_anulada)
  AND vd.barcode IN (SELECT barcode_complejo FROM componente_mc WHERE barcode_componente=barcode_mp::bigint)
  GROUP BY vd.barcode;

  -- Incluye en la tabla temporal venta_derivados_completa el detalle de los componentes de una merc. compleja
  INSERT INTO venta_derivados_completa
  SELECT vmcd.barcode_complejo AS barcode_l,
  	 SUM (vmcd.cantidad) AS amount_l,
	 SUM (vmcd.cantidad * vmcd.precio) AS sold_amount_l, -- SubTotal
	 SUM (vmcd.cantidad * vmcd.costo_promedio) AS costo_l,
	 SUM ((vmcd.precio * vmcd.cantidad) - ((vmcd.iva+vmcd.otros) + (vmcd.costo_promedio * vmcd.cantidad))) AS contrib_l
  FROM venta v
       INNER JOIN venta_mc_detalle vmcd
       ON vmcd.id_venta_vd = v.id
  WHERE vmcd.tipo_componente = derivada_l
  	AND v.fecha>=quote_literal(starts)::timestamp AND v.fecha<quote_literal(ends)::timestamp
  	AND v.id NOT IN (SELECT id_sale FROM venta_anulada)
	AND vmcd.barcode_complejo IN (SELECT barcode_complejo FROM componente_mc WHERE barcode_componente = barcode_mp::bigint)
  GROUP BY vmcd.barcode_complejo;

  q := $S$
     SELECT producto.barcode AS barcode,
     	    producto.descripcion AS descripcion,
       	    producto.marca AS marca,
	    producto.contenido AS contenido,
	    producto.unidad AS unidad,
	    producto.familia AS familia,
	    producto.impuestos AS impuestos,
	    SUM (vdc.amount_l) AS cantidad,
	    SUM (vdc.sold_amount_l) AS monto_vendido,
	    SUM (vdc.costo_l) AS costo,
       	    SUM (vdc.contrib_l) AS contribucion
      FROM venta_derivados_completa vdc
      	   INNER JOIN producto
	   ON vdc.barcode_l = producto.barcode
      GROUP BY vdc.barcode_l,1,2,3,4,5,6,7
      ORDER BY producto.descripcion ASC $S$;

  FOR l IN EXECUTE q loop
      barcode := l.barcode;
      descripcion := l.descripcion;
      marca := l.marca;
      contenido := l.contenido;
      unidad := l.unidad;
      cantidad := l.cantidad;
      monto_vendido := l.monto_vendido;
      costo := l.costo;
      contribucion := l.contribucion;
      RETURN NEXT;
  END LOOP;

  -- Se elimina la tabla temporal
  DROP TABLE venta_derivados_completa;

RETURN;
END; $$ LANGUAGE plpgsql;


---
-- Ranking de ventas de los componentes de un producto compuesto
-- (ventas indirectas).
---
CREATE OR REPLACE FUNCTION ranking_ventas_comp (IN starts date,
       	  	  	   		      	IN ends date,
						IN barcode_mc varchar,
					      	OUT barcode varchar,
					      	OUT descripcion varchar,
					      	OUT marca varchar,
					      	OUT contenido varchar,
					      	OUT unidad varchar,
					      	OUT cantidad double precision,
					      	OUT monto_vendido double precision,
					      	OUT costo double precision,
					      	OUT contribucion double precision)
RETURNS SETOF RECORD AS $$
DECLARE
	q text;
	l record;
	-------------
	derivada_l int4;  -- id tipo derivado
	corriente_l int4; -- id tipo corriente
        materia_prima_l int4; -- id tipo materia prima
	-------------
BEGIN
	SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';
	SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';
        SELECT id INTO materia_prima_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA';

	q := $S$
	     WITH RECURSIVE compuesta (id_venta, barcode_complejo, id_mh, tipo_complejo,
	     	  	    	       barcode_componente, tipo_componente, cantidad,
				       precio, costo_promedio, iva, otros, ganancia) AS
      	      	(
		   SELECT id_venta_vd, barcode_complejo, id_mh, tipo_complejo,
			  barcode_componente, tipo_componente, cantidad,
			  precio_proporcional, costo_promedio, iva, otros, ganancia
       		   FROM venta_mc_detalle
			INNER JOIN venta v
			ON venta_mc_detalle.id_venta_vd = v.id
		   WHERE barcode_complejo = $S$ || barcode_mc || $S$
			 AND fecha>=$S$ || quote_literal(starts) || $S$ AND fecha< $S$ || quote_literal(ends) || $S$
        	   UNION ALL
        	   SELECT vmcd.id_venta_vd, vmcd.barcode_complejo, vmcd.id_mh, vmcd.tipo_complejo,
		      	  vmcd.barcode_componente, vmcd.tipo_componente, vmcd.cantidad,
			  vmcd.precio_proporcional, vmcd.costo_promedio, vmcd.iva, vmcd.otros, vmcd.ganancia
              	   FROM venta_mc_detalle vmcd, compuesta
       		   WHERE vmcd.barcode_complejo = compuesta.barcode_componente AND
			 vmcd.id_venta_vd = compuesta.id_venta AND
			 vmcd.id_mh[1] = compuesta.id_mh[2]
      		)
		SELECT producto.barcode AS barcode,
     	    	       producto.descripcion AS descripcion,
       	    	       producto.marca AS marca,
	    	       producto.contenido AS contenido,
	    	       producto.unidad AS unidad,
	    	       producto.familia AS familia,
	    	       producto.impuestos AS impuestos,
	    	       SUM (c.cantidad) AS cantidad,
	    	       SUM (c.precio*c.cantidad) AS monto_vendido,
	    	       SUM (c.costo_promedio*c.cantidad) AS costo,
       	    	       SUM (c.ganancia) AS contribucion
      	        FROM compuesta c
      	   	     INNER JOIN producto
	   	     	   ON c.barcode_componente = producto.barcode
	             WHERE c.tipo_componente = $S$ ||derivada_l|| $S$
		     	   OR c.tipo_componente = $S$ ||corriente_l|| $S$
                           OR c.tipo_componente = $S$ ||materia_prima_l|| $S$
      		     GROUP BY c.barcode_componente,1,2,3,4,5,6,7
      		     ORDER BY producto.descripcion ASC $S$;

      	FOR l IN EXECUTE q loop
	    barcode := l.barcode;
            descripcion := l.descripcion;
	    marca := l.marca;
    	    contenido := l.contenido;
    	    unidad := l.unidad;
    	    cantidad := l.cantidad;
    	    monto_vendido := l.monto_vendido;
    	    costo := l.costo;
    	    contribucion := l.contribucion;
    	    RETURN NEXT;
        END LOOP;

RETURN;
END; $$ LANGUAGE plpgsql;


---
-- Ranking de ventas de las mercaderías compuestas
---
CREATE OR REPLACE FUNCTION ranking_ventas_mc (IN starts date,
       	  	  	   		      IN ends date,
					      OUT barcode varchar,
					      OUT descripcion varchar,
					      OUT marca varchar,
					      OUT contenido varchar,
					      OUT unidad varchar,
					      OUT cantidad double precision,
					      OUT monto_vendido double precision,
					      OUT costo double precision,
					      OUT contribucion double precision,
					      OUT familia integer)
RETURNS SETOF RECORD AS $$
DECLARE
	q text;
	l record;
	compuesta_l int4;
BEGIN
	SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
	q := $S$ SELECT p.barcode, p.descripcion, p.marca, p.contenido, p.unidad, p.familia,
		        SUM (vd.cantidad) AS cantidad,
			SUM ((vd.cantidad*vd.precio)-(v.descuento*((vd.cantidad*vd.precio)/(v.monto+v.descuento)))) AS monto_vendido,
			SUM (vd.cantidad*vd.fifo) AS costo,
       			SUM ((vd.precio*vd.cantidad)-((vd.iva+vd.otros)+(vd.fifo*vd.cantidad))) AS contribucion
		 FROM venta_detalle vd
		 INNER JOIN venta v
		 ON v.id = vd.id_venta
		 INNER JOIN producto p
		 ON p.barcode = vd.barcode
		 WHERE fecha>=$S$ || quote_literal(starts) || $S$ AND fecha<$S$ || quote_literal(ends) || $S$
		 AND vd.tipo=$S$ || compuesta_l || $S$
		 GROUP BY 1,2,3,4,5,6 ORDER BY p.descripcion ASC $S$;

      	FOR l IN EXECUTE q loop
	    barcode := l.barcode;
            descripcion := l.descripcion;
	    marca := l.marca;
    	    contenido := l.contenido;
    	    unidad := l.unidad;
	    familia := l.familia;
    	    cantidad := l.cantidad;
    	    monto_vendido := l.monto_vendido;
    	    costo := l.costo;
    	    contribucion := l.contribucion;
    	    RETURN NEXT;
        END LOOP;
RETURN;
END; $$ LANGUAGE plpgsql;


-- ---------------------------- RANKING TRASPASOS ----------

---
-- Se obtienen los detalles de cada traspaso
-- (agrupados por producto) para el ranking de traspasos.
--
-- NOTA: traspaso_envio = TRUE (enviados); ELSE (recibidos)
-- TODOO:: revisar, se deben mostrar las materias primas????
---
CREATE OR REPLACE FUNCTION ranking_traspaso (IN starts date,
       	  	  	   		     IN ends date,
					     IN traspaso_envio boolean,
       					     OUT barcode varchar,
       					     OUT descripcion varchar,
       					     OUT marca varchar,
       					     OUT contenido varchar,
       					     OUT unidad varchar,
       					     OUT familia integer,
       					     OUT amount double precision,
       					     OUT costo double precision)
RETURNS SETOF RECORD AS $$
DECLARE
  q text;
  l record;
  --------------
  corriente_l int4;
  derivada_l int4;
  --------------
  filtro text;
  --------------
BEGIN

  SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';
  SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';

  -- Condición para obtener la información de las mercaderías enviadas y/o recibidas
  IF traspaso_envio = TRUE THEN
     filtro := 'tdc.origen_l = 1';
  ELSE
     filtro := 'tdc.origen_l != 1';
  END IF;

  -- Se crea una tabla temporal con el detalle de la venta simple
  CREATE TEMPORARY TABLE traspaso_detalle_completa AS
  SELECT td.barcode AS barcode_l,
  	 t.origen AS origen_l,
  	 SUM (td.cantidad) AS amount_l,
	 SUM (td.cantidad * td.precio) AS costo_l -- SUM (Subtotal costo) = COSTO TOTAL
  FROM traspaso t
       INNER JOIN traspaso_detalle td
       ON td.id_traspaso = t.id
  WHERE (td.tipo = corriente_l OR td.tipo = derivada_l)
  	AND t.fecha>=quote_literal(starts)::timestamp AND t.fecha<quote_literal(ends)::timestamp
  GROUP BY td.barcode, t.origen;

  -- Incluye en la tabla temporal venta_detalle_completa el detalle de los componentes de una merc. compleja
  INSERT INTO traspaso_detalle_completa
  SELECT tmcd.barcode_componente AS barcode_l,
  	 t.origen AS origen_l,
  	 SUM (tmcd.cantidad) AS amount_l,
	 SUM (tmcd.cantidad * tmcd.costo_promedio) AS costo_l -- SUM (Subtotal costo) = COSTO TOTAL
  FROM traspaso t
       INNER JOIN traspaso_mc_detalle tmcd
       ON tmcd.id_traspaso = t.id
  WHERE (tmcd.tipo_componente = corriente_l OR tmcd.tipo_componente = derivada_l) --Incluir las materias primas??
  	AND t.fecha>=quote_literal(starts)::timestamp AND t.fecha<quote_literal(ends)::timestamp
  GROUP BY tmcd.barcode_componente, t.origen;

  q := $S$
     SELECT producto.barcode AS barcode,
     	    producto.descripcion AS descripcion,
       	    producto.marca AS marca,
	    producto.contenido AS contenido,
	    producto.unidad AS unidad,
	    producto.familia AS familia,
	    producto.impuestos AS impuestos,
	    SUM (tdc.amount_l) AS amount,
	    SUM (tdc.costo_l) AS costo
      FROM traspaso_detalle_completa tdc
      	   INNER JOIN producto
	   ON tdc.barcode_l = producto.barcode
      WHERE $S$||filtro||$S$
      GROUP BY tdc.barcode_l,1,2,3,4,5,6,7
      ORDER BY producto.descripcion ASC $S$;

  FOR l IN EXECUTE q LOOP
      barcode := l.barcode;
      descripcion := l.descripcion;
      marca := l.marca;
      contenido := l.contenido;
      unidad := l.unidad;
      familia := l.familia;
      amount := l.amount;
      costo := l.costo;
      RETURN NEXT;
  END LOOP;

  -- Se elimina la tabla temporal
  DROP TABLE traspaso_detalle_completa;

RETURN;
END; $$ language plpgsql;


---
-- Ranking de traspasos de las materias primas
-- (traspasos indirectos).
--
-- NOTA: traspaso_envio = TRUE (enviados); ELSE (recibidos)
---
CREATE OR REPLACE FUNCTION ranking_traspaso_mp (IN starts date,
       	  	  	   		        IN ends date,
					        IN traspaso_envio boolean,
					        OUT barcode varchar,
					        OUT descripcion varchar,
					        OUT marca varchar,
					        OUT contenido varchar,
					        OUT unidad varchar,
					        OUT cantidad double precision,
					        OUT costo double precision,
					        OUT familia integer)
RETURNS SETOF RECORD AS $$
DECLARE
	q text;
	l record;
	--------
	materia_prima_l int4;
	--------
	filtro text;
	--------
BEGIN
	SELECT id INTO materia_prima_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA';

	-- Condición para obtener la información de las mercaderías enviadas y/o recibidas
	IF traspaso_envio = TRUE THEN
	   filtro := 't.origen = 1';
	ELSE
	   filtro := 't.origen != 1';
	END IF;

	q := $S$ SELECT --CASE WHEN tmcd.tipo_hijo = $S$||materia_prima_l||$S$ THEN tmcd.barcode_hijo
	     	 	--     WHEN tmcd.tipo_madre = $S$||materia_prima_l||$S$ THEN tmcd.barcode_madre
			--END AS barcode,
			tmcd.barcode_componente AS barcode
	     	 	t.origen, p.descripcion, p.marca, p.contenido, p.unidad, p.familia,
		        SUM (tmcd.cantidad) AS cantidad,
			SUM (tmcd.cantidad*tmcd.costo_promedio) AS costo
		 FROM traspaso_mc_detalle tmcd
		      INNER JOIN traspaso t
		      	    ON t.id = tmcd.id_traspaso
		      INNER JOIN producto p
		      	    ON p.barcode = tmcd.barcode_componente
			       		   --CASE WHEN tmcd.tipo_hijo = $S$||materia_prima_l||$S$ THEN tmcd.barcode_hijo
	     	 	       		   --	WHEN tmcd.tipo_madre = $S$||materia_prima_l||$S$ THEN tmcd.barcode_madre
					   --END
		 WHERE fecha>=$S$ || quote_literal(starts) || $S$ AND fecha< $S$ || quote_literal(ends) || $S$
		       --AND (tmcd.tipo_hijo = $S$||materia_prima_l||$S$ OR tmcd.tipo_madre = $S$||materia_prima_l||$S$)
		       AND tmcd.tipo_componente = $S$||materia_prima_l||$S$
		       AND $S$ ||filtro|| $S$
		 GROUP BY 1,2,3,4,5,6,7 ORDER BY p.descripcion ASC $S$;

      	FOR l IN EXECUTE q loop
	    barcode := l.barcode;
            descripcion := l.descripcion;
	    marca := l.marca;
    	    contenido := l.contenido;
    	    unidad := l.unidad;
	    familia := l.familia;
    	    cantidad := l.cantidad;
    	    costo := l.costo;
    	    RETURN NEXT;
        END LOOP;
RETURN;
END; $$ LANGUAGE plpgsql;


---
-- Ranking de traspaso de los productos derivados
-- (traspasos indirectos).
---
CREATE OR REPLACE FUNCTION ranking_traspaso_deriv (IN starts date,
       	  	  	   		      	   IN ends date,
						   IN barcode_mp varchar,
					      	   OUT barcode varchar,
					      	   OUT descripcion varchar,
					      	   OUT marca varchar,
					      	   OUT contenido varchar,
					      	   OUT unidad varchar,
					      	   OUT cantidad double precision,
					      	   OUT costo double precision)
RETURNS SETOF RECORD AS $$
DECLARE
  q text;
  l record;
  --------------
  derivada_l int4;  -- id tipo derivado
  --------------
BEGIN
  SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';

  -- Se crea una tabla temporal con el detalle del traspaso simple
  CREATE TEMPORARY TABLE traspaso_derivados_completa AS
  SELECT td.barcode AS barcode_l,
  	 SUM (td.cantidad) AS amount_l,
  	 SUM (td.cantidad * td.precio) AS costo_l
  FROM traspaso t
       INNER JOIN traspaso_detalle td
       ON td.id_traspaso = t.id
  WHERE td.tipo = derivada_l
  	AND t.fecha>=quote_literal(starts)::timestamp AND t.fecha<quote_literal(ends)::timestamp
  	AND td.barcode IN (SELECT barcode_complejo FROM componente_mc WHERE barcode_componente=barcode_mp::bigint)
  GROUP BY td.barcode;

  -- Incluye en la tabla temporal traspaso_derivados_completa el detalle de los componentes de una merc. compleja
  INSERT INTO traspaso_derivados_completa
  SELECT tmcd.barcode_componente AS barcode_l,
  	 SUM (tmcd.cantidad) AS amount_l,
	 SUM (tmcd.cantidad * tmcd.costo_promedio) AS costo_l
  FROM traspaso t
       INNER JOIN traspaso_mc_detalle tmcd
       ON tmcd.id_traspaso = t.id
  WHERE tmcd.tipo_componente = derivada_l
  	AND t.fecha>=quote_literal(starts)::timestamp AND t.fecha<quote_literal(ends)::timestamp
	AND tmcd.barcode_complejo IN (SELECT barcode_complejo FROM componente_mc WHERE barcode_componente = barcode_mp::bigint)
  GROUP BY tmcd.barcode_complejo;

  q := $S$
     SELECT producto.barcode AS barcode,
     	    producto.descripcion AS descripcion,
       	    producto.marca AS marca,
	    producto.contenido AS contenido,
	    producto.unidad AS unidad,
	    producto.familia AS familia,
	    producto.impuestos AS impuestos,
	    SUM (tdc.amount_l) AS cantidad,
	    SUM (tdc.costo_l) AS costo
      FROM traspaso_derivados_completa tdc
      	   INNER JOIN producto
	   ON tdc.barcode_l = producto.barcode
      GROUP BY tdc.barcode_l,1,2,3,4,5,6,7
      ORDER BY producto.descripcion ASC $S$;

  FOR l IN EXECUTE q loop
      barcode := l.barcode;
      descripcion := l.descripcion;
      marca := l.marca;
      contenido := l.contenido;
      unidad := l.unidad;
      cantidad := l.cantidad;
      costo := l.costo;
      RETURN NEXT;
  END LOOP;

  -- Se elimina la tabla temporal
  DROP TABLE traspaso_derivados_completa;

RETURN;
END; $$ LANGUAGE plpgsql;


---
-- Ranking de traspaso de los componentes de un producto compuesto
-- (traspasos indirectos).
---
CREATE OR REPLACE FUNCTION ranking_traspaso_comp (IN starts date,
       	  	  	   		      	  IN ends date,
						  IN barcode_mc varchar,
					      	  OUT barcode varchar,
					      	  OUT descripcion varchar,
					      	  OUT marca varchar,
					      	  OUT contenido varchar,
					      	  OUT unidad varchar,
					      	  OUT cantidad double precision,
					      	  OUT costo double precision)
RETURNS SETOF RECORD AS $$
DECLARE
	q text;
	l record;
	-------------
	derivada_l int4;  -- id tipo derivado
	corriente_l int4; -- id tipo corriente
	-------------
BEGIN
	SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';
	SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';

	q := $S$
	     WITH RECURSIVE compuesta (id_traspaso, barcode_complejo, id_mh, tipo_complejo,
	     	  	    	       barcode_componente, tipo_componente, cantidad, costo_promedio) AS
      	      	(
		   SELECT id_traspaso, barcode_complejo, id_mh, tipo_complejo,
			  barcode_componente, tipo_componente, cantidad, costo_promedio
       		   FROM traspaso_mc_detalle
			INNER JOIN traspaso t
			ON traspaso_mc_detalle.id_traspaso = t.id
		   WHERE barcode_complejo = $S$ || barcode_mc || $S$
			 AND fecha>=$S$ || quote_literal(starts) || $S$ AND fecha< $S$ || quote_literal(ends) || $S$
        	   UNION ALL
        	   SELECT tmcd.id_traspaso, tmcd.barcode_complejo, tmcd.id_mh, tmcd.tipo_complejo,
		      	  tmcd.barcode_componente, tmcd.tipo_componente, tmcd.cantidad, tmcd.costo_promedio
              	   FROM traspaso_mc_detalle tmcd, compuesta
       		   WHERE tmcd.barcode_complejo = compuesta.barcode_componente AND
			 tmcd.id_traspaso = compuesta.id_traspaso AND
			 tmcd.id_mh[1] = compuesta.id_mh[2]
      		)
		SELECT producto.barcode AS barcode,
     	    	       producto.descripcion AS descripcion,
       	    	       producto.marca AS marca,
	    	       producto.contenido AS contenido,
	    	       producto.unidad AS unidad,
	    	       producto.familia AS familia,
	    	       producto.impuestos AS impuestos,
	    	       SUM (c.cantidad) AS cantidad,
	    	       SUM (c.costo_promedio*c.cantidad) AS costo
      	        FROM compuesta c
      	   	     INNER JOIN producto
	   	     	   ON c.barcode_componente = producto.barcode
	             WHERE c.tipo_complejo = $S$ ||derivada_l|| $S$
		     	   OR c.tipo_componente = $S$ ||corriente_l|| $S$
      		     GROUP BY c.barcode_componente,1,2,3,4,5,6,7
      		     ORDER BY producto.descripcion ASC $S$;

      	FOR l IN EXECUTE q loop
	    barcode := l.barcode;
            descripcion := l.descripcion;
	    marca := l.marca;
    	    contenido := l.contenido;
    	    unidad := l.unidad;
    	    cantidad := l.cantidad;
    	    costo := l.costo;
    	    RETURN NEXT;
        END LOOP;
RETURN;
END; $$ LANGUAGE plpgsql;


---
-- Ranking de traspaso de las mercaderías compuestas
--
-- NOTA: traspaso_envio = TRUE (enviados); ELSE (recibidos)
---
CREATE OR REPLACE FUNCTION ranking_traspaso_mc (IN starts date,
       	  	  	   		        IN ends date,
						IN traspaso_envio boolean,
					        OUT barcode varchar,
					      	OUT descripcion varchar,
					      	OUT marca varchar,
					      	OUT contenido varchar,
					      	OUT unidad varchar,
					      	OUT cantidad double precision,
					      	OUT costo double precision,
					      	OUT familia integer)
RETURNS SETOF RECORD AS $$
DECLARE
	q text;
	l record;
	-------------
	compuesta_l int4;
	-------------
	filtro text;
	-------------
BEGIN
	SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';

	-- Condición para obtener la información de las mercaderías enviadas y/o recibidas
	IF traspaso_envio = TRUE THEN
	   filtro := 't.origen = 1';
	ELSE
	   filtro := 't.origen != 1';
	END IF;

	q := $S$ SELECT p.barcode, t.origen, p.descripcion, p.marca, p.contenido, p.unidad, p.familia,
		        SUM (td.cantidad) AS cantidad,
			SUM (td.cantidad*td.precio) AS costo
		 FROM traspaso_detalle td
		 INNER JOIN traspaso t
		 ON t.id = td.id_traspaso
		 INNER JOIN producto p
		 ON p.barcode = td.barcode
		 WHERE fecha>=$S$ || quote_literal(starts) || $S$ AND fecha<$S$ || quote_literal(ends) || $S$
		 AND td.tipo=$S$ || compuesta_l || $S$
		 AND $S$||filtro|| $S$
		 GROUP BY 1,2,3,4,5,6,7 ORDER BY p.descripcion ASC $S$;

      	FOR l IN EXECUTE q loop
	    barcode := l.barcode;
            descripcion := l.descripcion;
	    marca := l.marca;
    	    contenido := l.contenido;
    	    unidad := l.unidad;
	    familia := l.familia;
    	    cantidad := l.cantidad;
    	    costo := l.costo;
    	    RETURN NEXT;
        END LOOP;
RETURN;
END; $$ LANGUAGE plpgsql;

-- ---------------------------- END RANKING TRASPASOS ------

--
--
--
create or replace function get_guide_detail(
		IN id_guia integer,
                IN rut_proveedor integer,
		OUT codigo_corto integer,
		OUT descripcion varchar(50),
		OUT marca varchar(35),
		OUT contenido varchar(10),
		OUT unidad varchar(10),
		OUT precio double precision,
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
		OUT codigo_corto varchar(16),
		OUT descripcion varchar(50),
		OUT marca varchar(35),
		OUT contenido varchar(10),
		OUT unidad varchar(10),
		OUT precio double precision,
		OUT cantidad double precision,
		OUT precio_compra double precision,
		OUT barcode bigint)
RETURNS SETOF record AS $$
DECLARE
        list record;
        query text;
BEGIN
        query := $S$ SELECT t2.codigo_corto, t2.descripcion, t2.marca, t2.contenido, t2.unidad, t2.precio as precio_venta,
	      	     	    t1.precio as precio_compra, t1.cantidad, t2.barcode
		     FROM factura_compra_detalle AS t1, producto AS t2, factura_compra as t3
		     WHERE t1.barcode=t2.barcode and t1.id_factura_compra=t3.id and t3.id=$S$|| id_invoice;

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



CREATE OR REPLACE FUNCTION insert_egreso (
       IN in_monto float8,
       IN in_tipo int,
       IN in_usuario int)
RETURNS int AS $$
DECLARE
	current_caja int;
BEGIN

SELECT last_value INTO current_caja FROM caja_id_seq;

IF (SELECT fecha_termino FROM caja WHERE id=current_caja) IS NOT NULL then
   raise notice 'Adding an egreso to caja that is closed (%)', current_caja;
END IF;

INSERT INTO egreso (monto, tipo, fecha, usuario, id_caja)
       VALUES (in_monto, in_tipo, now(), in_usuario, current_caja);

RETURN 0;
END; $$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION insert_ingreso (
       IN in_monto int,
       IN in_tipo int,
       IN in_usuario int)
RETURNS int AS $$
DECLARE
	current_caja int;
BEGIN

SELECT MAX(id) INTO current_caja FROM caja;

IF (SELECT fecha_termino FROM caja WHERE id=current_caja) IS NOT NULL then
   RAISE NOTICE 'Adding an ingreso to caja that is closed (%)', current_caja;
END IF;

INSERT INTO ingreso (monto, tipo, fecha, usuario, id_caja)
       VALUES (in_monto, in_tipo, now(), in_usuario, current_caja);

RETURN 0;
END; $$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION is_caja_abierta ()
RETURNS boolean AS $$
DECLARE
	current_caja int;
	fecha_ter timestamp;
BEGIN

	SELECT MAX(id) INTO current_caja FROM caja;

	IF current_caja IS NULL then
	   return FALSE;
	END IF;

	SELECT fecha_termino INTO fecha_ter FROM caja WHERE id=current_caja;

	IF (fecha_ter IS NULL) THEN
	   return TRUE;
	ELSE
	   return FALSE;
	END IF;

END; $$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION trg_insert_caja()
RETURNS TRIGGER AS $$
DECLARE
	last_sale bigint;
BEGIN
	SELECT LAST_VALUE INTO last_sale FROM venta_id_seq;
	new.id_venta_inicio = last_sale;
RETURN NEW;
END; $$ LANGUAGE plpgsql;


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
        open_date timestamp without time zone;
	close_date timestamp without time zone;
	close_date_back timestamp without time zone;
	cash_payed_money bigint;
	monto1 bigint;
	monto2 bigint;
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

if id_inicio = 1 then
   select sum(monto) into arqueo
          from venta
          where id > 0 and id <= id_termino
          and tipo_documento = 0
          and tipo_venta = 0;
else
   select sum(monto) into arqueo
          from venta
          where id > id_inicio and id <= id_termino
          and tipo_documento = 0
          and tipo_venta = 0;
end if;


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

select fecha_inicio into open_date
       from caja where id=last_caja;

close_date := now();

select fecha_termino into close_date_back
       from caja where id = last_caja - 1;

select sum (monto_abonado) into monto1
       from abono where fecha_abono > open_date and fecha_abono < close_date;

if monto1 is null then
   monto1 := 0;
end if;

select sum (monto_abonado) into monto2
       from abono where fecha_abono > close_date_back and  fecha_abono < open_date;

if monto2 is null then
   monto2 := 0;
end if;

cash_payed_money := monto1 + monto2;

return (monto_apertura + arqueo + cash_payed_money + ingresos - egresos);
end; $$ language plpgsql;


CREATE OR REPLACE FUNCTION informacion_producto_venta(IN in_codigo varchar(16),
						      OUT codigo_corto varchar(16),
                				      OUT barcode bigint,
						      OUT descripcion varchar(50),
						      OUT marca varchar(35),
						      OUT contenido varchar(10),
						      OUT unidad varchar(10),
						      OUT stock double precision,
						      OUT precio double precision,
						      OUT precio_mayor double precision,
						      OUT cantidad_mayor double precision,
						      OUT mayorista boolean,
						      OUT stock_day double precision,
						      OUT estado boolean,
						      OUT tipo integer)
RETURNS SETOF record AS $$
declare
	days double precision;
	datos record;
	query varchar;
	codigo varchar;
	corriente int4;
	materia_prima int4;
	codigo_only_numeric boolean;
BEGIN

SELECT TRIM (in_codigo) INTO codigo;

-- Comprueba si es un valor numerico
SELECT CASE
	WHEN LENGTH(SUBSTRING(codigo FROM '[0-9]+')) != LENGTH(codigo) THEN FALSE
	ELSE TRUE
       END INTO codigo_only_numeric;

-- Id tipos de producto
corriente := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'CORRIENTE');
materia_prima := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'MATERIA PRIMA');

query := $S$ SELECT *,
		    (stock::float / select_ventas_dia(producto.barcode, TRUE)::float) AS stock_day
		FROM producto WHERE $S$;

-- check if must use the barcode or the short code
IF codigo_only_numeric = TRUE THEN
   query := query||$S$ barcode=$S$||codigo||$S$ OR codigo_corto LIKE $S$||quote_literal(codigo);
ELSE
   query := query||$S$ codigo_corto=$S$||quote_literal(codigo);
END IF;

FOR datos IN EXECUTE query LOOP
    codigo_corto := datos.codigo_corto;
    barcode := datos.barcode;
    descripcion := datos.descripcion;
    marca := datos.marca;
    contenido := datos.contenido;
    unidad := datos.unidad;
        -- Su la mercadería es derivada, calcula su stock de acuerdo a sus componentes
    IF datos.tipo != corriente AND datos.tipo != materia_prima THEN
	stock := (SELECT * FROM obtener_stock_desde_barcode (datos.barcode));
    ELSE
	stock := datos.stock;
    END IF;

    precio := datos.precio;
    stock_day := datos.stock_day;
    mayorista := datos.mayorista;
    precio_mayor := datos.precio_mayor;
    cantidad_mayor := datos.cantidad_mayor;
    estado := datos.estado;
    tipo := datos.tipo;
    RETURN NEXT;
END LOOP;

RETURN;
END;
$$ LANGUAGE plpgsql;

----
-- nullify sale
create or replace function nullify_sale (
       in salesman_id int,
       in sale_id int,
       in con_egreso boolean)
returns boolean as $$
declare
	----------------------
        query text;
	l record;
	----------------------
        sale_amount float8;
        id_tipo_egreso integer;
	----------------------
	tipo_pago integer;
	forma_pago1 integer;
	forma_pago2 integer;
	monto_pago1 integer;
	monto_pago2 integer;
	----------------------
begin
	-- Tipo de pagos:
	-----------------
	-- EFECTIVO           = 0
	-- CREDITO            = 1
	-- CHEQUE RESTAURANT  = 2
	-- MIXTO              = 3
	-- CHEQUE             = 4 --SIN USO
	-- TARJETA            = 5 --SIN USO

        SELECT monto INTO sale_amount FROM venta WHERE id=sale_id;
        SELECT id INTO id_tipo_egreso FROM tipo_egreso WHERE descrip='Nulidad de Venta';
	SELECT tipo_venta INTO tipo_pago FROM venta WHERE id=sale_id;

	-- Si el pago es mixto se obtienen las formas de pago
	IF (tipo_pago = 3) THEN
	   SELECT tipo_pago1 INTO forma_pago1 FROM pago_mixto WHERE id_sale=sale_id;
	   SELECT tipo_pago2 INTO forma_pago2 FROM pago_mixto WHERE id_sale=sale_id;
	   SELECT monto1 INTO monto_pago1 FROM pago_mixto WHERE id_sale=sale_id;
	   SELECT monto2 INTO monto_pago2 FROM pago_mixto WHERE id_sale=sale_id;
	END IF;

	-- Si es la anulacion de una venta a credito o con cheque de restaurant o mixto sin pago efectivo no debe egresar dinero de caja
	-- Debe  considerar esa venta como "pagada", de esa forma de "restituye" el credito del cliente (correspondiente a esa venta)
	IF (tipo_pago = 1 OR tipo_pago = 2 OR (tipo_pago = 3 AND (forma_pago1 != 0 AND forma_pago2 != 0))) THEN
       	   UPDATE deuda SET pagada='t' WHERE id_venta=sale_id;
	ELSE
	   IF (tipo_pago = 3) THEN --Si el pago es con cheque de restaurant se egresa solo el monto efectivo
	      IF (forma_pago1 = 0) THEN -- Si el pago 1 fue en efectivo
	      	 sale_amount = monto_pago1;
	      ELSE -- Si el pago 2 fue en efectivo
	         sale_amount = monto_pago2;
	      END IF;
	   END IF;

	   IF con_egreso = TRUE THEN -- Solo si 'con_egreso' = true, se saca dinero de caja
	      perform insert_egreso (sale_amount, id_tipo_egreso, salesman_id);
	   END IF;
	END IF;

        insert into venta_anulada(id_sale, vendedor)
               values (sale_id, salesman_id);

	-- se llama a get_sale_detail con TRUE para que devuelva solo los datos de los componentes
        query := $S$ select * from get_sale_detail ($S$||sale_id||$S$, TRUE)$S$;

        for l in execute query loop
	    EXECUTE $S$UPDATE producto SET stock = stock+$S$||l.amount||$S$ WHERE barcode = $S$||l.barcode;
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
        out cash_on_mixed_sell integer,
        out cash_outcome integer,
	out nullify_sell integer,
	out current_expenses integer,
        out cash_income integer,
        out cash_payed_money integer,
	out cash_loss_money integer,
	out bottle_return integer,
	out bottle_deposit integer,
	out cash_close_outcome integer
	)
returns setof record as $$
declare
        query varchar;
        dat record;
        sell_first_id integer;
        sell_last_id integer;
	perdida_egreso integer;
        first_cash_box_id integer;
        last_cash_box_id integer;
	open_date2 timestamp without time zone;
	close_date_now timestamp without time zone;
	close_date_back timestamp without time zone;
	monto1 bigint;
	monto2 bigint;
begin

        select id_venta_inicio, fecha_inicio, inicio, id_venta_termino, fecha_termino, termino, COALESCE(perdida,0)
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

	-- ACLARACION: Si el id de apertura y cierre son iguales significa que no hubo venta
	-- La que la siguiente operación debería realizarse solo si hay venta en la primera apertura de caja

	-- To avoid problems with the first sell
        if sell_first_id = 1 AND sell_first_id != sell_last_id then
                sell_first_id := 0;
        end if;

	-- sell_first_id es el último id de venta antes de la apertura de caja
        -- Ve las ventas directas en efectivo y el efectivo de las ventas mixtas
        if sell_last_id = 0 or sell_last_id is null then
                --Cash sell
                select sum (monto) into cash_sells
                from venta
                where id > sell_first_id and tipo_venta = 0;
                --Cash on mixed sell
                select sum (pm.monto2) into cash_on_mixed_sell
                from pago_mixto pm
                where pm.id_sale > sell_first_id and pm.tipo_pago2 = 0;
        else
                --Cash sell
                select sum (monto) into cash_sells
                from venta
                where id > sell_first_id and id <= sell_last_id and tipo_venta = 0;
                --Cash on mixed sell
                select sum (pm.monto2) into cash_on_mixed_sell
                from pago_mixto pm
                where pm.id_sale > sell_first_id and pm.id_sale <= sell_last_id and pm.tipo_pago2 = 0;
        end if;

        if cash_sells is null then
                cash_sells := 0;
        end if;

        select sum (monto) into cash_outcome
        from egreso e
	inner join tipo_egreso te
	on e.tipo = te.id
        where e.id_caja = cash_box_id
	and te.descrip = 'Retiro';

        if cash_outcome is null then
                cash_outcome := 0;
        end if;

	select sum (monto) into cash_close_outcome
        from egreso e
	inner join tipo_egreso te
	on e.tipo = te.id
        where e.id_caja = cash_box_id
	and te.descrip = 'Retiro por cierre';

        if cash_close_outcome is null then
                cash_close_outcome := 0;
        end if;


	select sum (monto) into nullify_sell
        from egreso e
	inner join tipo_egreso te
	on e.tipo = te.id
        where e.id_caja = cash_box_id
	and te.descrip = 'Nulidad de Venta';

        if nullify_sell is null then
                nullify_sell := 0;
        end if;

	select sum (monto) into current_expenses
        from egreso e
	inner join tipo_egreso te
	on e.tipo = te.id
        where e.id_caja = cash_box_id
	and te.descrip = 'Gastos Corrientes';

        if current_expenses is null then
                current_expenses := 0;
        end if;

	select sum (monto) into perdida_egreso
        from egreso e
	inner join tipo_egreso te
	on e.tipo = te.id
        where e.id_caja = cash_box_id
	and te.descrip = 'Perdida';

        if perdida_egreso is null then
                perdida_egreso := 0;
        end if;

	select sum (monto) into bottle_return
        from egreso e
	inner join tipo_egreso te
	on e.tipo = te.id
        where e.id_caja = cash_box_id
	and te.descrip = 'Devolucion envases';

        if bottle_return is null then
                bottle_return := 0;
        end if;

        select sum (monto) into cash_income
        from ingreso, tipo_ingreso
        where id_caja = cash_box_id
	and tipo = tipo_ingreso.id
	and tipo_ingreso.descrip != 'Deposito envases';

        if cash_income is null then
                cash_income := 0;
        end if;

	select sum (monto) into bottle_deposit
        from ingreso i
	inner join tipo_ingreso ti
	on i.tipo = ti.id
        where i.id_caja = cash_box_id
	and ti.descrip = 'Deposito envases';

        if bottle_deposit is null then
                bottle_deposit := 0;
        end if;

        select last_value into last_cash_box_id from caja_id_seq;

        select fecha_inicio into open_date2
               from caja where id = last_cash_box_id;

        close_date_now := now();

        select fecha_termino into close_date_back
               from caja where id = last_cash_box_id - 1;

        select sum (monto_abonado) into monto1
        from abono where fecha_abono > open_date and fecha_abono < close_date;

        if monto1 is null then
           monto1 := 0;
        end if;

        select sum (monto_abonado) into monto2
               from abono where fecha_abono > close_date_back and  fecha_abono < open_date;

        if monto2 is null then
           monto2 := 0;
        end if;

        cash_payed_money := monto1 + monto2;
	cash_loss_money := cash_loss_money + perdida_egreso;

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

---
-- Entrega el detalle de la venta (venta_detalle y venta_mc_detalle)
--
-- Si solo componentes = TRUE devuelve solamente materias primas y corrientes
-- (buscando el detalle de los compuestos y derivados vendidos)
--
-- Si es FALSE devuelve los datos de la venta tal cual como fué consedida
---
CREATE OR REPLACE FUNCTION get_sale_detail (IN sale_id int,
       	  	  	   		    IN solo_componentes boolean,
          	  	   		    OUT barcode bigint,
					    OUT tipo int,
        				    OUT price double precision,
        				    OUT amount double precision)
RETURNS SETOF record AS $$
DECLARE
	-----------------
        l1 record;
	l2 record;
        q1 varchar (255);
	q2 varchar;
	-----------------
	corriente_l int4;     -- id tipo corriente
	compuesta_l int4;     -- id compuesta
	materia_prima_l int4; -- id tipo materia prima
	derivada_l int4;      -- id tipo derivada
BEGIN
	SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';
	SELECT id INTO materia_prima_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA';
	SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';
	SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';

	q1 := $S$SELECT barcode, precio, cantidad, tipo FROM venta_detalle WHERE id_venta=$S$||sale_id;

      	FOR l1 IN EXECUTE q1 LOOP
            barcode := l1.barcode;
	    tipo := l1.tipo;
            price := l1.precio;
            amount := l1.cantidad;

	    -- Si es una mercadería compuesta, se retornan sus componentes (si solo_componentes = TRUE)
	    IF ((l1.tipo = compuesta_l OR l1.tipo = derivada_l) AND solo_componentes = TRUE) THEN
	       q2 := $S$
	       WITH RECURSIVE compuesta (id_venta, barcode_complejo, id_mh, tipo_complejo,
	     	  	    	     	 barcode_componente, tipo_componente, cantidad, precio) AS
      	       (
	         SELECT id_venta_vd, barcode_complejo, id_mh, tipo_complejo,
		      	barcode_componente, tipo_componente, cantidad, precio_proporcional
       	       	 FROM venta_mc_detalle
	       	 WHERE barcode_complejo = $S$||l1.barcode||$S$
		       AND venta_mc_detalle.id_venta_vd = $S$||sale_id||$S$
                 UNION ALL
               	 SELECT vmcd.id_venta_vd, vmcd.barcode_complejo, vmcd.id_mh, vmcd.tipo_complejo,
	      	      	vmcd.barcode_componente, vmcd.tipo_componente, vmcd.cantidad, precio_proporcional
                 FROM venta_mc_detalle vmcd, compuesta
       	       	 WHERE vmcd.barcode_complejo = compuesta.barcode_componente AND
		       vmcd.id_venta_vd = compuesta.id_venta AND
		       vmcd.id_mh[1] = compuesta.id_mh[2]
	      )
	      SELECT barcode_componente AS barcode, tipo_componente AS tipo, cantidad, precio
	      FROM compuesta
	      WHERE tipo_componente = $S$||corriente_l||$S$
	      OR tipo_componente = $S$||materia_prima_l;

	      FOR l2 IN EXECUTE q2 LOOP
	      	  barcode := l2.barcode;
	    	  tipo := l2.tipo;
            	  price := l2.precio;
            	  amount := l2.cantidad;
	      	  RETURN NEXT;
	      END LOOP;
	    ELSE -- Si es una mercadería corriente o materia prima (o cualquiera, en caso que solo_componentes = FALSE)
	      RETURN NEXT;
	    END IF;
	END LOOP;

RETURN;
END; $$ LANGUAGE plpgsql;



create or replace function calculate_fifo (
        in product_barcode bigint,
        in compra_id int)
returns double precision as $$
declare
        current_fifo double precision;
        current_stock double precision;
        costo double precision;
        stock_add double precision;
        suma double precision;
        fifo double precision;
begin

        select costo_promedio, stock into current_fifo, current_stock from producto where barcode=product_barcode;
        select precio, cantidad_ingresada into costo, stock_add from compra_detalle where barcode_product=product_barcode and id_compra=compra_id;

        if current_fifo is null then
            current_fifo = 0;
        end if;

        suma = current_stock * current_fifo;
        suma = suma + (stock_add * costo);

        current_stock = current_stock + stock_add;

        fifo = (suma / current_stock);

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
	--Tipo Venta = 0 (es en efectivo), tipo_venta debería llamarse "tipo_pago"
        select COALESCE (SUM (descuento), 0) into total_cash_discount from venta where fecha >= starts and fecha < ends+'1 days' and descuento!=0
                and tipo_venta=0 and venta.id not in (select id_sale from venta_anulada);

        select COALESCE (count(*), 0) into total_discount from venta where fecha >= starts and fecha < ends+'1 days' and descuento!=0
                and tipo_venta=0 and venta.id not in (select id_sale from venta_anulada);
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
       in in_precio_compra double precision)
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


---
-- Registra la merma, merma detalle (a través de otra función) y ajusta el stock
---
CREATE OR REPLACE FUNCTION registrar_merma (IN barcode_in bigint,
					    IN unidades_in double precision, --cantidad
					    IN motivo_in int4)
RETURNS void AS $$
DECLARE
  ----------------------
  id_merma_l int4;
  ----------------------
  tipo_l int4;
  ----------------------
  derivada_l int4;
  compuesta_l int4;
  corriente_l int4;
  materia_prima_l int4;
  ----------------------
  cantidad_mp double precision;
  barcode_mp bigint;
  ----------------------
BEGIN
	-- Se obtie el id de los tipos de productos
	SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';
	SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
	SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';
	SELECT id INTO materia_prima_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA';
	SELECT tipo INTO tipo_l FROM producto WHERE barcode = barcode_in;

	-- Se registra la merma
	INSERT INTO merma (id, barcode, tipo, unidades, motivo, fecha)
	VALUES (DEFAULT, barcode_in, tipo_l, unidades_in, motivo_in, NOW())
	RETURNING id INTO id_merma_l;

	-- Se registra el detalle de la merma de un compuesto
	IF (tipo_l = compuesta_l OR tipo_l = derivada_l) THEN
	   -- Se llama a la función que registra los componentes de esta merma
	   -- NOTA: Ella también se encarga de disminuir el stock de sus componentes
	   PERFORM registrar_merma_compuesto (id_merma_l, barcode_in, unidades_in);

	ELSIF (tipo_l = corriente_l OR tipo_l = materia_prima_l) THEN
	  -- Se disminuye su stock en la cantidad especificada
	  PERFORM disminuir_stock_desde_barcode (barcode_in, unidades_in);
	END IF;
RETURN;
END; $$ language plpgsql;


---
-- Registra el detalle del traspaso de un compuesto
---
CREATE OR REPLACE FUNCTION registrar_merma_compuesto (IN id_merma_in int4,
       	  	  	   			      IN barcode_madre_in bigint,
						      IN cantidad_in double precision)
RETURNS void AS $$

DECLARE
	-- Consulta inicial
	q text;
	l record;
	--------------------------
	-- Subconsulta (recursiva)
	q2 text;
	l2 record;
	--------------------------
	compuesta_l int4; -- id tipo compuesto
	derivada_l int4;  -- id tipo derivado
	corriente_l int4; -- id tipo corriente
	materia_prima_l int4; -- id tipo materia prima
	--------------------------
	-- Mercaderia hija temporal (materia prima)
	barcode_mp bigint;
	cantidad_mp double precision;
	costo_mp double precision;
	--------------------------
BEGIN
	SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';
	SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
	SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';
	SELECT id INTO materia_prima_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA';

	-- Creo una secuencia temporal para crear un id único para cada producto
	CREATE SEQUENCE id_mh_seq START 1;
	-- Se realiza la consulta recursiva y el resultado se almacena en una tabla temporal
	CREATE TEMPORARY TABLE arbol_componentes AS
	WITH RECURSIVE compuesta (barcode_complejo, id_mh, tipo_complejo, barcode_componente, tipo_componente, cant_mud) AS
      	 (
	   SELECT barcode_complejo,
	   	  ARRAY[0, nextval('id_mh_seq')] AS id_mh,
		  tipo_complejo, barcode_componente, tipo_componente, cant_mud
       	   FROM componente_mc WHERE barcode_complejo = barcode_madre_in
           UNION ALL
           SELECT componente_mc.barcode_complejo,
	   	  ARRAY[id_mh[2], nextval('id_mh_seq')],
		  componente_mc.tipo_complejo,
	      	  componente_mc.barcode_componente, componente_mc.tipo_componente,
              	  componente_mc.cant_mud * compuesta.cant_mud
           FROM componente_mc, compuesta
       	   WHERE componente_mc.barcode_complejo = compuesta.barcode_componente
      	)
	SELECT c.barcode_complejo, c.id_mh, c.tipo_complejo, c.barcode_componente, c.tipo_componente, c.cant_mud
      	FROM compuesta c
	ORDER BY id_mh[2] ASC;

	q := $S$ SELECT * FROM arbol_componentes$S$;

	-- Se registra el detalle de compuesto (y subcompuestos) y se actualiza su stock
	FOR l IN EXECUTE q LOOP
       	    INSERT INTO merma_mc_detalle (id, id_merma, id_mh, barcode_complejo, barcode_componente,
	    	   		          tipo_complejo, tipo_componente, cantidad)
	    VALUES (DEFAULT, id_merma_in, l.id_mh, l.barcode_complejo, l.barcode_componente,
	    	    l.tipo_complejo, l.tipo_componente, l.cant_mud * cantidad_in);

	    IF (l.tipo_componente = corriente_l OR l.tipo_componente = derivada_l) THEN
	      -- Se actuaiza el stock de la mercadería corriente
      	      UPDATE producto SET stock=stock-(l.cant_mud * cantidad_in) WHERE barcode=l.barcode_componente;
	    END IF;
	END LOOP;

        -- Se elimina la tabla temporal arbol_componentes
   	DROP TABLE arbol_componentes;
	-- Se elimina la secuencia temporal
	DROP SEQUENCE id_mh_seq;
RETURN;
END; $$ LANGUAGE plpgsql;


---
-- Registra el traspaso de mercaderías
---
create or replace function registrar_traspaso (IN monto double precision,
       	  	  	   		       IN origen integer,
  					       IN destino integer,
  					       IN vendedor integer,
  					       OUT inserted_id integer)
RETURNS integer as $$
BEGIN
	EXECUTE $S$ INSERT INTO traspaso( id, monto, fecha, origen, destino, vendedor)
	VALUES ( DEFAULT, $S$|| monto ||$S$, NOW(),$S$|| origen ||$S$,$S$|| destino ||$S$,$S$|| vendedor ||$S$) $S$;

	SELECT currval( 'traspaso_id_seq' ) INTO inserted_id;

return;
END; $$ language plpgsql;


---
-- Registra el detalle del traspaso de mercaderías
-- y actualiza el stock aumentandolo o disminuyéndolo según el caso
---
CREATE OR REPLACE FUNCTION registrar_traspaso_detalle (
       IN in_id_traspaso int,
       IN in_barcode bigint,
       IN in_cantidad double precision,
       IN in_costo double precision,
       IN in_precio double precision,
       IN in_traspaso_envio boolean, -- si es TRUE es envío; FALSO recibe
       IN in_modificar_costo boolean) -- si se le modifica el costo al producto
RETURNS void AS $$
DECLARE
   aux int;
   num_linea int;
   ----
   compuesta_l int4;
   derivada_l int4;
   materia_prima_l int4;
   corriente_l int4;
   ----
   tipo_l int4;
   ----
   barcode_mp bigint;
   costo_mp double precision;
   cantidad_mp double precision;
   ----
BEGIN
	SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
	SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';
	SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';
	SELECT id INTO materia_prima_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA';

	SELECT tipo INTO tipo_l FROM producto WHERE barcode = in_barcode;

	-- Se modifica el costo en caso de ser necesario
	IF in_modificar_costo = TRUE AND (tipo_l = corriente_l OR tipo_l = materia_prima_l) THEN
	   UPDATE producto
	   	  SET costo_promedio = in_costo,
		      precio = in_precio
           WHERE barcode = in_barcode
	   AND (costo_promedio = 0 OR costo_promedio IS NULL);
	END IF;


	aux := (SELECT COUNT(*) FROM traspaso_detalle WHERE id_traspaso = in_id_traspaso);

	IF aux = 0 THEN
	   num_linea := 0;
	ELSE
	   num_linea := (SELECT MAX(id) FROM traspaso_detalle WHERE id_traspaso = in_id_traspaso);
	   num_linea := num_linea + 1;
	END IF;

	INSERT INTO traspaso_detalle (id,
				      id_traspaso,
				      barcode,
				      cantidad,
				      precio, --costo del producto
				      precio_venta,
				      tipo,
				      costo_modificado)
	       VALUES (num_linea,
		       in_id_traspaso,
		       in_barcode,
		       in_cantidad,
		       in_costo,
		       in_precio,
		       tipo_l,
		       in_modificar_costo);

       	-- Si es una mercadería compuesta, se registrará todo su detalle en traspaso_mc_detalle
	IF (tipo_l = compuesta_l OR tipo_l = derivada_l) THEN -- NOTA:registrar_traspaso_compuesto actualiza el stock de sus componentes por si solo
   	   -- (envia o recibe, aumenta o disminuye según el tipo de traspaso)
	   PERFORM registrar_traspaso_compuesto (in_id_traspaso::int4, num_linea::int4, in_barcode::bigint,
			                       	 in_cantidad::double precision, in_traspaso_envio);
	-- Si es mercadería corriente
	ELSIF (tipo_l = corriente_l OR tipo_l = materia_prima_l) THEN
	   IF (in_traspaso_envio = TRUE) THEN -- se disminuye el stock porque se envía
	      PERFORM disminuir_stock_desde_barcode (in_barcode, in_cantidad);
	   ELSE -- se aumenta el stock porque se recibe
	      PERFORM aumentar_stock_desde_barcode (in_barcode, in_cantidad);
	   END IF;
	END IF;

END;$$ LANGUAGE plpgsql;


---
-- Registra el detalle del traspaso de un compuesto
---
CREATE OR REPLACE FUNCTION registrar_traspaso_compuesto (IN id_traspaso_in int4,
       	  	  	   				 IN id_traspaso_detalle_in int4,
       	  	  	   				 IN barcode_madre_in bigint,
							 IN cantidad_in double precision,
							 IN traspaso_envio_in boolean) -- cantidad madre
RETURNS void AS $$

DECLARE
	-- Consulta inicial
	q text;
	l record;
	--------------------------
	-- Subconsulta (recursiva)
	q2 text;
	l2 record;
	--------------------------
	compuesta_l int4; -- id tipo compuesto
	derivada_l int4;  -- id tipo derivado
	corriente_l int4; -- id tipo corriente
	materia_prima_l int4; -- id tipo materia prima
	--------------------------
	-- Mercaderia hija temporal (materia prima)
	barcode_mp bigint;
	cantidad_mp double precision;
	costo_mp double precision;
	--------------------------
BEGIN
	SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';
	SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
	SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';
	SELECT id INTO materia_prima_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA';

	-- Creo una secuencia temporal para crear un id único para cada producto
	CREATE SEQUENCE id_mh_seq START 1;
	-- Se realiza la consulta recursiva y el resultado se almacena en una tabla temporal
	CREATE TEMPORARY TABLE arbol_componentes AS
	WITH RECURSIVE compuesta (barcode_complejo, id_mh, tipo_complejo, barcode_componente, tipo_componente, cant_mud) AS
      	 (
	   SELECT barcode_madre,
	   	  ARRAY[0, nextval('id_mh_seq')] AS id_mh,
		  tipo_madre, barcode_componente, tipo_componente, cant_mud
       	   FROM componente_mc WHERE barcode_complejo = barcode_madre_in
           UNION ALL
           SELECT componente_mc.barcode_complejo,
	   	  ARRAY[id_mh[2], nextval('id_mh_seq')],
		  componente_mc.tipo_complejo,
	      	  componente_mc.barcode_componente, componente_mc.tipo_componente,
              	  componente_mc.cant_mud * compuesta.cant_mud
           FROM componente_mc, compuesta
       	   WHERE componente_mc.barcode_complejo = compuesta.barcode_componente
      	 )
	SELECT c.barcode_complejo, c.id_mh, c.tipo_complejo, c.barcode_componente, c.tipo_componente, c.cant_mud,
	       (SELECT costo FROM obtener_costo_promedio_desde_barcode (c.barcode_componente)) AS costo_componente
      	FROM compuesta c
	ORDER BY id_mh[2] ASC;

	q := $S$ SELECT * FROM arbol_componentes$S$;

	-- Se registra el detalle de compuesto (y subcompuestos) y se actualiza su stock
	FOR l IN EXECUTE q LOOP
       	    INSERT INTO traspaso_mc_detalle (id, id_traspaso, id_traspaso_detalle, id_mh, barcode_complejo, barcode_componente,
	    	   			     tipo_complejo, tipo_componente, cantidad, costo_promedio)
	    VALUES (DEFAULT, id_traspaso_in, id_traspaso_detalle_in, l.id_mh, l.barcode_complejo, l.barcode_componente,
	    	    l.tipo_complejo, l.tipo_componente, l.cant_mud * cantidad_in, l.costo_componente);

	    -- Si es una mercadería corriente se actualiza su stock
	    IF (l.tipo_componente = corriente_l OR l.tipo_componente = materia_prima_l) THEN
	      -- Se actuaiza el stock
	      IF (traspaso_envio_in = TRUE) THEN -- se disminuye el stock porque se envía
	      	 UPDATE producto SET stock=stock-(l.cant_mud * cantidad_in) WHERE barcode=l.barcode_componente;
	      ELSE -- se aumenta el stock porque se recibe
	      	 UPDATE producto SET stock=stock+(l.cant_mud * cantidad_in) WHERE barcode=l.barcode_componente;
	      END IF;
	    END IF;

	END LOOP;

        -- Se elimina la tabla temporal arbol_componentes
   	DROP TABLE arbol_componentes;
	-- Se elimina la secuencia temporal
	DROP SEQUENCE id_mh_seq;
RETURN;
END; $$ LANGUAGE plpgsql;



-- Retorna como resultado la suma de todas las operaciones hechas sobre un
-- producto dentro del día especificado
CREATE OR REPLACE FUNCTION movimiento_en_fecha (IN fecha_inicio timestamp,
       	  	  	   		        IN barcode_in bigint,
						OUT barcode_out varchar,
       						OUT movimiento_out double precision,
						OUT costo_promedio_en_fecha_out double precision)
RETURNS SETOF record AS $$
DECLARE
	q text;
	l record;
	corriente int4;
	materia_prima int4;
BEGIN

SELECT id INTO corriente FROM tipo_mercaderia WHERE upper(nombre) LIKE 'CORRIENTE';
SELECT id INTO materia_prima FROM tipo_mercaderia WHERE upper(nombre) LIKE 'MATERIA PRIMA';

--SELECT SUM (COALESCE(cantidad_ingresada,0) - COALESCE(cantidad_c_anuladas,0) - COALESCE(cantidad_vendida,0) - COALESCE(cantidad_vmcd,0) - COALESCE(unidades_merma,0) + COALESCE(cantidad_anulada,0) - COALESCE(cantidad_devolucion,0) - COALESCE(cantidad_envio,0) + COALESCE(cantidad_recibida,0)) AS movimiento
q := $S$ SELECT p.barcode, cantidad_ingresada, cantidad_c_anuladas, cantidad_vendida, cantidad_vmcd, unidades_merma, cantidad_anulada, cantidad_devolucion, cantidad_envio, cantidad_recibida,
     	 	(SELECT fcd.costo_promedio
			FROM factura_compra_detalle fcd
			INNER JOIN factura_compra fc
			ON fcd.id_factura_compra = fc.id

			WHERE fc.fecha < $S$ ||quote_literal (fecha_inicio+'1 days')|| $S$
			AND barcode = p.barcode
			ORDER BY fcd.id_factura_compra DESC
			LIMIT 1) AS costo_fecha
       	 	FROM producto p

	 	-- Las compras ingresadas hechas hasta la fecha determinada
	 	LEFT JOIN (SELECT SUM(fcd.cantidad) AS cantidad_ingresada, fcd.barcode AS barcode
		          	  FROM factura_compra_detalle fcd
       		     	          INNER JOIN factura_compra fc
       		     	          ON fc.id = fcd.id_factura_compra

       		     	          WHERE fc.fecha >= $S$ || quote_literal (fecha_inicio) || $S$ AND fc.fecha < $S$ || quote_literal (fecha_inicio+'1 days') || $S$
                     	          AND fcd.cantidad > 0
                     	          GROUP BY barcode) AS cantidad_ingresada
                ON p.barcode = cantidad_ingresada.barcode

	        -- Las anulaciones de compras hechas hasta la fecha determinada
		LEFT JOIN (SELECT SUM(cad.cantidad_anulada) AS cantidad_c_anuladas, cad.barcode AS barcode
		       	          FROM compra_anulada ca
				  INNER JOIN compra_anulada_detalle cad
				  ON ca.id = cad.id_compra_anulada

				  WHERE ca.fecha_anulacion >= $S$ || quote_literal (fecha_inicio) || $S$ AND ca.fecha_anulacion < $S$ || quote_literal (fecha_inicio+'1 days') || $S$
				  GROUP BY barcode) AS compras_anuladas
	        ON p.barcode = compras_anuladas.barcode

       		-- Las Ventas hechas hasta la fecha determinada
       		LEFT JOIN (SELECT SUM(vd.cantidad) AS cantidad_vendida, vd.barcode AS barcode -- LEFT JOIN MUESTRA TODOS LOS PRODUCTOS
       	     	                  FROM venta v
		 	      	  INNER JOIN venta_detalle vd
			  	  ON v.id = vd.id_venta

			  	  WHERE v.fecha >= $S$ || quote_literal (fecha_inicio) || $S$ AND v.fecha < $S$ || quote_literal (fecha_inicio+'1 days') || $S$
			  	  GROUP BY barcode) AS ventas
		ON p.barcode = ventas.barcode

       	        -- Las anulaciones de venta hechas hasta la fecha determinada
       		LEFT JOIN (SELECT SUM(vd.cantidad) AS cantidad_anulada, vd.barcode AS barcode -- LEFT JOIN MUESTRA TODOS LOS PRODUCTOS
       	     	                  FROM venta v
		 	          INNER JOIN venta_detalle vd
			  	  ON v.id = vd.id_venta

			  	  INNER JOIN venta_anulada va
			    	  ON va.id_sale = v.id

				  WHERE va.fecha >= $S$ || quote_literal (fecha_inicio) || $S$ AND va.fecha < $S$ || quote_literal (fecha_inicio+'1 days') || $S$
			          GROUP BY barcode) AS ventas_anuladas
                ON p.barcode = ventas_anuladas.barcode

		-- Las Ventas (de compuestos) menos sus anulaciones hechas hasta la fecha determinada
       		LEFT JOIN (SELECT SUM(vmcd.cantidad) AS cantidad_vmcd, vmcd.barcode_componente AS barcode -- LEFT JOIN MUESTRA TODOS LOS PRODUCTOS
       	     	                  FROM venta_mc_detalle vmcd
				  INNER JOIN venta v ON vmcd.id_venta_vd = v.id

			  	  WHERE vmcd.id_venta_vd NOT IN (SELECT id_sale FROM venta_anulada)
				  AND v.fecha >= $S$ || quote_literal (fecha_inicio) || $S$ AND v.fecha < $S$ || quote_literal (fecha_inicio+'1 days') || $S$
			  	  GROUP BY barcode_componente) AS ventas_mcd
		ON p.barcode = ventas_mcd.barcode

	        -- Las Mermas sufridas hasta la fecha determinada
		LEFT JOIN (SELECT barcode, SUM(unidades) AS unidades_merma
       	     	                  FROM merma m

		         	  WHERE m.fecha >= $S$ || quote_literal (fecha_inicio) || $S$ AND m.fecha < $S$ || quote_literal (fecha_inicio+'1 days') || $S$
			          GROUP BY barcode) AS merma
	        ON p.barcode = merma.barcode

		-- Las Mermas sufridas hasta la fecha determinada
		LEFT JOIN (SELECT mmcd.barcode_componente AS barcode, SUM(mmcd.cantidad) AS unidades_merma_mcd
       	     	                  FROM merma m
				  INNER JOIN merma_mc_detalle mmcd
				  ON m.id = mmcd.id_merma

		         	  WHERE m.fecha >= $S$ || quote_literal (fecha_inicio) || $S$ AND m.fecha < $S$ || quote_literal (fecha_inicio+'1 days') || $S$
			          GROUP BY barcode_componente) AS merma_mcd
	        ON p.barcode = merma_mcd.barcode

                -- Las devoluciones hechas hasta la fecha determinada
       		LEFT JOIN (SELECT dd.barcode AS barcode, SUM(dd.cantidad) AS cantidad_devolucion
       	     	                  FROM devolucion d
		 	          INNER JOIN devolucion_detalle dd
			 	  ON d.id = dd.id_devolucion

			          WHERE d.fecha >= $S$ || quote_literal (fecha_inicio) || $S$ AND d.fecha < $S$ || quote_literal (fecha_inicio+'1 days') || $S$
			          GROUP BY barcode) AS devolucion
                ON p.barcode = devolucion.barcode

	        -- Los traspasos enviados hasta la fecha determinada
       		LEFT JOIN (SELECT td.barcode AS barcode, SUM(td.cantidad) AS cantidad_envio
       	     	                  FROM traspaso t
		 	          INNER JOIN traspaso_detalle td
			 	  ON t.id = td.id_traspaso

			          WHERE t.fecha >= $S$ || quote_literal (fecha_inicio) || $S$ AND t.fecha < $S$ || quote_literal (fecha_inicio+'1 days') || $S$
				  AND t.origen = 1
			          GROUP BY barcode) AS traspaso_envio
                ON p.barcode = traspaso_envio.barcode

		-- Los traspasos enviados (a traves de un compuesto) hasta la fecha determinada
		LEFT JOIN (SELECT tmcd.barcode_componente AS barcode, SUM(tmcd.cantidad) AS cantidad_mc_envio
       	     	                  FROM traspaso t
		 	          INNER JOIN traspaso_mc_detalle tmcd
			 	  ON t.id = tmcd.id_traspaso

			          WHERE t.fecha >= $S$ || quote_literal (fecha_inicio) || $S$ AND t.fecha < $S$ || quote_literal (fecha_inicio+'1 days') || $S$
				  AND t.origen = 1
			          GROUP BY barcode_componente) AS traspaso_mc_envio
                ON p.barcode = traspaso_mc_envio.barcode

	        -- Los traspasos recibidos hasta la fecha determinada
       		LEFT JOIN (SELECT td.barcode AS barcode, SUM(td.cantidad) AS cantidad_recibida
       	     	                  FROM traspaso t
		 	          INNER JOIN traspaso_detalle td
			 	  ON t.id = td.id_traspaso

			          WHERE t.fecha >= $S$ || quote_literal (fecha_inicio) || $S$ AND t.fecha < $S$ || quote_literal (fecha_inicio+'1 days') || $S$
				  AND t.origen != 1
			          GROUP BY barcode) AS traspaso_recibido
                ON p.barcode = traspaso_recibido.barcode

		-- Los traspasos recibidos (a traves de un compuesto) hasta la fecha determinada
       		LEFT JOIN (SELECT tmcd.barcode_componente AS barcode, SUM(tmcd.cantidad) AS cantidad_mc_recibida
       	     	                  FROM traspaso t
		 	          INNER JOIN traspaso_mc_detalle tmcd
			 	  ON t.id = tmcd.id_traspaso

			          WHERE t.fecha >= $S$ || quote_literal (fecha_inicio) || $S$ AND t.fecha < $S$ || quote_literal (fecha_inicio+'1 days') || $S$
				  AND t.origen != 1
			          GROUP BY barcode_componente) AS traspaso_mc_recibido
                ON p.barcode = traspaso_mc_recibido.barcode

                WHERE p.estado = true
		AND (p.tipo = $S$ || corriente || $S$
		     OR p.tipo = $S$ || materia_prima || $S$ ) $S$ ;

IF barcode_in != 0 THEN
    q := q || $S$ AND p.barcode = $S$ || barcode_in;
END IF;

FOR l IN EXECUTE q LOOP
    barcode_out := l.barcode;
    movimiento_out := COALESCE(l.cantidad_ingresada,0) - COALESCE(l.cantidad_c_anuladas,0) - COALESCE(l.cantidad_vendida,0) - COALESCE(l.cantidad_vmcd,0) - COALESCE(l.unidades_merma,0) + COALESCE(l.cantidad_anulada,0) - COALESCE(l.cantidad_devolucion,0) - COALESCE(l.cantidad_envio,0) + COALESCE(l.cantidad_recibida,0);
    costo_promedio_en_fecha_out := COALESCE(l.costo_fecha,0);
    RETURN NEXT;
END LOOP;

RETURN;
END; $$ LANGUAGE plpgsql;



---
-- Obtiene las unidades y valorizado de ventas de un producto determinado o
-- de todos los productos
---
CREATE OR REPLACE FUNCTION ventas_en_fecha (IN fecha_in timestamp,
				            IN barcode_in bigint,
					    OUT barcode_out bigint,
					    OUT cantidad_vendida_out double precision,
       					    OUT costo_vendido_out double precision,
					    OUT precio_vendido_out double precision,
					    OUT ganancia_vendido_out double precision)
RETURNS SETOF record AS $$
DECLARE
	q text;
	l record;
BEGIN
	-- TODO: agregar tb venta_mc_detalle
	q := $S$ SELECT barcode, SUM (cantidad) AS cantidad, SUM (fifo*cantidad) AS sub_costo_venta, SUM (precio*cantidad) AS sub_venta, SUM (ganancia*cantidad) AS sub_ganancia
	         FROM venta_detalle vd
	     	 INNER JOIN venta v
	     	 ON vd.id_venta = v.id
	     	 WHERE v.fecha >= $S$ ||quote_literal (fecha_in)|| $S$ AND v.fecha < $S$ ||quote_literal (fecha_in+'1 days');

        IF barcode_in != 0 THEN
    	   q := q || $S$ AND barcode = $S$ || barcode_in;
	END IF;

	q := q || $S$ GROUP BY barcode $S$;

	EXECUTE q INTO l;

	FOR l IN EXECUTE q LOOP
	    cantidad_vendida_out := COALESCE (l.cantidad,0);
	    costo_vendido_out := COALESCE (l.sub_costo_venta);
	    precio_vendido_out := COALESCE (l.sub_venta);
	    ganancia_vendido_out := COALESCE (l.sub_ganancia);
	    barcode_out := l.barcode;
	    RETURN NEXT;
	END LOOP;

RETURN;
END; $$ LANGUAGE plpgsql;


---
-- Obtiene el stock, valorizacion stock, ventas y valorizacion de ventas
-- dia a dia de un producto determinado o de todos los productos
---
CREATE OR REPLACE FUNCTION movimiento_en_periodo (IN fecha_inicio_in timestamp,
       	  	  	   	      	     	  IN fecha_final_in timestamp,
					          IN barcode_in bigint,
       					     	  OUT fecha_out timestamp,
       					     	  OUT stock_fecha_out double precision,
						  OUT valor_stock_fecha_out double precision,
						  OUT cantidad_vendida_fecha_out double precision,
						  OUT monto_venta_fecha_out double precision)
RETURNS SETOF record AS $$
DECLARE
	q text;
	l record;
	costo_fecha double precision;
BEGIN
	fecha_out := fecha_inicio_in;
	stock_fecha_out := 0;
	valor_stock_fecha_out := 0;
	cantidad_vendida_fecha_out := 0;
	monto_venta_fecha_out := 0;
	costo_fecha := 0;

	--TODO: buscar la forma de usar producto_en_fecha una sola vez y los demás días usar movimiento_en_fecha
	-- Se recorre día a día el rango de fecha
	WHILE fecha_out <= fecha_final_in LOOP
	      -- q := $S$ SELECT barcode_out, movimiento_out, costo_promedio_en_fecha_out FROM movimiento_en_fecha ($S$||quote_literal(fecha_out)||$S$,$S$||barcode_in||$S$)$S$;
	      q := $S$ SELECT barcode, costo_fecha, COALESCE (cantidad_fecha,0) AS cantidad_fecha, COALESCE (cantidad_vendida_out,0) AS c_vendido, COALESCE (precio_vendido_out,0) AS m_vendido
	      	       FROM producto_en_fecha2 ($S$||quote_literal(fecha_out+'1 days')||$S$,$S$||barcode_in||$S$) AS pf
		       LEFT JOIN ventas_en_fecha ($S$||quote_literal(fecha_out)||$S$,$S$||barcode_in||$S$) AS vf
		       ON pf.barcode::bigint = vf.barcode_out $S$;

   	      FOR l IN EXECUTE q LOOP
		  stock_fecha_out := l.cantidad_fecha;

		  -- Se obtiene el costo promedio del producto a esa fecha
		  -- SELECT COALESCE (fcd.costo_promedio, 0) INTO costo_fecha
		  -- 	 FROM factura_compra_detalle fcd
		  -- 	 INNER JOIN factura_compra fc
		  -- 	       ON fcd.id_factura_compra = fc.id
		  -- 	 WHERE fc.fecha < fecha_out+'1 days'
		  -- 	       AND fcd.barcode = l.barcode::bigint
		  -- 	 ORDER BY fcd.id_factura_compra DESC
		  -- 	 LIMIT 1;

		  costo_fecha := l.costo_fecha;

		  valor_stock_fecha_out := COALESCE (stock_fecha_out*costo_fecha, 0);

		  -- SELECT INTO cantidad_vendida_fecha_out, monto_venta_fecha_out
		  --     	      COALESCE (cantidad_vendida_out,0), COALESCE (precio_vendido_out,0)
                  -- FROM ventas_en_fecha (fecha_out, l.barcode::bigint);

		  cantidad_vendida_fecha_out := l.c_vendido;
		  monto_venta_fecha_out := l.m_vendido;
		  RETURN NEXT;
	    END LOOP;

    	fecha_out := fecha_out + '1 days';
	END LOOP;

RETURN;
END; $$ LANGUAGE plpgsql;

---
-- Obtiene la información de un producto en un día determinado con el costo_promedio del producto en ese día
---
create or replace function producto_en_fecha2(
       in fecha_inicio timestamp,
       in barcode_in bigint,
       out barcode varchar,
       out codigo_corto varchar,
       out descripcion varchar,
       out marca varchar,
       out cont_un varchar,
       out familia integer,
       out cantidad_ingresada double precision,
       out cantidad_c_anuladas double precision,
       out cantidad_vendida double precision,
       out cantidad_anulada double precision,
       out cantidad_insumida double precision,
       out cantidad_merma double precision,
       out cantidad_devoluciones double precision,
       out cantidad_envio double precision,
       out cantidad_recibida double precision,
       out cantidad_fecha double precision,
       out costo_fecha double precision
       )
returns setof record as $$
declare
q text;
l record;
corriente int4;
materia_prima int4;
begin

corriente := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'CORRIENTE');
materia_prima := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'MATERIA PRIMA');

q := $S$ SELECT p.barcode, p.codigo_corto, p.marca, p.descripcion, p.contenido, p.unidad, p.familia, cantidad_ingresada, cantidad_c_anuladas, cantidad_vendida, unidades_merma, unidades_merma_mcd, cantidad_anulada, cantidad_vmcd, cantidad_devolucion, cantidad_envio, cantidad_mc_envio, cantidad_recibida, cantidad_mc_recibida,
     	 	(SELECT fcd.costo_promedio
			FROM factura_compra_detalle fcd
			INNER JOIN factura_compra fc
			ON fcd.id_factura_compra = fc.id

			WHERE fc.fecha < $S$ ||quote_literal (fecha_inicio+'1 days')|| $S$
			AND barcode = p.barcode
			ORDER BY fcd.id_factura_compra DESC
			LIMIT 1) AS costo_fecha

       	 	FROM producto p

	 	-- Las compras ingresadas hechas hasta la fecha determinada
	 	LEFT JOIN (SELECT SUM(fcd.cantidad) AS cantidad_ingresada, fcd.barcode AS barcode
		          	  FROM factura_compra_detalle fcd
       		     	          INNER JOIN factura_compra fc
       		     	          ON fc.id = fcd.id_factura_compra

       		     	          WHERE fc.fecha < $S$ || quote_literal(fecha_inicio) || $S$
                     	          AND fcd.cantidad > 0
                     	          GROUP BY barcode) AS cantidad_ingresada
                ON p.barcode = cantidad_ingresada.barcode

	        -- Las anulaciones de compras hechas hasta la fecha determinada
		LEFT JOIN (SELECT SUM(cad.cantidad_anulada) AS cantidad_c_anuladas, cad.barcode AS barcode
		       	          FROM compra_anulada ca
				  INNER JOIN compra_anulada_detalle cad
				  ON ca.id = cad.id_compra_anulada

				  WHERE ca.fecha_anulacion < $S$ || quote_literal(fecha_inicio) || $S$
				  GROUP BY barcode) AS compras_anuladas
	        ON p.barcode = compras_anuladas.barcode

       		-- Las Ventas hechas hasta la fecha determinada
       		LEFT JOIN (SELECT SUM(vd.cantidad) AS cantidad_vendida, vd.barcode AS barcode -- LEFT JOIN MUESTRA TODOS LOS PRODUCTOS
       	     	                  FROM venta v
		 	      	  INNER JOIN venta_detalle vd
			  	  ON v.id = vd.id_venta

			  	  WHERE v.fecha < $S$ || quote_literal(fecha_inicio) || $S$
			  	  GROUP BY barcode) AS ventas
		ON p.barcode = ventas.barcode

       	        -- Las anulaciones de venta hechas hasta la fecha determinada
       		LEFT JOIN (SELECT SUM(vd.cantidad) AS cantidad_anulada, vd.barcode AS barcode -- LEFT JOIN MUESTRA TODOS LOS PRODUCTOS
       	     	                  FROM venta v
		 	          INNER JOIN venta_detalle vd
			  	  ON v.id = vd.id_venta

			  	  INNER JOIN venta_anulada va
			    	  ON va.id_sale = v.id

				  WHERE va.fecha < $S$ || quote_literal(fecha_inicio) || $S$
			          GROUP BY barcode) AS ventas_anuladas
                ON p.barcode = ventas_anuladas.barcode

		-- Las Ventas (de compuestos) menos sus anulaciones hechas hasta la fecha determinada
       		LEFT JOIN (SELECT SUM(vmcd.cantidad) AS cantidad_vmcd, vmcd.barcode_componente AS barcode -- LEFT JOIN MUESTRA TODOS LOS PRODUCTOS
       	     	                  FROM venta_mc_detalle vmcd
				  INNER JOIN venta v ON vmcd.id_venta_vd = v.id

			  	  WHERE vmcd.id_venta_vd NOT IN (SELECT id_sale FROM venta_anulada)
				  AND v.fecha < $S$ || quote_literal(fecha_inicio) || $S$
			  	  GROUP BY barcode_componente) AS ventas_mcd
		ON p.barcode = ventas_mcd.barcode

	        -- Las Mermas sufridas hasta la fecha determinada
		LEFT JOIN (SELECT barcode, SUM(unidades) AS unidades_merma
       	     	                  FROM merma m

		         	  WHERE m.fecha < $S$ || quote_literal(fecha_inicio) || $S$
			          GROUP BY barcode) AS merma
	        ON p.barcode = merma.barcode

		-- Las Mermas sufridas hasta la fecha determinada
		LEFT JOIN (SELECT mmcd.barcode_componente AS barcode, SUM(mmcd.cantidad) AS unidades_merma_mcd
       	     	                  FROM merma m
				  INNER JOIN merma_mc_detalle mmcd
				  ON m.id = mmcd.id_merma

		         	  WHERE m.fecha < $S$ || quote_literal(fecha_inicio) || $S$
			          GROUP BY barcode_componente) AS merma_mcd
	        ON p.barcode = merma_mcd.barcode

                -- Las devoluciones hechas hasta la fecha determinada
       		LEFT JOIN (SELECT dd.barcode AS barcode, SUM(dd.cantidad) AS cantidad_devolucion
       	     	                  FROM devolucion d
		 	          INNER JOIN devolucion_detalle dd
			 	  ON d.id = dd.id_devolucion

			          WHERE d.fecha < $S$ || quote_literal(fecha_inicio) || $S$
			          GROUP BY barcode) AS devolucion
                ON p.barcode = devolucion.barcode

	        -- Los traspasos enviados hasta la fecha determinada
       		LEFT JOIN (SELECT td.barcode AS barcode, SUM(td.cantidad) AS cantidad_envio
       	     	                  FROM traspaso t
		 	          INNER JOIN traspaso_detalle td
			 	  ON t.id = td.id_traspaso

			          WHERE t.fecha < $S$ || quote_literal(fecha_inicio) || $S$
				  AND t.origen = 1
			          GROUP BY barcode) AS traspaso_envio
                ON p.barcode = traspaso_envio.barcode

		-- Los traspasos enviados (a traves de un compuesto) hasta la fecha determinada
		LEFT JOIN (SELECT tmcd.barcode_componente AS barcode, SUM(tmcd.cantidad) AS cantidad_mc_envio
       	     	                  FROM traspaso t
		 	          INNER JOIN traspaso_mc_detalle tmcd
			 	  ON t.id = tmcd.id_traspaso

			          WHERE t.fecha < $S$ || quote_literal(fecha_inicio) || $S$
				  AND t.origen = 1
			          GROUP BY barcode_componente) AS traspaso_mc_envio
                ON p.barcode = traspaso_mc_envio.barcode

	        -- Los traspasos recibidos hasta la fecha determinada
       		LEFT JOIN (SELECT td.barcode AS barcode, SUM(td.cantidad) AS cantidad_recibida
       	     	                  FROM traspaso t
		 	          INNER JOIN traspaso_detalle td
			 	  ON t.id = td.id_traspaso

			          WHERE t.fecha < $S$ || quote_literal(fecha_inicio) || $S$
				  AND t.origen != 1
			          GROUP BY barcode) AS traspaso_recibido
                ON p.barcode = traspaso_recibido.barcode

		-- Los traspasos recibidos (a traves de un compuesto) hasta la fecha determinada
       		LEFT JOIN (SELECT tmcd.barcode_componente AS barcode, SUM(tmcd.cantidad) AS cantidad_mc_recibida
       	     	                  FROM traspaso t
		 	          INNER JOIN traspaso_mc_detalle tmcd
			 	  ON t.id = tmcd.id_traspaso

			          WHERE t.fecha < $S$ || quote_literal(fecha_inicio) || $S$
				  AND t.origen != 1
			          GROUP BY barcode_componente) AS traspaso_mc_recibido
                ON p.barcode = traspaso_mc_recibido.barcode

                WHERE p.estado = true
		AND (p.tipo = $S$ || corriente || $S$
		     OR p.tipo = $S$ || materia_prima || $S$ ) $S$ ;

if barcode_in != 0 then
    q := q || $S$ AND p.barcode = $S$ || barcode_in;
end if;

q := q || $S$ GROUP BY p.barcode, p.codigo_corto, p.marca, p.descripcion, p.contenido, p.unidad, p.familia, cantidad_ingresada, cantidad_c_anuladas, cantidad_vendida, unidades_merma, unidades_merma_mcd, cantidad_anulada, cantidad_vmcd, cantidad_devolucion, cantidad_envio, cantidad_mc_envio, cantidad_recibida, cantidad_mc_recibida, costo_fecha
              ORDER BY barcode $S$;

for l in execute q loop
    barcode := l.barcode;
    codigo_corto := l.codigo_corto;
    marca := l.marca;
    descripcion := l.descripcion;
    cont_un := l.contenido ||' '|| l.unidad;
    familia := l.familia;
    cantidad_ingresada := COALESCE (l.cantidad_ingresada,0);
    cantidad_c_anuladas := COALESCE(l.cantidad_c_anuladas,0);
    cantidad_vendida := COALESCE(l.cantidad_vendida,0);
    cantidad_merma := COALESCE(l.unidades_merma,0) + COALESCE(l.unidades_merma_mcd,0);
    cantidad_anulada := COALESCE(l.cantidad_anulada,0);
    cantidad_insumida := COALESCE(l.cantidad_vmcd,0);
    cantidad_devoluciones := COALESCE(l.cantidad_devolucion,0);
    cantidad_envio := COALESCE(l.cantidad_envio,0) + COALESCE(l.cantidad_mc_envio,0);
    cantidad_recibida := COALESCE(l.cantidad_recibida,0) + COALESCE(l.cantidad_mc_recibida,0);
    cantidad_fecha := COALESCE(l.cantidad_ingresada,0) - COALESCE(l.cantidad_c_anuladas,0) - COALESCE(l.cantidad_vendida,0) - COALESCE(l.cantidad_vmcd,0) - COALESCE(l.unidades_merma,0) + COALESCE(l.cantidad_anulada,0) - COALESCE(l.cantidad_devolucion,0) - COALESCE(l.cantidad_envio,0) + COALESCE(l.cantidad_recibida,0);
    costo_fecha := COALESCE (l.costo_fecha, 0);
    return next;
end loop;

return;
end; $$ language plpgsql;


-- Obtiene la información de un producto en un día determinado
create or replace function producto_en_fecha(
       in fecha_inicio timestamp,
       in barcode_in bigint,
       out barcode varchar,
       out codigo_corto varchar,
       out descripcion varchar,
       out marca varchar,
       out cont_un varchar,
       out familia integer,
       out cantidad_ingresada double precision,
       out cantidad_c_anuladas double precision,
       out cantidad_vendida double precision,
       out cantidad_anulada double precision,
       out cantidad_insumida double precision,
       out cantidad_merma double precision,
       out cantidad_devoluciones double precision,
       out cantidad_envio double precision,
       out cantidad_recibida double precision,
       out cantidad_fecha double precision
       )
returns setof record as $$
declare
q text;
l record;
corriente int4;
materia_prima int4;
begin

corriente := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'CORRIENTE');
materia_prima := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'MATERIA PRIMA');

q := $S$ SELECT p.barcode, p.codigo_corto, p.marca, p.descripcion, p.contenido, p.unidad, p.familia, cantidad_ingresada, cantidad_c_anuladas, cantidad_vendida, unidades_merma, unidades_merma_mcd, cantidad_anulada, cantidad_vmcd, cantidad_devolucion, cantidad_envio, cantidad_mc_envio, cantidad_recibida, cantidad_mc_recibida
       	 	FROM producto p

	 	-- Las compras ingresadas hechas hasta la fecha determinada
	 	LEFT JOIN (SELECT SUM(fcd.cantidad) AS cantidad_ingresada, fcd.barcode AS barcode
		          	  FROM factura_compra_detalle fcd
       		     	          INNER JOIN factura_compra fc
       		     	          ON fc.id = fcd.id_factura_compra

       		     	          WHERE fc.fecha < $S$ || quote_literal(fecha_inicio) || $S$
                     	          AND fcd.cantidad > 0
                     	          GROUP BY barcode) AS cantidad_ingresada
                ON p.barcode = cantidad_ingresada.barcode

	        -- Las anulaciones de compras hechas hasta la fecha determinada
		LEFT JOIN (SELECT SUM(cad.cantidad_anulada) AS cantidad_c_anuladas, cad.barcode AS barcode
		       	          FROM compra_anulada ca
				  INNER JOIN compra_anulada_detalle cad
				  ON ca.id = cad.id_compra_anulada

				  WHERE ca.fecha_anulacion < $S$ || quote_literal(fecha_inicio) || $S$
				  GROUP BY barcode) AS compras_anuladas
	        ON p.barcode = compras_anuladas.barcode

       		-- Las Ventas hechas hasta la fecha determinada
       		LEFT JOIN (SELECT SUM(vd.cantidad) AS cantidad_vendida, vd.barcode AS barcode -- LEFT JOIN MUESTRA TODOS LOS PRODUCTOS
       	     	                  FROM venta v
		 	      	  INNER JOIN venta_detalle vd
			  	  ON v.id = vd.id_venta

			  	  WHERE v.fecha < $S$ || quote_literal(fecha_inicio) || $S$
			  	  GROUP BY barcode) AS ventas
		ON p.barcode = ventas.barcode

       	        -- Las anulaciones de venta hechas hasta la fecha determinada
       		LEFT JOIN (SELECT SUM(vd.cantidad) AS cantidad_anulada, vd.barcode AS barcode -- LEFT JOIN MUESTRA TODOS LOS PRODUCTOS
       	     	                  FROM venta v
		 	          INNER JOIN venta_detalle vd
			  	  ON v.id = vd.id_venta

			  	  INNER JOIN venta_anulada va
			    	  ON va.id_sale = v.id

				  WHERE va.fecha < $S$ || quote_literal(fecha_inicio) || $S$
			          GROUP BY barcode) AS ventas_anuladas
                ON p.barcode = ventas_anuladas.barcode

		-- Las Ventas (de compuestos) menos sus anulaciones hechas hasta la fecha determinada
       		LEFT JOIN (SELECT SUM(vmcd.cantidad) AS cantidad_vmcd, vmcd.barcode_componente AS barcode -- LEFT JOIN MUESTRA TODOS LOS PRODUCTOS
       	     	                  FROM venta_mc_detalle vmcd
				  INNER JOIN venta v ON vmcd.id_venta_vd = v.id

			  	  WHERE vmcd.id_venta_vd NOT IN (SELECT id_sale FROM venta_anulada)
				  AND v.fecha < $S$ || quote_literal(fecha_inicio) || $S$
			  	  GROUP BY barcode_componente) AS ventas_mcd
		ON p.barcode = ventas_mcd.barcode

	        -- Las Mermas sufridas hasta la fecha determinada
		LEFT JOIN (SELECT barcode, SUM(unidades) AS unidades_merma
       	     	                  FROM merma m

		         	  WHERE m.fecha < $S$ || quote_literal(fecha_inicio) || $S$
			          GROUP BY barcode) AS merma
	        ON p.barcode = merma.barcode

		-- Las Mermas sufridas hasta la fecha determinada
		LEFT JOIN (SELECT mmcd.barcode_componente AS barcode, SUM(mmcd.cantidad) AS unidades_merma_mcd
       	     	                  FROM merma m
				  INNER JOIN merma_mc_detalle mmcd
				  ON m.id = mmcd.id_merma

		         	  WHERE m.fecha < $S$ || quote_literal(fecha_inicio) || $S$
			          GROUP BY barcode_componente) AS merma_mcd
	        ON p.barcode = merma_mcd.barcode

                -- Las devoluciones hechas hasta la fecha determinada
       		LEFT JOIN (SELECT dd.barcode AS barcode, SUM(dd.cantidad) AS cantidad_devolucion
       	     	                  FROM devolucion d
		 	          INNER JOIN devolucion_detalle dd
			 	  ON d.id = dd.id_devolucion

			          WHERE d.fecha < $S$ || quote_literal(fecha_inicio) || $S$
			          GROUP BY barcode) AS devolucion
                ON p.barcode = devolucion.barcode

	        -- Los traspasos enviados hasta la fecha determinada
       		LEFT JOIN (SELECT td.barcode AS barcode, SUM(td.cantidad) AS cantidad_envio
       	     	                  FROM traspaso t
		 	          INNER JOIN traspaso_detalle td
			 	  ON t.id = td.id_traspaso

			          WHERE t.fecha < $S$ || quote_literal(fecha_inicio) || $S$
				  AND t.origen = 1
			          GROUP BY barcode) AS traspaso_envio
                ON p.barcode = traspaso_envio.barcode

		-- Los traspasos enviados (a traves de un compuesto) hasta la fecha determinada
		LEFT JOIN (SELECT tmcd.barcode_componente AS barcode, SUM(tmcd.cantidad) AS cantidad_mc_envio
       	     	                  FROM traspaso t
		 	          INNER JOIN traspaso_mc_detalle tmcd
			 	  ON t.id = tmcd.id_traspaso

			          WHERE t.fecha < $S$ || quote_literal(fecha_inicio) || $S$
				  AND t.origen = 1
			          GROUP BY barcode_componente) AS traspaso_mc_envio
                ON p.barcode = traspaso_mc_envio.barcode

	        -- Los traspasos recibidos hasta la fecha determinada
       		LEFT JOIN (SELECT td.barcode AS barcode, SUM(td.cantidad) AS cantidad_recibida
       	     	                  FROM traspaso t
		 	          INNER JOIN traspaso_detalle td
			 	  ON t.id = td.id_traspaso

			          WHERE t.fecha < $S$ || quote_literal(fecha_inicio) || $S$
				  AND t.origen != 1
			          GROUP BY barcode) AS traspaso_recibido
                ON p.barcode = traspaso_recibido.barcode

		-- Los traspasos recibidos (a traves de un compuesto) hasta la fecha determinada
       		LEFT JOIN (SELECT tmcd.barcode_componente AS barcode, SUM(tmcd.cantidad) AS cantidad_mc_recibida
       	     	                  FROM traspaso t
		 	          INNER JOIN traspaso_mc_detalle tmcd
			 	  ON t.id = tmcd.id_traspaso

			          WHERE t.fecha < $S$ || quote_literal(fecha_inicio) || $S$
				  AND t.origen != 1
			          GROUP BY barcode_componente) AS traspaso_mc_recibido
                ON p.barcode = traspaso_mc_recibido.barcode

                WHERE p.estado = true
		AND (p.tipo = $S$ || corriente || $S$
		     OR p.tipo = $S$ || materia_prima || $S$ ) $S$ ;

if barcode_in != 0 then
    q := q || $S$ AND p.barcode = $S$ || barcode_in;
end if;

q := q || $S$ GROUP BY p.barcode, p.codigo_corto, p.marca, p.descripcion, p.contenido, p.unidad, p.familia, cantidad_ingresada, cantidad_c_anuladas, cantidad_vendida, unidades_merma, unidades_merma_mcd, cantidad_anulada, cantidad_vmcd, cantidad_devolucion, cantidad_envio, cantidad_mc_envio, cantidad_recibida, cantidad_mc_recibida
              ORDER BY barcode $S$;

for l in execute q loop
    barcode := l.barcode;
    codigo_corto := l.codigo_corto;
    marca := l.marca;
    descripcion := l.descripcion;
    cont_un := l.contenido ||' '|| l.unidad;
    familia := l.familia;
    cantidad_ingresada := COALESCE (l.cantidad_ingresada,0);
    cantidad_c_anuladas := COALESCE(l.cantidad_c_anuladas,0);
    cantidad_vendida := COALESCE(l.cantidad_vendida,0);
    cantidad_merma := COALESCE(l.unidades_merma,0) + COALESCE(l.unidades_merma_mcd,0);
    cantidad_anulada := COALESCE(l.cantidad_anulada,0);
    cantidad_insumida := COALESCE(l.cantidad_vmcd,0);
    cantidad_devoluciones := COALESCE(l.cantidad_devolucion,0);
    cantidad_envio := COALESCE(l.cantidad_envio,0) + COALESCE(l.cantidad_mc_envio,0);
    cantidad_recibida := COALESCE(l.cantidad_recibida,0) + COALESCE(l.cantidad_mc_recibida,0);
    cantidad_fecha := COALESCE(l.cantidad_ingresada,0) - COALESCE(l.cantidad_c_anuladas,0) - COALESCE(l.cantidad_vendida,0) - COALESCE(l.cantidad_vmcd,0) - COALESCE(l.unidades_merma,0) + COALESCE(l.cantidad_anulada,0) - COALESCE(l.cantidad_devolucion,0) - COALESCE(l.cantidad_envio,0) + COALESCE(l.cantidad_recibida,0);
    return next;
end loop;

return;
end; $$ language plpgsql;


-- Entrega la informacion del producto desde la fecha otorgada hasta ahora
create or replace function producto_en_periodo(
       in fecha_inicio timestamp,
       out barcode varchar,
       out codigo_corto varchar,
       out descripcion varchar,
       out marca varchar,
       out cont_un varchar,
       out familia integer,
       out stock_inicial double precision,
       out compras_periodo double precision,
       out anulaciones_c_periodo double precision,
       out ventas_periodo double precision,
       out anulaciones_periodo double precision,
       out insumidos_periodo double precision,
       out devoluciones_periodo double precision,
       out mermas_periodo double precision,
       out enviados_periodo double precision,
       out recibidos_periodo double precision,
       out stock_teorico double precision
       )
RETURNS setof record AS $$
DECLARE
q text;
l record;
z record;
BEGIN

q := $S$ SELECT stock1.barcode AS barcode,
	        stock1.codigo_corto AS codigo_corto,
       	 	stock1.descripcion AS descripcion,
		stock1.marca AS marca,
		stock1.cont_un AS cont_un,
		stock1.familia AS familia,

		-- Los mismos datos de distintas fechas --
       	 	-- stock inicial
       	 	stock1.cantidad_fecha AS stock1_cantidad_fecha,
       	 	stock2.cantidad_fecha AS stock2_cantidad_fecha,
       	 	-- compras_periodo
       	 	stock1.cantidad_ingresada AS stock1_cantidad_ingresada,
       	 	stock2.cantidad_ingresada AS stock2_cantidad_ingresada,
		-- anulaciones_c_periodo
		stock1.cantidad_c_anuladas AS stock1_cantidad_c_anuladas,
       	 	stock2.cantidad_c_anuladas AS stock2_cantidad_c_anuladas,
       	 	-- ventas_periodo
       	 	stock1.cantidad_vendida AS stock1_cantidad_vendida,
       	 	stock2.cantidad_vendida AS stock2_cantidad_vendida,
		-- anulaciones_periodo
       	 	stock1.cantidad_anulada AS stock1_cantidad_anulada,
       	 	stock2.cantidad_anulada AS stock2_cantidad_anulada,
		-- insumidos_periodo
       	 	stock1.cantidad_insumida AS stock1_cantidad_insumida,
       	 	stock2.cantidad_insumida AS stock2_cantidad_insumida,
       	 	-- devoluciones_periodo
       	 	stock1.cantidad_devoluciones AS stock1_cantidad_devoluciones,
       	 	stock2.cantidad_devoluciones AS stock2_cantidad_devoluciones,
       	 	-- mermas_periodo
       	 	stock1.cantidad_merma AS stock1_cantidad_merma,
       	 	stock2.cantidad_merma AS stock2_cantidad_merma,
       	 	-- envios_periodo
       	 	stock1.cantidad_envio AS stock1_cantidad_envio,
       	 	stock2.cantidad_envio AS stock2_cantidad_envio,
		-- recibidos_periodo
       	 	stock1.cantidad_recibida AS stock1_cantidad_recibida,
       	 	stock2.cantidad_recibida AS stock2_cantidad_recibida,

       	 	-- El stock actual --
       	 	-- stock_teorico
       	 	stock2.cantidad_fecha

	 FROM producto_en_fecha( $S$ || quote_literal(fecha_inicio) || $S$, 0 ) stock1 INNER JOIN producto_en_fecha( $S$ || quote_literal(current_date::timestamp + '1 days') || $S$, 0 ) stock2
       	 ON stock1.barcode = stock2.barcode $S$;

FOR l IN EXECUTE q loop
    barcode := l.barcode;
    codigo_corto := l.codigo_corto;
    marca := l.marca;
    familia := l.familia;
    descripcion := l.descripcion;
    cont_un := l.cont_un;
    stock_inicial := l.stock1_cantidad_fecha;  -- cantidad_fecha = stock con el que se inicio el día seleccionado (ESTE SE MANTIENE)
    compras_periodo := l.stock2_cantidad_ingresada - l.stock1_cantidad_ingresada;
    anulaciones_c_periodo := l.stock2_cantidad_c_anuladas - l.stock1_cantidad_c_anuladas;
    ventas_periodo := l.stock2_cantidad_vendida - l.stock1_cantidad_vendida;
    anulaciones_periodo := l.stock2_cantidad_anulada - l.stock1_cantidad_anulada;
    insumidos_periodo := l.stock2_cantidad_insumida - l.stock1_cantidad_insumida;
    devoluciones_periodo := l.stock2_cantidad_devoluciones - l.stock1_cantidad_devoluciones;
    mermas_periodo := l.stock2_cantidad_merma - l.stock1_cantidad_merma;
    enviados_periodo := l.stock2_cantidad_envio - l.stock1_cantidad_envio;
    recibidos_periodo := l.stock2_cantidad_recibida - l.stock1_cantidad_recibida;
    stock_teorico := l.stock2_cantidad_fecha;
    RETURN NEXT;
END loop;

return;
end; $$ language plpgsql;


--
-- This funtion get true if date is valid
--
CREATE OR REPLACE FUNCTION is_valid_date(char, char) RETURNS boolean AS $$
DECLARE
     result BOOL;
BEGIN
     SELECT TO_CHAR(TO_DATE($1,$2),$2) = $1
     INTO result;
     RETURN result;
END; $$ LANGUAGE plpgsql;


----
-- Busca deudas del cliente
-- Si la venta fue mixta, retorna información del segundo modo de pago
-- de ser solamente a crédito, retorna solo información de credito y rellena
-- las demás columnas con -1
CREATE OR replace FUNCTION search_deudas_cliente (
       IN rut INT,
       IN solo_no_pagadas boolean,
       OUT id INT,
       OUT monto INT,
       OUT maquina INT,
       OUT vendedor INT,
       OUT tipo_venta INT,
       OUT tipo_complementario INT,
       OUT monto_complementario INT,
       OUT fecha TIMESTAMP WITHOUT TIME ZONE)
RETURNS setof record AS $$
DECLARE
        sale_amount INTEGER;
        sale_type INTEGER;
        query TEXT;
	l RECORD;
BEGIN
	--                       0      1             2            3       4       5
	--TIPO PAGO EN RIZOMA (CASH, CREDITO, CHEQUE_RESTAURANT, MIXTO, CHEQUE, TARJETA)
	--
	IF solo_no_pagadas = true THEN --Solo se listan las dedudas sin pago
	   query := $S$ SELECT v.id, v.monto, v.maquina, v.vendedor, v.fecha, v.tipo_venta,
	      	     	       pm.rut1, pm.rut2, pm.tipo_pago1, pm.tipo_pago2, pm.monto1, pm.monto2
	      	     	FROM venta v LEFT JOIN pago_mixto pm ON pm.id_sale = v.id
		     	WHERE v.id IN (SELECT id_venta FROM deuda WHERE rut_cliente=$S$||rut||$S$ AND pagada='f')
		     	AND v.id NOT IN (SELECT id_sale FROM venta_anulada)
		     	ORDER BY v.id DESC $S$;
	ELSE --Se listan todas las deudas (incluso las ya pagadas)
	   query := $S$ SELECT v.id, v.monto, v.maquina, v.vendedor, v.fecha, v.tipo_venta,
	      	     	       pm.rut1, pm.rut2, pm.tipo_pago1, pm.tipo_pago2, pm.monto1, pm.monto2
	      	        FROM venta v LEFT JOIN pago_mixto pm ON pm.id_sale = v.id
		     	WHERE v.id IN (SELECT id_venta FROM deuda WHERE rut_cliente=$S$||rut||$S$)
		     	AND v.id NOT IN (SELECT id_sale FROM venta_anulada)
		     	ORDER BY v.id DESC $S$;
	END IF;

        FOR l IN EXECUTE query loop
	      	id = l.id;
		maquina = l.maquina;
		vendedor = l.vendedor;
		fecha = l.fecha;
		tipo_venta = l.tipo_venta;

		-- Si es una venta mixta
		IF l.tipo_venta = 3 THEN

		   -- Si ambos pagos son credito y pertenecen a la misma cuenta
		   IF l.tipo_pago1 = 1 AND l.rut1 = rut AND l.tipo_pago2 = 1 AND l.rut2 = rut THEN
		      monto = l.monto1 + l.monto2;
		   ELSE
		      -- Si el primer pago fue a credito y pertenece a esta cuenta
		      IF l.tipo_pago1 = 1 AND l.rut1 = rut THEN
		         monto = l.monto1;
			 tipo_complementario = l.tipo_pago2;
			 monto_complementario = l.monto2;
		      ELSE -- Si el segundo pago fue a credito y pertenece a esta cuenta
			 monto = l.monto2;
			 tipo_complementario = l.tipo_pago1;
			 monto_complementario = l.monto1;
		      END IF;
		   END IF;

		ELSE
		   monto = l.monto;
		   tipo_complementario = -1;
		   monto_complementario = 0;
		END IF;
		RETURN NEXT;
        END loop;

RETURN;
END; $$ LANGUAGE plpgsql;



--
--
--
CREATE OR replace FUNCTION search_facturas_guias (IN rut_cliente_in INT,
						  IN solo_pendientes_in boolean,
						  IN tipo_guia_in INT,
						  IN tipo_factura_in INT,
						  IN tipo_documento_in INT, -- Tipo documento a mostrar (0 = guias y facturas)
						  IN filtro_in VARCHAR,
       						  OUT id_venta_out INT,
						  OUT id_documento_out INT,
						  OUT id_factura_out INT,
						  OUT rut_cliente_out INT,
       						  OUT monto_out INT,
       						  OUT maquina_out INT,
       						  OUT vendedor_out INT,
       						  OUT tipo_documento_out INT,
       						  OUT fecha_emision_out TIMESTAMP WITHOUT TIME ZONE,
						  OUT pagado_out BOOLEAN)
RETURNS setof record AS $$
DECLARE
        query TEXT;
	l RECORD;
BEGIN
	--
	--
	--
	IF solo_pendientes_in = TRUE THEN --Solo se listan guias sin facturar y las facturas sin pago (con sus guias (facturadas) respectivas en caso de tenerlas)
   	   CREATE TEMPORARY TABLE facturas_guias AS
		  WITH RECURSIVE facturas (id_venta, maquina, vendedor, id_documento, id_factura, fecha_emision, tipo_documento, monto, rut_cliente, pagado) AS
		  (
			SELECT COALESCE (de.id_venta,0) AS id_venta, COALESCE (maquina,0) AS maquina,
			       COALESCE(vendedor,0) AS vendedor, de.id AS id_documento, de.id_factura,
			       de.fecha_emision, de.tipo_documento, de.monto, de.rut_cliente, de.pagado
	   	  	FROM documentos_emitidos de
	          	LEFT JOIN venta v ON v.id = de.id_venta
	   	  	WHERE (de.tipo_documento = tipo_factura_in OR (de.tipo_documento = tipo_guia_in AND id_factura = 0))
	   	  	AND de.pagado = FALSE
		  	AND id_venta NOT IN (SELECT id_sale FROM venta_anulada)
		  	UNION ALL
		        SELECT v.id AS id_venta, v.maquina, v.vendedor, de.id AS id_documento, de.id_factura,
   	    	       	       de.fecha_emision, de.tipo_documento, de.monto, de.rut_cliente, de.pagado
	      	  	FROM venta v INNER JOIN documentos_emitidos de
		  	ON v.id = de.id_venta
		  	INNER JOIN facturas
		  	ON facturas.id_documento = de.id_factura
		  	AND v.id NOT IN (SELECT id_sale FROM venta_anulada)
		  )
		  SELECT * FROM facturas;
	ELSE --Se listan todas las deudas (incluso las ya pagadas)
   	   CREATE TEMPORARY TABLE facturas_guias AS
	   	  WITH RECURSIVE facturas (id_venta, maquina, vendedor, id_documento, id_factura, fecha_emision, tipo_documento, monto, rut_cliente, pagado) AS
		  (
			SELECT COALESCE (de.id_venta,0) AS id_venta, COALESCE (maquina,0) AS maquina,
			       COALESCE(vendedor,0) AS vendedor, de.id AS id_documento, de.id_factura,
			       de.fecha_emision, de.tipo_documento, de.monto, de.rut_cliente, de.pagado
	   	  	FROM documentos_emitidos de
	          	LEFT JOIN venta v ON v.id = de.id_venta
	   	  	WHERE id_venta NOT IN (SELECT id_sale FROM venta_anulada)
		  	UNION ALL
		        SELECT v.id AS id_venta, v.maquina, v.vendedor, de.id AS id_documento, de.id_factura,
   	    	       	       de.fecha_emision, de.tipo_documento, de.monto, de.rut_cliente, de.pagado
	      	  	FROM venta v INNER JOIN documentos_emitidos de
		  	ON v.id = de.id_venta
		  	INNER JOIN facturas
		  	ON facturas.id_documento = de.id_factura
		  	AND v.id NOT IN (SELECT id_sale FROM venta_anulada)
		  )
		  SELECT * FROM facturas;
	END IF;

	query := $S$ SELECT * FROM facturas_guias WHERE id_venta IS NOT NULL $S$;

	IF rut_cliente_in != 0 THEN
	   query := query || $S$ AND rut_cliente = $S$||rut_cliente_in;
	END IF;

	IF tipo_documento_in = tipo_factura_in THEN
	   query := query || $S$ AND (tipo_documento = $S$||tipo_factura_in||$S$ OR id_factura != 0) $S$;
	ELSIF tipo_documento_in = tipo_guia_in THEN
	   query := query || $S$ AND tipo_documento = $S$||tipo_guia_in;
	   IF solo_pendientes_in = TRUE THEN
	      query := query || $S$ AND id_factura = 0 $S$;
	   END IF;
	END IF;

	IF filtro_in != '' THEN
	   query := query || $S$ AND ($S$||filtro_in||$S$)$S$;
	END IF;

	query := query || $S$ ORDER BY id_venta DESC $S$;

        FOR l IN EXECUTE query loop
	      	id_venta_out := l.id_venta;
		id_documento_out := l.id_documento;
		id_factura_out := l.id_factura;
		rut_cliente_out := l.rut_cliente;
		maquina_out := l.maquina;
		vendedor_out := l.vendedor;
		fecha_emision_out := l.fecha_emision;
		tipo_documento_out := l.tipo_documento;
		monto_out := l.monto;
		pagado_out := l.pagado;
		RETURN NEXT;
        END loop;

	DROP TABLE facturas_guias;

RETURN;
END; $$ LANGUAGE plpgsql;



---
-- Función recursiva (solo se llama a sí misma cuando se topa con una mercadería compuesta)
-- Ingresa el detalle del compuesto en la tabla venta_mc_detalle
---
CREATE OR REPLACE FUNCTION registrar_detalle_compuesto (IN id_venta_in int4,
       	  	  	   				IN id_venta_detalle_in int4,
       	  	  	   				IN barcode_madre_in bigint,
       	  	  	   				IN id_mh_in int4[], -- Debe ser ARRAY[0,0] !!!
							IN costo_madre_in double precision, -- Debe ser 0 !!!
							IN precio_proporcional_in double precision, -- precio de venta madre
							IN cantidad_in double precision, -- cantidad madre
							IN proporcion_iva_in double precision,
							IN proporcion_otros_in double precision,
							OUT iva_out double precision,
							OUT otros_out double precision,
							OUT iva_residual_out double precision,
							OUT otros_residual_out double precision,
							OUT ganancia_out double precision,
							OUT tipo_out int4)
RETURNS SETOF record AS $$

DECLARE
	-- Consulta inicial
	q text;
	l record;
	--------------------------
	-- Subconsulta (recursiva)
	q2 text;
	l2 record;
	--------------------------
	compuesta_l int4; -- id tipo compuesto
	derivada_l int4;  -- id tipo derivado
	corriente_l int4; -- id tipo corriente
	materia_prima_l int4; -- id tipo materia prima
	--------------------------
	-- Datos mercadería madre
	costo_l double precision;
	precio_proporcional_l double precision;
	iva_madre double precision;
	otros_madre double precision;
	iva_residual_madre double precision;
	otros_residual_madre double precision;
	ganancia_madre double precision;
	-- Mercaderia hija temporal (materia prima)
	barcode_mp bigint;
	cantidad_mp double precision;
	costo_mp double precision;
	--------------------------
BEGIN
	SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';
	SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
	SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';
	SELECT id INTO materia_prima_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA';

	IF (id_mh_in[1] = 0 AND id_mh_in[2] = 0) THEN
	   -- Obtengo el costo de madre
	   SELECT costo INTO costo_l FROM obtener_costo_promedio_desde_barcode (barcode_madre_in);
	   -- Creo una secuencia temporal para crear un id único para cada producto
	   CREATE SEQUENCE id_mh_seq START 1;
	   -- Se realiza la consulta recursiva y el resultado se almacena en una tabla temporal
	   CREATE TEMPORARY TABLE arbol_componentes AS
	   WITH RECURSIVE compuesta (barcode_complejo, id_mh, tipo_complejo, barcode_componente, tipo_componente, cant_mud) AS
      	      	(
		   SELECT barcode_complejo,
		   	  ARRAY[0, nextval('id_mh_seq')] AS id_mh,
			  tipo_complejo, barcode_componente, tipo_componente, cant_mud
       		   FROM componente_mc WHERE barcode_complejo = barcode_madre_in
        	   UNION ALL
        	   SELECT componente_mc.barcode_complejo,
		   	  ARRAY[id_mh[2], nextval('id_mh_seq')],
			  componente_mc.tipo_complejo,
		      	  componente_mc.barcode_componente, componente_mc.tipo_componente,
           	      	  componente_mc.cant_mud * compuesta.cant_mud
              	   FROM componente_mc, compuesta
       		   WHERE componente_mc.barcode_complejo = compuesta.barcode_componente
      		)
		SELECT c.barcode_complejo, c.id_mh, c.tipo_complejo, c.barcode_componente, c.tipo_componente, c.cant_mud,
		       (SELECT precio FROM producto WHERE barcode = c.barcode_componente) AS precio_componente,
		       (SELECT costo FROM obtener_costo_promedio_desde_barcode (c.barcode_componente)) AS costo_componente,
		       (SELECT valor FROM get_iva (c.barcode_componente))/100 AS iva,
		       (SELECT valor FROM get_otro_impuesto (c.barcode_componente))/100 AS otros
      		FROM compuesta c
		ORDER BY id_mh[2] ASC;
	ELSE
	   costo_l := costo_madre_in;
	END IF;

	q := $S$ SELECT * FROM arbol_componentes
	     	 WHERE id_mh[1] = $S$ || id_mh_in[2] || $S$
		 ORDER BY id_mh[2] $S$;

	FOR l IN EXECUTE q LOOP
	    -- Si es una mercadería compuesta entra en ella
	    IF (l.tipo_componente = compuesta_l OR l.tipo_componente = derivada_l) THEN
	       -- PRECIO PROPORCIONAL (costo_componente/costo_madre) * precio_proporcional_madre
	       precio_proporcional_l := (l.costo_componente/costo_l) * precio_proporcional_in;

	       q2 := 'SELECT * FROM registrar_detalle_compuesto ('||id_venta_in||'::int4,'
	       	     	       	    				  ||id_venta_detalle_in||'::int4,'
								  ||barcode_madre_in||'::bigint,
								  ARRAY['||l.id_mh[1]||','||l.id_mh[2]||']::int4[],'
								  ||l.costo_componente||'::double precision,'
								  ||precio_proporcional_l||'::double precision,'
								  ||cantidad_in||'::double precision,'
								  ||proporcion_iva_in||'::double precision,'
								  ||proporcion_otros_in||'::double precision)';
	       iva_madre := 0;
	       otros_madre := 0;
	       iva_residual_madre := 0;
	       otros_residual_madre := 0;
	       ganancia_madre := 0;
	       FOR l2 IN EXECUTE q2 LOOP
	       	   -- Sumo los impuestos y la ganancia de los componentes para setearselo al padre
		   iva_madre := iva_madre + l2.iva_out;
		   otros_madre := otros_madre + l2.otros_out;
		   iva_residual_madre := iva_residual_madre + l2.iva_residual_out;
		   otros_residual_madre := otros_residual_madre + l2.otros_residual_out;
		   ganancia_madre := ganancia_madre + l2.ganancia_out;
	       END LOOP;

	       iva_out := iva_madre;
	       otros_out := otros_madre;
	       iva_residual_out := iva_residual_madre;
	       otros_residual_out := otros_residual_madre;
	       ganancia_out := ganancia_madre;

	       -- Se guardan los valores de la mercadería madre que corresponden
	       INSERT INTO venta_mc_detalle (id, id_venta_detalle, id_venta_vd, id_mh, barcode_complejo, barcode_componente, cantidad,
				       	     precio_proporcional, precio, costo_promedio, ganancia, iva, otros, iva_residual, otros_residual,
				       	     tipo_complejo, tipo_componente, proporcion_iva, proporcion_otros)
	       VALUES (DEFAULT, id_venta_detalle_in, id_venta_in, l.id_mh, l.barcode_complejo, l.barcode_componente, l.cant_mud * cantidad_in,
	    	       precio_proporcional_l, l.precio_componente, l.costo_componente, ganancia_madre, iva_madre, otros_madre, iva_residual_madre, otros_residual_madre,
		       l.tipo_complejo, l.tipo_componente, proporcion_iva_in, proporcion_otros_in);

	    ELSE --Si no es mercadería compuesta
	       -- PRECIO PROPORCIONAL (costo_componente/costo_madre) * precio_proporcional_madre
	       precio_proporcional_l := (l.costo_componente/costo_l) * precio_proporcional_in;
	       -- IVA ((PRECIO NETO -> precio de venta sin impuestos) * iva)  * cantidad_requerida * cantidad_vendida
	       iva_out := ((precio_proporcional_l / (l.iva + l.otros + 1)) * l.iva) * l.cant_mud * cantidad_in;
	       -- OTROS ((PRECIO NETO -> precio de venta sin impuestos) * otros) * cantidad_requerida * cantidad_vendida
	       otros_out := ((precio_proporcional_l / (l.iva + l.otros + 1)) * l.otros) * l.cant_mud * cantidad_in;
	       -- IVA y OTROS (residuales-> por pago con cheques de restaurant (singulares o mixtos))
	       iva_residual_out := ((precio_proporcional_l / (l.iva + l.otros + 1)) * l.iva) * l.cant_mud * cantidad_in * proporcion_iva_in;
	       otros_residual_out := ((precio_proporcional_l / (l.iva + l.otros + 1)) * l.otros) * l.cant_mud * cantidad_in * proporcion_otros_in;
	       -- GANANCIA ((PRECIO NETO -> precio de venta sin impuestos) - costo del producto) * cantidad_requerida * cantidad_vendida
	       ganancia_out := ((precio_proporcional_l / (l.iva + l.otros + 1)) - l.costo_componente) * l.cant_mud * cantidad_in;

	       INSERT INTO venta_mc_detalle (id, id_venta_detalle, id_venta_vd, id_mh, barcode_complejo, barcode_componente, cantidad,
				       	     precio_proporcional, precio, costo_promedio, ganancia, iva, otros, iva_residual, otros_residual,
				       	     tipo_complejo, tipo_componente, proporcion_iva, proporcion_otros)
	       VALUES (DEFAULT, id_venta_detalle_in, id_venta_in, l.id_mh, l.barcode_complejo, l.barcode_componente, l.cant_mud * cantidad_in,
	    	       precio_proporcional_l, l.precio_componente, l.costo_componente, ganancia_out, iva_out, otros_out, iva_residual_out, otros_residual_out,
		       l.tipo_complejo, l.tipo_componente, proporcion_iva_in, proporcion_otros_in);

	       UPDATE producto SET stock=stock-(l.cant_mud * cantidad_in) WHERE barcode=l.barcode_componente;
	    END IF;

	    tipo_out := l.tipo_componente;

	    RETURN NEXT;
	END LOOP;

	-- Una vez terminado todo el ciclo (y la recursividad)
	-- se realizan los últimos ajustes
	IF (id_mh_in[1] = 0 AND id_mh_in[2] = 0) THEN
	   -- Se actualizan los impuestos y ganancia de la mercadería
	   -- compuesta en venta detalle
	   UPDATE venta_detalle
	   	  SET iva = vmcd.sum_iva,
		      otros = vmcd.sum_otros,
		      iva_residual = vmcd.sum_iva_residual,
		      otros_residual = vmcd.sum_otros_residual,
		      ganancia = vmcd.sum_ganancia
		  FROM
		  (
		    SELECT (SUM (iva)) AS sum_iva,
		    	   (SUM (otros)) AS sum_otros,
			   (SUM (iva_residual)) AS sum_iva_residual,
			   (SUM (otros_residual)) AS sum_otros_residual,
			   (SUM (ganancia)) AS sum_ganancia
	   	    FROM venta_mc_detalle
	   	    WHERE id_venta_vd = id_venta_in
	   	    	  AND id_venta_detalle = id_venta_detalle_in
		 	  AND tipo_componente != compuesta_l
		 	  AND tipo_componente != derivada_l
		  ) vmcd
	   WHERE id_venta = id_venta_in
	   	 AND id = id_venta_detalle_in;

           -- Se elimina la tabla temporal arbol_componentes
	   DROP TABLE arbol_componentes;
	   -- Se elimina la secuencia temporal
	   DROP SEQUENCE id_mh_seq;
	END IF;
RETURN;
END; $$ LANGUAGE plpgsql;

--
-- Actualiza el monto de la factura calculando los valores de su detalle
--
CREATE OR REPLACE FUNCTION update_factura_compra_amount (IN id_factura_compra_in integer)
RETURNS VOID AS $$
BEGIN
	UPDATE factura_compra
	SET monto = (SELECT SUM ((precio*cantidad)+iva+otros)
	  	            FROM factura_compra_detalle fcd
			    INNER JOIN factura_compra fc
			    ON fcd.id_factura_compra = fc.id
			    WHERE fc.id = id_factura_compra_in)
	WHERE id = id_factura_compra_in;

RETURN;
END; $$ LANGUAGE plpgsql;


--
-- Actualiza la cantidad comprada (a partir de sus facturas)
-- y recalcula el monto total de acuerdo a su detalle
--
CREATE OR REPLACE FUNCTION update_compra_detalle_amount (IN id_compra_in integer)
RETURNS VOID AS $$

DECLARE
	q text;
	l record;
BEGIN
	q := $S$ SELECT fc.id_compra, fcd.barcode, cd.barcode_product, fcd.precio, cd.cantidad_ingresada,
       	     	 	SUM (fcd.cantidad) AS cantidad_total_factura_compra, cd.cantidad AS cantidad_pedida
		 	FROM compra c
			     INNER JOIN compra_detalle cd
			     	   ON c.id = cd.id_compra
			     INNER JOIN factura_compra fc
			     	   ON fc.id_compra = c.id
			     INNER JOIN factura_compra_detalle fcd
			     	   ON fcd.id_factura_compra = fc.id
			WHERE barcode = barcode_product
			     AND c.id = $S$|| id_compra_in ||$S$
			GROUP BY fc.id_compra, fcd.barcode, fcd.precio, cd.barcode_product, cd.cantidad_ingresada, cd.cantidad $S$;

	FOR l IN EXECUTE q LOOP
	    -- Se actualiza la cantidad ingresada en compra_detalle
	    UPDATE compra_detalle
	    	   SET cantidad_ingresada = l.cantidad_total_factura_compra,
		       precio = l.precio
	    	   WHERE barcode_product = l.barcode
		   AND id_compra = id_compra_in;

            -- Si la cantidad pedida en compra es menor a la cantidad ingresada
	    --IF l.cantidad_pedida < l.cantidad_total_factura_compra THEN

	    -- Se igualan las cantidades (pedida y solicitada)
	    UPDATE compra_detalle
	           SET cantidad = l.cantidad_total_factura_compra
		   WHERE barcode_product = l.barcode
		   AND id_compra = id_compra_in;

	    --END IF;

	END LOOP;

RETURN;
END; $$ LANGUAGE plpgsql;


--
-- Actualiza el monto de la venta tomando los valores de su detalle
--
CREATE OR REPLACE FUNCTION update_venta_amount (IN id_venta_in integer)
RETURNS VOID AS $$
BEGIN
	UPDATE venta
	SET monto = (SELECT SUM (precio*cantidad)
                    	    FROM venta_detalle
                    	    WHERE id_venta = id_venta_in)
	WHERE id = id_venta_in;

RETURN;
END; $$ LANGUAGE plpgsql;


---
-- Funcion recursiva
-- Actualiza los costos de las mercaderías compuestas en venta_mc_detalle
-- y de la compuesta en venta_detalle (madre de las anteriores)
-- pertenecientes al id de venta e id de venta detalle especificados.
---
CREATE OR REPLACE FUNCTION update_costo_promedio_detalle_compuesto (IN id_venta_in int4,
       	  	  	   				   	    IN id_venta_detalle_in int4,
							   	    IN id_mh_in int4[],  -- Debe ser ARRAY[0,0] !!!
								    OUT costo_promedio_out double precision,
								    OUT cantidad_out double precision)
RETURNS SETOF record AS $$
DECLARE
  ------------
  q text;
  l record;
  q2 text;
  l2 record;
  ------------
  costo_compuesto_l double precision;
  ------------
  derivada_l int4;  -- id tipo derivado
  corriente_l int4; -- id tipo corriente
  compuesta_l int4; -- id tipo compuesta
  materia_prima_l int4; -- id tipo Materia Prima
  ------------
BEGIN
  SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
  SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';
  SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';
  SELECT id INTO materia_prima_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA';

  -- Si se entra a esta funcion por primera vez
  IF id_mh_in[1] = 0 AND id_mh_in[2] = 0 THEN
     -- Se crea una tabla temporal para priorizar el orden de las mercaderias
     -- Con esto nos aseguraremos de recorrer las compuestas y derivadas antes que mp y corrientes
     CREATE TEMPORARY TABLE prioridad_a (
     	    tipo_id int4,
	    orden int4
     );
     INSERT INTO prioridad_a VALUES (compuesta_l, 1);
     INSERT INTO prioridad_a VALUES (derivada_l, 2);
     INSERT INTO prioridad_a VALUES (materia_prima_l, 3);
     INSERT INTO prioridad_a VALUES (corriente_l, 4);

     -- Se crea una tabla temporal con todos los componentes de la mercadería compuesta a buscar
     CREATE TEMPORARY TABLE arbol_componentes_a AS
     WITH RECURSIVE compuesta (id, id_venta, id_venta_detalle, barcode_complejo, id_mh, tipo_complejo, barcode_componente, tipo_componente,
     	  	    	       cantidad, costo_promedio) AS
       (
         SELECT id, id_venta_vd, id_venta_detalle, barcode_complejo, id_mh, tipo_complejo, barcode_componente, tipo_componente,
	 	cantidad, costo_promedio
       	 FROM venta_mc_detalle
	 WHERE venta_mc_detalle.id_mh[1] = 0
	       AND venta_mc_detalle.id_venta_vd = id_venta_in
	       AND venta_mc_detalle.id_venta_detalle = id_venta_detalle_in
         UNION ALL
         SELECT vmcd.id, vmcd.id_venta_vd, vmcd.id_venta_detalle, vmcd.barcode_complejo, vmcd.id_mh, vmcd.tipo_complejo,
	 	vmcd.barcode_componente, vmcd.tipo_componente, vmcd.cantidad, vmcd.costo_promedio
         FROM venta_mc_detalle vmcd, compuesta
       	 WHERE vmcd.barcode_complejo = compuesta.barcode_componente
	       AND vmcd.id_venta_vd = compuesta.id_venta
	       AND vmcd.id_venta_detalle = compuesta.id_venta_detalle
	       AND vmcd.id_mh[1] = compuesta.id_mh[2]
       )
       SELECT *,
       	      (SELECT valor FROM get_iva (c.barcode_componente))/100 AS iva,
	      (SELECT valor FROM get_otro_impuesto (c.barcode_componente))/100 AS otros
       	      --CASE WHEN tipo_componente != materia_prima_l
	      --     THEN (SELECT valor FROM get_iva (c.barcode_componente))/100
	      --     ELSE (SELECT valor FROM get_iva (c.barcode_complejo))/100
	      --END AS iva,
	      --CASE WHEN tipo_componente != materia_prima_l
	      --     THEN (SELECT valor FROM get_otro_impuesto (c.barcode_componente))/100
	      --     ELSE (SELECT valor FROM get_otro_impuesto (c.barcode_complejo))/100
	      --END AS otros,
       FROM compuesta c;
  END IF;

  q := $S$ SELECT ac.*
	   FROM arbol_componentes_a ac
	   INNER JOIN prioridad_a pd
	   	 ON ac.tipo_componente = pd.tipo_id
	   INNER JOIN producto po
	   	 ON po.barcode = ac.barcode_componente
	   WHERE id_mh[1] = $S$||id_mh_in[2]||$S$
	   ORDER BY pd.orden ASC $S$;

  FOR l IN EXECUTE q LOOP
      -- Si el hijo actual es compuesto (sera la proxima madre)
      IF l.tipo_componente = compuesta_l OR l.tipo_componente = derivada_l THEN
      	-- Se consulta así mismo y calcula el costo_promedio unitario del compuesto
      	q2 := 'SELECT * FROM update_costo_promedio_detalle_compuesto ('||id_venta_in||',
							              '||id_venta_detalle_in||',
								       ARRAY['||l.id_mh[1]||','||l.id_mh[2]||']::int4[])';

	costo_compuesto_l := 0;
	FOR l2 IN EXECUTE q2 LOOP
	    costo_compuesto_l := costo_compuesto_l + (l2.costo_promedio_out * l2.cantidad_out);
	END LOOP;

	costo_compuesto_l := costo_compuesto_l / l.cantidad;

	--RAISE NOTICE 'v_id: %, vd_id: %, costo: %, id_mh: %', id_venta_in, id_venta_detalle_in, costo_compuesto_l, l.id_mh;

	UPDATE venta_mc_detalle
	       SET costo_promedio = costo_compuesto_l
	WHERE id = l.id;

	costo_promedio_out := costo_compuesto_l;
	cantidad_out := l.cantidad;

      ELSIF l.tipo_componente = corriente_l OR l.tipo_componente = materia_prima_l THEN
        costo_promedio_out := l.costo_promedio;
	cantidad_out := l.cantidad;
      END IF;

      RETURN NEXT;
  END LOOP;

  IF id_mh_in[1] = 0 AND id_mh_in[2] = 0 THEN
     -- Se eliminan las tablas temporales 'arbol_componentes_a' y 'prioridad_a'
     DROP TABLE arbol_componentes_a;
     DROP TABLE prioridad_a;

     -- Se actualiza el costo promedio (unitario) del compuesto en venta detalle
     UPDATE venta_detalle
	    SET fifo = ((SELECT SUM (costo_promedio * cantidad)
	     	         FROM venta_mc_detalle
		         WHERE id_venta_vd = id_venta_in
		      	       AND id_venta_detalle = id_venta_detalle_in
		     	       AND id_mh[1] = 0) / cantidad)
     WHERE id_venta = id_venta_in
     AND id = id_venta_detalle_in;
  END IF;

RETURN;
END; $$ LANGUAGE plpgsql;


---
-- Funcion recursiva
-- Actualiza las ganancias de los componentes y el compuesto de la venta especificada
---
CREATE OR REPLACE FUNCTION update_profits_detalle_compuesto (IN id_venta_in int4,
       	  	  	   		      		     IN id_venta_detalle_in int4,
							     IN id_mh_in int4[], -- Debe ser ARRAY[0,0] !!!
							     IN precio_madre_in double precision, --Se llama con 0
							     IN costo_madre_in double precision, --Se llama con 0
							     OUT barcode_out varchar,
							     OUT costo_out double precision,
							     OUT precio_out double precision,
							     OUT cantidad_out double precision,
							     OUT ganancia_out double precision,
							     OUT iva_out double precision,
							     OUT otros_out double precision,
							     OUT iva_residual_out double precision,
							     OUT otros_residual_out double precision)
RETURNS SETOF record AS $$
DECLARE
  ------------
  q text;
  l record;
  q2 text;
  l2 record;
  ------------
  precio_madre_l double precision;
  costo_madre_l double precision;
  ------------
  derivada_l int4;  -- id tipo derivado
  corriente_l int4; -- id tipo corriente
  compuesta_l int4; -- id tipo compuesta
  materia_prima_l int4; -- id tipo Materia Prima
  ------------
BEGIN
  SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
  SELECT id INTO corriente_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'CORRIENTE';
  SELECT id INTO derivada_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'DERIVADA';
  SELECT id INTO materia_prima_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'MATERIA PRIMA';

  -- Si se entra a esta funcion por primera vez
  IF id_mh_in[1] = 0 AND id_mh_in[2] = 0 THEN
     -- Se actualizan los costos de todas las mercaderías
     PERFORM update_costo_promedio_detalle_compuesto (id_venta_in, id_venta_detalle_in, id_mh_in);
     -- Se crea una tabla temporal para priorizar el orden de las mercaderias
     -- Con esto nos aseguraremos de recorrer las compuestas y derivadas antes que mp y corrientes
     CREATE TEMPORARY TABLE prioridad_b (
     	    tipo_id int4,
	    orden int4
     );
     INSERT INTO prioridad_b VALUES (compuesta_l, 1);
     INSERT INTO prioridad_b VALUES (derivada_l, 2);
     INSERT INTO prioridad_b VALUES (materia_prima_l, 3);
     INSERT INTO prioridad_b VALUES (corriente_l, 4);

     -- Se crea una tabla temporal con todos los componentes de la mercadería compuesta a buscar
     CREATE TEMPORARY TABLE arbol_componentes_b AS
     WITH RECURSIVE compuesta (id, id_venta, id_venta_detalle, barcode_complejo, id_mh, tipo_complejo, barcode_componente, tipo_componente,
     	  	    	       cantidad, costo_promedio, proporcion_iva, proporcion_otros) AS
       (
         SELECT id, id_venta_vd, id_venta_detalle, barcode_complejo, id_mh, tipo_complejo, barcode_componente, tipo_componente,
	 	cantidad, costo_promedio, proporcion_iva, proporcion_otros
       	 FROM venta_mc_detalle
	 WHERE venta_mc_detalle.id_mh[1] = 0
	       AND venta_mc_detalle.id_venta_vd = id_venta_in
	       AND venta_mc_detalle.id_venta_detalle = id_venta_detalle_in
         UNION ALL
         SELECT vmcd.id, vmcd.id_venta_vd, vmcd.id_venta_detalle, vmcd.barcode_complejo, vmcd.id_mh, vmcd.tipo_complejo,
	 	vmcd.barcode_componente, vmcd.tipo_componente, vmcd.cantidad, vmcd.costo_promedio, vmcd.proporcion_iva, vmcd.proporcion_otros
         FROM venta_mc_detalle vmcd, compuesta
       	 WHERE vmcd.barcode_complejo = compuesta.barcode_componente
	       AND vmcd.id_venta_vd = compuesta.id_venta
	       AND vmcd.id_venta_detalle = compuesta.id_venta_detalle
	       AND vmcd.id_mh[1] = compuesta.id_mh[2]
       )
       SELECT *,
       	      (SELECT valor FROM get_iva (c.barcode_componente))/100 AS iva_percent,
	      (SELECT valor FROM get_otro_impuesto (c.barcode_componente))/100 AS otros_percent
       FROM compuesta c;
  END IF;

  q := $S$ SELECT ac.*
	   FROM arbol_componentes_b ac
	   INNER JOIN prioridad_b pd
	   	 ON ac.tipo_componente = pd.tipo_id
	   INNER JOIN producto po
	   	 ON po.barcode = ac.barcode_componente
	   WHERE id_mh[1] = $S$||id_mh_in[2]||$S$
	   ORDER BY pd.orden ASC $S$;

  -- Se obtiene el precio de la madre
  IF id_mh_in[1] = 0 AND id_mh_in[2] = 0 THEN
     costo_madre_l := (SELECT fifo FROM venta_detalle WHERE id = id_venta_detalle_in AND id_venta = id_venta_in);
     precio_madre_l := (SELECT precio FROM venta_detalle WHERE id = id_venta_detalle_in AND id_venta = id_venta_in);
  ELSE
     costo_madre_l := costo_madre_in;
     precio_madre_l := precio_madre_in;
  END IF;

  -- Se recorre el arbol de componentes
  FOR l IN EXECUTE q LOOP
      -- Si el hijo actual es compuesto (sera la proxima madre)
      IF l.tipo_componente = compuesta_l OR l.tipo_componente = derivada_l THEN
	-- precio proporcional =  (proporcion del COSTO hijo con respecto a madre) * PRECIO madre
        precio_out := (l.costo_promedio / costo_madre_l) * precio_madre_l;

      	-- Se consulta así mismo
      	q2 := 'SELECT * FROM update_profits_detalle_compuesto ('||id_venta_in||',
	      	  	   				       '||id_venta_detalle_in||',
							       ARRAY['||l.id_mh[1]||','||l.id_mh[2]||']::int4[],
							       '||precio_out||',
							       '||l.costo_promedio||')';
	  ganancia_out := 0;
	  iva_out := 0;
	  otros_out := 0;
	  iva_residual_out := 0;
	  otros_residual_out := 0;
	FOR l2 IN EXECUTE q2 LOOP
	  ganancia_out := ganancia_out + l2.ganancia_out;
	  iva_out := iva_out + l2.iva_out;
	  otros_out := otros_out + l2.otros_out;
	  iva_residual_out := iva_residual_out + l2.iva_residual_out;
	  otros_residual_out := otros_residual_out + l2.otros_residual_out;
	END LOOP;

	barcode_out := l.barcode_componente;
	costo_out := l.costo_promedio;
	cantidad_out := l.cantidad;

      ELSIF l.tipo_componente = corriente_l OR l.tipo_componente = materia_prima_l THEN

      	-- precio proporcional =  (proporcion del COSTO hijo con respecto a madre) * PRECIO madre
        precio_out := (l.costo_promedio / costo_madre_l) * precio_madre_l;

        -- ganancia total =               precio venta neto               - costo             *  cantidad
        ganancia_out := ((precio_out / (l.iva_percent + l.otros_percent + 1)) - l.costo_promedio) * l.cantidad;

	-- Impuesto =            precio venta neto                     * impuesto         *  cantidad
	iva_out :=   ((precio_out / (l.iva_percent + l.otros_percent + 1)) * l.iva_percent)   * l.cantidad;
	otros_out := ((precio_out / (l.iva_percent + l.otros_percent + 1)) * l.otros_percent) * l.cantidad;

	iva_residual_out := iva_out * l.proporcion_iva;
	otros_residual_out := otros_out * l.proporcion_otros;

	barcode_out := l.barcode_componente;
	costo_out := l.costo_promedio;
	cantidad_out := l.cantidad;
      END IF;

      --RAISE NOTICE 'id: % gan: %, pp: %, iva: %, otros: %, iva_r: %, otros_r: %', l.id, ganancia_out, precio_out, iva_out, otros_out,
      --	    	   	    	   	   	     	       		      	  iva_residual_out, otros_residual_out;

      -- Se actualiza la ganancia, precio prop, iva, otros
      UPDATE venta_mc_detalle
       	     SET ganancia = ganancia_out,
	      	 precio_proporcional = precio_out,
	     	 iva = iva_out,
	     	 otros = otros_out,
		 iva_residual = iva_residual_out,
		 otros_residual = otros_residual_out
      WHERE id = l.id;

      RETURN NEXT;
  END LOOP;

  -- Una vez finalizado todo
  IF id_mh_in[1] = 0 AND id_mh_in[2] = 0 THEN
      -- Se actualizan los datos del compuesto madre en venta_detalle
      -- El costo se actualizo desde un inicio con la funcion 'PERFORM update_costo_promedio_detalle_compuesto'
      -- El costo
      -- UPDATE venta_detalle
      -- 	 SET fifo = (SELECT SUM (costo_promedio * cantidad)
      -- 	     	     FROM venta_mc_detalle
      -- 		     WHERE id_venta_vd = id_venta_in
      -- 		     AND id_venta_detalle = id_venta_detalle_in
      -- 		     AND id_mh[1] = 0)
      -- WHERE id_venta = id_venta_in
      -- AND id = id_venta_detalle_in;

      -- Se actualiza el iva, otros, ganancia
      UPDATE venta_detalle
	 SET iva = vmcd.sum_iva,
	     otros = vmcd.sum_otros,
	     iva_residual = vmcd.sum_iva_residual,
	     otros_residual = vmcd.sum_otros_residual,
	     ganancia = vmcd.sum_ganancia
	 FROM
	 (
	   SELECT SUM (iva) AS sum_iva,
	   	  SUM (otros) AS sum_otros,
		  SUM (iva_residual) AS sum_iva_residual,
	   	  SUM (otros_residual) AS sum_otros_residual,
		  SUM (ganancia) AS sum_ganancia
	   FROM venta_mc_detalle
	   WHERE id_venta_vd = id_venta_in
	   	 AND id_venta_detalle = id_venta_detalle_in
		 AND id_mh[1] = 0
         ) vmcd
      WHERE id_venta = id_venta_in
      AND id = id_venta_detalle_in;

     -- Se eliminan las tablas temporales 'arbol_componentes' y 'prioridad'
     DROP TABLE arbol_componentes_b;
     DROP TABLE prioridad_b;
  END IF;

RETURN;
END; $$ LANGUAGE plpgsql;


---
-- Actualiza las ganancias dentro del rango de fecha estipulado
-- NOTA: esta funcion crea una tabla 'componente_en_venta' (se debe borrar una vez usada)
-- con los id de las ventas e id ventas detalles donde participa el producto especificado
-- (dentro de venta_mc_detalle).
---
CREATE OR REPLACE FUNCTION update_profits_on_date_range (IN fecha_inicio timestamp,
       	  	  	   		      		 IN fecha_termino timestamp,
							 IN barcode varchar,
							 OUT barcode_out varchar,
							 OUT costo_out double precision,
							 OUT precio_out double precision,
							 OUT cantidad_out double precision,
							 OUT ganancia_out double precision)
RETURNS SETOF record AS $$

DECLARE
	------------
	q text;
	l record;
	iva_percent double precision;
	otros_percent double precision;
	------------
	costo_padre_l double precision;
	precio_padre_l double precision;
	precio_proporcional_l double precision;
	------------
BEGIN
	-- OBTIENE LAS VENTAS (EN venta_detalle) DEL PRODUCTO DETERMINADO EN EL RANGO DE FECHA ESPECIFICADO
	q := $S$ SELECT v.id AS v_id, vd.id AS vd_id,
	     	 	vd.barcode, vd.fifo AS costo, vd.precio, vd.cantidad, v.fecha
		 FROM venta_detalle vd
		 INNER JOIN venta v
		 ON vd.id_venta = v.id
		 WHERE fecha > $S$ || quote_literal (fecha_inicio) || $S$
		 AND fecha < $S$ || quote_literal (fecha_termino) || $S$
		 AND vd.barcode = $S$ || barcode;

        -- OBTIENE LOS IMPUESTOS
        SELECT COALESCE(valor,0) INTO iva_percent FROM get_iva (barcode::BIGINT);
        SELECT COALESCE(valor,0) INTO otros_percent FROM get_otro_impuesto (barcode::BIGINT);

	-- IMPUESTOS
	IF iva_percent = 0 THEN
	   iva_percent := 0;
	ELSE
	   iva_percent := iva_percent / 100;
	END IF;

        IF otros_percent = 0 THEN
	   otros_percent := 0;
	ELSE
	   otros_percent := otros_percent / 100;
	END IF;

	-- RECORRE EL DETALLE DE LA VENTA (venta_detalle)
	FOR l IN EXECUTE q LOOP
                           --   (precio venta neto)                        - costo    * cantidad = ganancia total
           ganancia_out = ( (l.precio / (iva_percent + otros_percent + 1)) - l.costo ) * l.cantidad;

            -- ACTUALIZA LA GANANCIA
	    UPDATE venta_detalle
	    	   SET ganancia = ganancia_out
	    	   WHERE id = l.vd_id
	    	   AND id_venta = l.v_id;

	    barcode_out := l.barcode;
	    costo_out := l.costo;
	    precio_out := l.precio;
	    cantidad_out := l.cantidad;
	    RETURN NEXT;
	END LOOP;
RETURN;
END; $$ LANGUAGE plpgsql;


---
-- Recalcula el costo promedio en las compras de un producto
-- desde el id de compra especificado en adelante
-- (Si es 0 recalcula todas las compras)
CREATE OR REPLACE FUNCTION update_avg_cost (IN codigo_barras bigint,
	                                    IN id_fcompra integer,
					    OUT id_fcompra_r integer,
					    OUT barcode_r bigint,
					    OUT stock_r double precision,
                                            OUT avg_cost_anterior_r double precision,
					    OUT cantidad_ingresada_r double precision,
					    OUT costo_compra_r double precision,
					    OUT avg_cost_new_r double precision)
RETURNS SETOF RECORD AS $$

DECLARE
    -----------
    q text;
    q2 text;
    q3 text;
    l record;
    l2 record;
    -----------
    iva_local double precision;
    otros_local double precision;
    iva_percent double precision;
    otros_percent double precision;
    -----------
    avg_cost double precision;
    cantidad_traspaso double precision;
    new_stock double precision;
    -----------
    compuesta_l int4;
    -----------
    precio_proporcional_l double precision;
    costo_total_venta_l double precision;
    monto_venta_l integer;
    next_date_l timestamp;
    id_ventas_l int4[];
    -----------
BEGIN
    -- Se obtiene el id del tipo compuesto
    SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
    -- Se crea una tabla temporal donde se registraran todos los id_venta e id_venta_detalle
    -- relacionado con el detalle del compuesto donde se encuentre este producto
    CREATE TEMPORARY TABLE componente_en_venta (v_id int4, vd_id int4, descuento int2);
    -- Se inicializa con un valor
    avg_cost := 0;
    id_ventas_l := ARRAY[0];

    iva_percent := get_iva (codigo_barras);
    otros_percent := get_otro_impuesto (codigo_barras);

    -- IVA
    IF iva_percent = -1 OR iva_percent = 0 OR iva_percent IS NULL THEN
       iva_percent := 0;
    ELSE
       iva_percent := iva_percent / 100;
    END IF;

    -- OTROS
    IF otros_percent = -1 OR otros_percent = 0 OR otros_percent IS NULL THEN
       otros_percent := 0;
    ELSE
       otros_percent := otros_percent / 100;
    END IF;

    -- Se obtiene la información del producto y de la compra en que se adquirió
    q := $S$ SELECT fcd.id AS id_fcd, fc.id AS id_fc, fc.id_compra AS id_c,
      	     	    fcd.barcode, fcd.precio, fcd.cantidad, fc.fecha, fcd.costo_promedio,

                    -- Se obtiene el stock que había de ese producto antes de comprarlo
                    COALESCE ((SELECT cantidad_fecha
			              FROM producto_en_fecha (fc.fecha, fcd.barcode))
				      ,0) AS stock,

	            -- Se obtiene la fecha de la compra siguiente
		    COALESCE ((SELECT fecha
			              FROM factura_compra
			              WHERE id = (SELECT MIN (fcdi.id_factura_compra)
						         FROM factura_compra_detalle fcdi
							 INNER JOIN factura_compra fci
							       ON fci.id = fcdi.id_factura_compra
							 INNER JOIN compra ci
							       on ci.id = fci.id_compra
							 WHERE fcdi.id_factura_compra > fc.id
							 AND ci.anulada_pi = false
							 AND fcdi.barcode = fcd.barcode)),NULL) AS next_date

                    FROM factura_compra_detalle fcd
			  INNER JOIN factura_compra fc
			  ON fc.id = fcd.id_factura_compra
			  INNER JOIN compra c
			  ON c.id = fc.id_compra
                    WHERE c.anulada_pi = FALSE
			  AND fcd.barcode = $S$ || codigo_barras || $S$
                          AND fc.id >= $S$ || id_fcompra || $S$
                    ORDER BY fc.id, fc.fecha ASC $S$;

    FOR l IN EXECUTE q LOOP
        -- Se obtiene el costo promedio de la compra anterior
        avg_cost := (SELECT costo_promedio
	             FROM factura_compra_detalle fcde
	             WHERE fcde.id_factura_compra = (SELECT MAX (fcd.id_factura_compra)
						     FROM factura_compra_detalle fcd
						     INNER JOIN factura_compra fc
						    	   ON fc.id = fcd.id_factura_compra
						     INNER JOIN compra c
						    	   ON c.id = fc.id_compra
                                                     WHERE fcd.id_factura_compra < l.id_fc
						    	   AND c.anulada_pi = FALSE
                                                    	   AND fcd.barcode = l.barcode)
	             AND fcde.barcode = l.barcode);

	avg_cost := COALESCE (avg_cost,0);

	-- Si no hay una compra anterior, se busca un traspaso que le haya dado un costo
	-- (traspaso de un producto sin compra previa)
	IF avg_cost = 0 THEN
	   SELECT INTO avg_cost, cantidad_traspaso
		       td.precio, td.cantidad
	   FROM
	    (
	      SELECT td.precio, td.cantidad, t.fecha
	      FROM traspaso_detalle td
	      INNER JOIN traspaso t
	      	    ON t.id = td.id_traspaso
	      WHERE costo_modificado = TRUE
	      	    AND barcode = l.barcode
	    ) td;

	   avg_cost := COALESCE (avg_cost,0);
	   cantidad_traspaso := COALESCE (cantidad_traspaso,0);
	   avg_cost_anterior_r = avg_cost;
	   avg_cost = ((avg_cost_anterior_r*cantidad_traspaso) + (l.precio*l.cantidad)) / (l.cantidad+l.stock);
           avg_cost = ROUND (avg_cost::NUMERIC, 3);
	ELSE
	  avg_cost := COALESCE (avg_cost,0);
	  avg_cost_anterior_r = avg_cost;
          avg_cost = ((avg_cost_anterior_r*l.stock) + (l.precio*l.cantidad)) / (l.cantidad+l.stock);
          avg_cost = ROUND (avg_cost::NUMERIC, 3);
	END IF;

	iva_local = (l.precio * l.cantidad) * iva_percent;
	otros_local = (l.precio * l.cantidad) * otros_percent;

	-- Actualiza el costo promedio y los impuestos en factura_compra_detalle
	UPDATE factura_compra_detalle
               SET costo_promedio = avg_cost,
	       	   iva = iva_local,
	           otros = otros_local
        WHERE id = l.id_fcd;

	-- Recalcular monto total factura_compra
	PERFORM update_factura_compra_amount (l.id_fc);

	-- Actualiza las cantidades ingresadas en compra_detalle
	PERFORM update_compra_detalle_amount (l.id_c);

	-- Actualiza costo promedio en venta_detalle
	q2 := $S$ UPDATE venta_detalle vd
	       	  	 SET fifo = $S$ || avg_cost || $S$
	       	  FROM venta v
	      	  WHERE v.id = vd.id_venta
	       	   	AND vd.barcode = $S$ || l.barcode || $S$
	       	  	AND v.fecha > $S$ || quote_literal (l.fecha); --fecha de la compra donde se cambio el costo_promedio

	-- Actualiza costo promedio en venta_mc_detalle
	q3 := $S$ UPDATE venta_mc_detalle vmcd
	       	  	 SET costo_promedio = $S$ || avg_cost || $S$
	       	  FROM venta v
	      	  WHERE v.id = vmcd.id_venta_vd
	       	  	AND vmcd.barcode_componente = $S$ || l.barcode || $S$
	       	  	AND v.fecha > $S$ || quote_literal (l.fecha); --fecha de la compra donde se cambio el costo_promedio

	-- Si next-date es null
	IF l.next_date IS NULL THEN
	   next_date_l := NOW()::timestamp;
	ELSE
	   next_date_l := l.next_date::timestamp;
	   q2 := q2 || $S$ AND v.fecha < $S$ || quote_literal (l.next_date); --fecha de la proxima compra de ese producto
	   q3 := q3|| $S$ AND v.fecha < $S$ || quote_literal (l.next_date); --fecha de la proxima compra de ese producto
	END IF;

	EXECUTE q2; -- Update venta_detalle
	EXECUTE q3; -- Update venta_mc_detalle

   	-- RECALCULA GANANCIAS DENTRO DEL RANGO DE FECHAS
	PERFORM update_profits_on_date_range (quote_literal(l.fecha)::TIMESTAMP,
	   				      quote_literal(next_date_l)::TIMESTAMP,
					      l.barcode::VARCHAR);

        -- Registra las ventas indirectas del producto en el rango de fecha especificado
	INSERT INTO componente_en_venta (v_id, vd_id, descuento)
	SELECT v.id AS v_id, vmcd.id_venta_detalle AS vd_id, v.descuento AS descuento
	FROM venta_mc_detalle vmcd
	     INNER JOIN venta v
	     ON vmcd.id_venta_vd = v.id
	WHERE v.fecha > l.fecha::TIMESTAMP
	      AND v.fecha < next_date_l::TIMESTAMP
	      AND vmcd.barcode_componente = l.barcode::BIGINT;

        -- Se asignan los valores a retornar
        id_fcompra_r := l.id_fc;
	barcode_r := l.barcode;
	stock_r := l.stock;
        avg_cost_new_r := avg_cost;
	cantidad_ingresada_r := l.cantidad;
	costo_compra_r := l.precio;
        RETURN NEXT;
    END LOOP;

    -- Se actualizan los costos, ganancias, precio proporcional e impuestos en venta_mc_detalle
    -- a partir de los id_venta e id_venta_detalle registrados en la tabla 'componente_en_venta'
    q := $S$ SELECT v_id, vd_id, descuento
      	     FROM componente_en_venta
	     GROUP BY v_id, vd_id, descuento $S$;

    FOR l IN EXECUTE q LOOP
    	-- Si esa venta tiene un descuento se debe recalcular el precio proporcional del compuesto
	IF l.descuento > 0 AND NOT (id_ventas_l @> ARRAY[l.v_id]) THEN
	   --array_append (id_ventas_l, l.v_id); -- Se agrega el id de venta al array de id de ventas
	   id_ventas_l := id_ventas_l || l.v_id;

	   -- Se seleccionan todos los productos compuestos que participen de esta venta y que tengan
	   -- un componente cuyo costo se haya modificado
	   q2 := $S$ SELECT id AS vd_id, id_venta AS v_id
	      	     FROM venta_detalle
		     WHERE id_venta = $S$||l.v_id||$S$
		     AND id IN (SELECT vd_id
		     	       	FROM componente_en_venta
				WHERE v_id = $S$||l.v_id||$S$)
		     AND tipo = $S$||compuesta_l;

	   FOR l2 IN EXECUTE q2 LOOP
	       -- Se actualiza el costo del compuesto
	       PERFORM update_costo_promedio_detalle_compuesto (l2.v_id, l2.vd_id, ARRAY[0,0]);
	   END LOOP;

	   -- Se obtiene el monto total de la venta
	   monto_venta_l := (SELECT monto FROM venta WHERE id = l.v_id);
	   -- Se actualizan los precios proporcionales de las mercaderías en esta venta
	   costo_total_venta_l := (SELECT SUM(fifo) FROM venta_detalle WHERE id_venta = l.v_id);

	   -- Se seleccionan todos los productos esta venta para actualizar el precio proporcional, impuestos y ganancias
	   q2 := $S$ SELECT id AS id_vd, id_venta AS id_v, barcode, fifo, tipo, proporcion_iva, proporcion_otros,
	      	     	    (SELECT valor FROM get_iva (barcode))/100 AS iva_percent,
		     	    (SELECT valor FROM get_otro_impuesto (barcode))/100 AS otros_percent
	      	     FROM venta_detalle
		     WHERE id_venta = $S$||l.v_id;

	   FOR l2 IN EXECUTE q2 LOOP
	       precio_proporcional_l := (l2.fifo/costo_total_venta_l) * monto_venta_l;

	       UPDATE venta_detalle
	              SET precio = precio_proporcional_l
	       WHERE id = l.vd_id
	       AND id_venta = l.v_id;

	       -- Se calculan y actualizan los impuestos y ganancias de las mercaderias que nos son compuestas
	       IF l2.tipo != compuesta_l THEN
	       	  -- Actualiza el iva, otros, ganancia
		  UPDATE venta_detalle
	              	 SET iva = ((precio_proporcional_l / (l2.iva_percent+l2.otros_percent+1)) * l2.iva_percent),
		      	     otros = ((precio_proporcional_l / (l2.iva_percent+l2.otros_percent+1)) * l2.otros_percent),
			     iva_residual = iva * l2.proporcion_iva,
		      	     otros_residual = otros * l2.proporcion_otros,
			     ganancia = ((precio_proporcional_l / (l2.iva_percent+l2.otros_percent+1)) - l2.fifo)
	       	  WHERE id = l.vd_id
	       	  	AND id_venta = l.v_id;
	       END IF;

	   END LOOP;
	END IF;

	PERFORM update_profits_detalle_compuesto (l.v_id::int4, l.vd_id::int4, ARRAY[0,0]::int4[], 0::double precision, 0::double precision);
    END LOOP;
    -- Se dropea la tabla temporal componente_en_venta
    DROP TABLE componente_en_venta;

    IF avg_cost >= 0 THEN
       UPDATE producto
       	      SET costo_promedio = avg_cost
	      WHERE barcode = codigo_barras;
    END IF;

    SELECT cantidad_fecha INTO new_stock
    FROM producto_en_fecha (now()::TIMESTAMP + '1 seconds', codigo_barras);

    IF new_stock >= 0 THEN
       UPDATE producto
      	      SET stock = new_stock
      	      WHERE barcode = codigo_barras;
    END IF;

    RETURN;
END; $$ LANGUAGE plpgsql;


--
-- Información de las compras en las que ha estado el producto especificado
--
CREATE OR REPLACE FUNCTION product_on_ingress (IN barcode_in bigint,
       	  	  	   		       OUT id_fc_out integer,
					       OUT id_fcd_out integer,
					       OUT id_t_out integer,
					       OUT id_td_out integer,
					       OUT fecha_out timestamp,
					       OUT barcode_out bigint,
					       OUT cantidad_pre_ingreso double precision,
					       OUT cantidad_ingresada double precision)
RETURNS SETOF RECORD AS $$

DECLARE
    q text;
    l record;

BEGIN

    CREATE TEMPORARY TABLE ingreso_producto AS
    SELECT fc.id AS id_fc, fcd.id AS id_fcd, 0 AS id_t, 0 AS id_td, fecha, barcode, cantidad,
      	   (SELECT cantidad_fecha FROM producto_en_fecha (fecha, fcd.barcode)) AS cantidad_pre
    FROM factura_compra fc
    INNER JOIN factura_compra_detalle fcd
    	  ON fc.id = fcd.id_factura_compra
    WHERE barcode = barcode_in
    ORDER BY id_fc ASC;

    INSERT INTO ingreso_producto
    SELECT 0 AS id_fc, 0 AS id_fcd, t.id AS id_t, td.id AS id_td, fecha, barcode, cantidad,
      	   (SELECT cantidad_fecha FROM producto_en_fecha (fecha, td.barcode)) AS cantidad_pre
    FROM traspaso t
    INNER JOIN traspaso_detalle td
    	  ON t.id = td.id_traspaso
    WHERE barcode = barcode_in
    AND origen != 1 --Traspasos recibidos
    ORDER BY id_t ASC;

    q := $S$ SELECT * FROM ingreso_producto ORDER BY fecha ASC $S$;

    FOR l IN EXECUTE q LOOP
    	id_fc_out := l.id_fc;
	id_fcd_out := l.id_fcd;
	id_t_out := l.id_t;
	id_td_out := l.id_td;
	fecha_out := l.fecha;
	barcode_out := l.barcode;
	cantidad_pre_ingreso := l.cantidad_pre;
	cantidad_ingresada := l.cantidad;
        RETURN NEXT;
    END LOOP;

    --Retorno Final
    id_fc_out := 0;
    id_fcd_out := 0;
    id_t_out := 0;
    id_td_out := 0;
    fecha_out := now();
    barcode_out := barcode_in;
    cantidad_pre_ingreso := (SELECT cantidad_fecha
      	                     FROM producto_en_fecha (now()::TIMESTAMP, barcode_in));
    cantidad_ingresada := 0;
    RETURN NEXT;

    DROP TABLE ingreso_producto;

    RETURN;
END; $$ LANGUAGE plpgsql;


--
-- Anula una compra y devuelve el resultado
--
create or replace function nullify_buy (IN id_compra_in integer,
       	  	  	   	        IN maquina_in integer,
       					OUT barcode_out varchar,
       					OUT cantidad_out double precision,
       					OUT cantidad_anulada_out double precision,
       					OUT nuevo_stock_out double precision,
       					OUT costo_out double precision,
       					OUT precio_out double precision)
RETURNS setof record AS $$
DECLARE
   id_nc integer;
   id_c_anulada integer;
   id_fc_anterior integer;
   stock_actual double precision;

   query text;
   l record;
BEGIN

   -- Se inserta los datos de la compra en la tabla compra_anulada TODO: que guarde el id del usuario
   EXECUTE 'INSERT INTO compra_anulada (id_compra, id_factura_compra, fecha_anulacion, rut_proveedor, maquina)
               	   SELECT id_compra, id, NOW(), rut_proveedor, '||maquina_in||'
	 	          FROM factura_compra
	 	      	  WHERE id_compra = '|| id_compra_in;

   -- Se ingresa una nota de credito si la factura esta pagada
   EXECUTE $S$ INSERT INTO nota_credito (fecha, id_factura_compra, fecha_factura, rut_proveedor)
	   	      SELECT NOW(), id, fecha, rut_proveedor
	 	      	     FROM factura_compra
	 	      	     WHERE pagada = true
	 	      	     AND id_compra = $S$|| id_compra_in;

   -- Obtengo todos los productos correspondiente a la compra
   query := $S$ SELECT fc.id AS id_fc, fc.pagada, fcd.barcode,
   	    	       fcd.precio AS costo, fcd.cantidad,
              	       (SELECT precio
		      	       FROM producto
			       WHERE barcode = fcd.barcode) AS precio
      	     	FROM factura_compra_detalle fcd
	  	     INNER JOIN factura_compra fc
	  	     ON fcd.id_factura_compra = fc.id
	        WHERE fc.id_compra = $S$ || id_compra_in;

   FOR l IN EXECUTE query loop
       -- Se Inicializan las variables
       id_nc := 0;
       -- Registrando datos a retornar
       barcode_out := l.barcode;
       costo_out := l.costo;
       precio_out := l.precio;
       cantidad_out := l.cantidad;

       -- Obtengo el stock actual de producto
       SELECT stock INTO stock_actual FROM producto WHERE producto.barcode = l.barcode;

       -- La cantidad a anular no puede ser mayor al stock actual
       IF stock_actual < l.cantidad OR stock_actual = 0 THEN
          cantidad_anulada_out := stock_actual;
       	  nuevo_stock_out := 0;
       ELSE
          cantidad_anulada_out := l.cantidad;
          nuevo_stock_out := stock_actual - l.cantidad;
       END IF;

       -- Reajustar stock producto (De esto se encargará update_avg_cost)
       --EXECUTE 'UPDATE producto SET stock = '||nuevo_stock_out||' WHERE barcode = '||l.barcode;
       RAISE NOTICE 'El stock de %, es: %', l.barcode, nuevo_stock_out;

       -- Si se anuló una factura pagada se registra la información correspondiente en nota_credito
       IF l.pagada = true THEN
       	  SELECT id INTO id_nc FROM nota_credito WHERE id_factura_compra = l.id_fc;
          -- Se ingresa el detalle de nota de credito si existe (nota credito se crea cuando se anula una factura pagada)
	  IF cantidad_anulada_out > 0 AND (id_nc > 0 AND id_nc IS NOT NULL) THEN
   	     EXECUTE 'INSERT INTO nota_credito_detalle (id_nota_credito, barcode, costo, precio, cantidad)
	  	      VALUES ('||id_nc||','||l.barcode||','||l.costo||','||l.precio||','||cantidad_anulada_out||')';
          END IF;
       END IF;

       -- Ingresa los productos en la tabla compra_anulada_detalle
       IF cantidad_anulada_out > 0 THEN
       	  SELECT id INTO id_c_anulada FROM compra_anulada WHERE id_factura_compra = l.id_fc;
	  IF id_c_anulada > 0 AND id_c_anulada IS NOT NULL THEN
       	     EXECUTE 'INSERT INTO compra_anulada_detalle (id_compra_anulada, barcode, costo, precio, cantidad, cantidad_anulada)
                      VALUES ('||id_c_anulada||','||l.barcode||','||l.costo||','||l.precio||','||l.cantidad||','||cantidad_anulada_out||')';
          END IF;
       END IF;

       RETURN NEXT;
   END loop;

   --Actualizar anulada_pi en compra
   UPDATE compra SET anulada_pi = 't' WHERE id = id_compra_in;

   --Recalcular costo_promedio
   query := $S$ SELECT fc.id AS id_fc, fcd.barcode
       	     	       FROM factura_compra fc
		       INNER JOIN factura_compra_detalle fcd
      		       	     ON fc.id = fcd.id_factura_compra
      		       WHERE fc.id_compra = $S$ || id_compra_in;

   FOR l IN EXECUTE query loop
       -- Obtiene el id de la última factura anterior a esta compra
       -- en donde estaba este producto
       SELECT COALESCE (MAX (fcd.id_factura_compra), 0) INTO id_fc_anterior
       FROM factura_compra_detalle fcd
            INNER JOIN factura_compra fc
	          ON fc.id = fcd.id_factura_compra
	    INNER JOIN compra c
	          ON c.id = fc.id_compra
       WHERE fcd.id_factura_compra < l.id_fc
       AND c.anulada_pi = FALSE
       AND fcd.barcode = l.barcode;

       PERFORM update_avg_cost (l.barcode, id_fc_anterior);
       RAISE NOTICE 'Se ejecuta update_avg_cost (barcode: %, id_fc: %)', l.barcode, id_fc_anterior;
   END LOOP;

RETURN;
END; $$ language plpgsql;

--
-- function to help info_abonos
--
CREATE OR REPLACE FUNCTION select_abonos (IN in_rut_cliente int4,
					  OUT o_id integer,
					  OUT o_rut_cliente int4,
					  OUT o_monto_abonado integer,
					  OUT o_fecha_abono timestamp)
RETURNS SETOF RECORD AS $$
DECLARE
   l record;
   q varchar;

BEGIN
   q := $S$ SELECT id, rut_cliente, monto_abonado, fecha_abono
    	    FROM abono
	    WHERE rut_cliente = $S$ || in_rut_cliente || $S$
	    ORDER BY fecha_abono ASC $S$;

   FOR l IN EXECUTE q LOOP
       o_id := l.id;
       o_rut_cliente := l.rut_cliente;
       o_monto_abonado := l.monto_abonado;
       o_fecha_abono := l.fecha_abono;
       RETURN NEXT;
   END LOOP;

   o_id := 0;
   o_rut_cliente := l.rut_cliente;
   o_monto_abonado := 0;
   o_fecha_abono := now();
   RETURN NEXT;

RETURN;
END; $$ LANGUAGE plpgsql;


--
-- Busca todas las deudas y abonos de un cliente específico
--
CREATE OR REPLACE FUNCTION info_abonos (IN in_rut_cliente int4,
					OUT out_fecha timestamp,
					OUT out_id_venta integer,
					OUT out_monto_deuda integer,
					OUT out_abono integer,
					OUT out_deuda_total integer)
RETURNS SETOF RECORD AS $$
DECLARE
   l1 record;
   q1 varchar;
   l2 record;
   q2 varchar;
   deuda_total integer;
   fecha_abono_anterior timestamp;

BEGIN

   deuda_total := 0;
   out_deuda_total := 0;

   q1 := $S$ SELECT o_id AS id, o_rut_cliente AS rut_cliente,
      	     	    o_monto_abonado AS monto_abonado, o_fecha_abono AS fecha_abono
    	     FROM select_abonos ( $S$ || in_rut_cliente || $S$ )
	     ORDER BY o_fecha_abono ASC $S$;

   FOR l1 IN EXECUTE q1 LOOP

       q2 := $S$ SELECT id, monto, fecha
     	     	 FROM search_deudas_cliente ($S$ ||in_rut_cliente|| $S$, false) $S$;

       IF fecha_abono_anterior IS NULL THEN
	  q2 := q2 || $S$ WHERE fecha < $S$ || quote_literal (l1.fecha_abono);
	  fecha_abono_anterior := l1.fecha_abono;
       ELSE
          q2 := q2 || $S$ WHERE fecha > $S$ || quote_literal (fecha_abono_anterior) || $S$
	     	      	  AND fecha < $S$ || quote_literal (l1.fecha_abono);
	  fecha_abono_anterior := l1.fecha_abono;
       END IF;

       q2 := q2 || $S$ ORDER BY fecha ASC $S$;

       deuda_total := out_deuda_total;
       FOR l2 IN EXECUTE q2 LOOP
       	   deuda_total := deuda_total + l2.monto;
      	   out_fecha := l2.fecha;
	   out_id_venta := l2.id;
	   out_monto_deuda := l2.monto;
	   out_abono := 0;
	   out_deuda_total := deuda_total;
           RETURN NEXT;
       END LOOP;

       out_fecha := l1.fecha_abono;
       out_id_venta := 0;
       out_monto_deuda := deuda_total;
       out_abono := l1.monto_abonado;
       out_deuda_total := deuda_total - l1.monto_abonado;

       RETURN NEXT;
   END LOOP;

RETURN;
END; $$ LANGUAGE plpgsql;
