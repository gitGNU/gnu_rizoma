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

-- retorna TODOS los productos
-- administracion_productos.c:1376
-- NO RETORNA merma_unid
create or replace function select_producto( OUT barcode int8,
					    OUT codigo_corto varchar(16),
					    OUT marca varchar(35),
					    OUT descripcion varchar(50),
					    OUT contenido varchar(10),
					    OUT unidad varchar(10),
					    OUT stock float8,
					    OUT precio int4,
					    OUT costo_promedio float8,
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
					    OUT mayorista bool,
					    OUT tipo int4)
returns setof record as $$
declare
	list record;
	query text;
	compuesta int4;
begin
query := $S$ SELECT codigo_corto, barcode, descripcion, marca, contenido,
      	     	    unidad, stock, precio, costo_promedio, vendidos, impuestos,
		    otros, familia, perecibles, stock_min, margen_promedio,
		    fraccion, canje, stock_pro, tasa_canje, precio_mayor,
		    cantidad_mayor, mayorista, tipo
		    FROM producto ORDER BY descripcion, marca$S$;

compuesta := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'COMPUESTA');

FOR list IN EXECUTE query LOOP
    barcode := list.barcode;
    codigo_corto := list.codigo_corto;
    marca := list.marca;
    descripcion := list.descripcion;
    contenido := list.contenido;
    unidad := list.unidad;

    -- Su la mercadería es compuesta, calcula su stock de acuerdo a sus componentes
    IF list.tipo = compuesta THEN 
	stock := (SELECT disponible FROM obtener_stock_desde_barcode (list.barcode));
    ELSE
	stock := list.stock;
    END IF;

    precio := list.precio;

    -- Si la mercadería es compuesta, calcula su costo promedio a partir del costo de sus componentes
    IF list.tipo = compuesta THEN
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


-- Si la mercadería es compuesta:
-- calcula el máximo stock posible de la mercadería compuesta a partir del stock de sus componentes
-- y la cantidad que usa de cada uno de ellos.
--
-- Si es de otro tipo, simplemente retorna el stock que corresponde
--
CREATE OR REPLACE FUNCTION obtener_stock_desde_barcode ( IN codigo_barras bigint,
       	  	  	   		   	         OUT disponible double precision)
RETURNS double precision AS $$
declare
	list record;
	sub_list record;
	query text;

	compuesta_l int4;
	tipo_l int4;
	cant_mud_l double precision;
	stock_l double precision;
	fraccionado boolean;
BEGIN
	SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
	SELECT tipo INTO tipo_l FROM producto WHERE barcode = codigo_barras;
	
	disponible := 0;

	-- VERIFICAR QUE LA MERCADERÍA SEA COMPUESTA
	IF (tipo_l = compuesta_l) THEN
	   -- VERIFICAR QUE TENGA PRODUCTOS ASOCIADOS (sino retorna 0) -- mas adelante recursivo
	   query := $S$ SELECT * FROM componente_mc WHERE barcode_derivado = $S$ || codigo_barras;

	   -- OBTENER EL STOCK DE SUS COMPONENTES Y VER PARA CUANTOS COMPUESTOS ALCANZAN -- mas adelante recursivo
	   FOR list IN EXECUTE query LOOP
	     stock_l := (SELECT stock FROM producto WHERE barcode = list.barcode_madre);
	     fraccionado :=  (SELECT fraccion FROM producto WHERE barcode = list.barcode_madre);
	     cant_mud_l := list.cant_mud;

	     -- Deben ser mayores a 0
	     IF (cant_mud_l <= 0 OR stock_l <= 0) THEN
	     	RETURN;
	     END IF;
	     
	     -- Si se cumple esa condición no se pueden vender ese compuesto
	     IF (stock_l < cant_mud_l) THEN
	     	RETURN;
	     END IF;
	     
	     -- Se inicializa disponible en ese caso
	     IF (disponible = 0) THEN
	     	disponible := stock_l/cant_mud_l;
	     ELSE
		-- Se elige el menor stock disponible
	     	IF ((stock_l/cant_mud_l) < disponible) THEN
	     	   disponible := stock_l/cant_mud_l;
	     	END IF;
	     END IF;

	   END LOOP;
	
	ELSE
	   disponible := (SELECT stock FROM producto WHERE barcode = codigo_barras);
	END IF;

RETURN; -- Retorna el valor de "disponible"

END; $$ language plpgsql;


-- Calcula el costo del producto compuesto a partir del costo de todos los productos
-- que lo componen.
CREATE OR REPLACE FUNCTION obtener_costo_promedio_desde_barcode ( IN codigo_barras bigint,
       	  	  	   			      		  OUT costo double precision)
RETURNS double precision AS $$
declare
	list record;
	sub_list record;
	query text;

	compuesta_l int4;
	tipo_l int4;
BEGIN
	
	SELECT id INTO compuesta_l FROM tipo_mercaderia WHERE UPPER(nombre) LIKE 'COMPUESTA';
	SELECT tipo INTO tipo_l FROM producto WHERE barcode = codigo_barras;

	costo := 0;

	-- VERIFICAR QUE LA MERCADERÍA SEA COMPUESTA
	IF (tipo_l = compuesta_l) THEN
	   query := $S$ SELECT barcode_madre, barcode_derivado, costo_promedio, cant_mud
	      	     	FROM producto p INNER JOIN componente_mc cmc
		     	ON p.barcode = cmc.barcode_madre
		     	WHERE barcode_derivado = $S$ || codigo_barras;

   	   -- Se obtiene el costo de la mercodería compuesta a partir de sus componenetes
   	   FOR list IN EXECUTE query LOOP
     	       costo := costo + (list.costo_promedio * list.cant_mud);
   	   END LOOP;

	ELSE -- Si no retorna el costo_promedio del producto
	   costo := (SELECT costo_promedio FROM producto WHERE barcode = codigo_barras);
	END IF;

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
		OUT precio integer,
		OUT costo_promedio double precision,
		OUT stock_min double precision,
		OUT margen_promedio double precision,
		OUT contrib_agregada double precision,
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
                OUT total_vendido double precision,
		OUT tipo int4)
RETURNS SETOF record AS $$
declare
	days double precision;
	datos record;
	query varchar;
	codbar int8;
	compuesta int4;
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

compuesta := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'COMPUESTA');

FOR datos IN EXECUTE query LOOP
    codigo_corto := datos.codigo_corto;
    barcode := datos.barcode;
    descripcion := datos.descripcion;
    marca := datos.marca;
    contenido := datos.contenido;
    unidad := datos.unidad;

    IF datos.tipo = compuesta THEN
        stock := (SELECT disponible FROM obtener_stock_desde_barcode (datos.barcode));
    ELSE
        stock := datos.stock;
    END IF;

    precio := datos.precio;

    -- Costo_promedio de los compuestos debería estar en el producto mismo?
    IF datos.tipo = compuesta THEN
        costo_promedio := (SELECT costo FROM obtener_costo_promedio_desde_barcode (datos.barcode));
    ELSE
	costo_promedio := datos.costo_promedio;
    END IF;

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
    tipo := datos.tipo;
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
					    OUT codigo_corto varchar(16),
					    OUT marca varchar(35),
					    OUT descripcion varchar(50),
					    OUT contenido varchar(10),
					    OUT unidad varchar(10),
					    OUT stock float8,
					    OUT precio int4,
					    OUT costo_promedio float8,
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
					    OUT mayorista bool,
					    OUT tipo int4)
returns setof record as $$
declare
	list record;
	query text;
	compuesta int4;
begin
query := $S$ SELECT barcode, codigo_corto, marca, descripcion, contenido,
      	     	    unidad, stock, precio, costo_promedio, vendidos, impuestos,
		    otros, familia, perecibles, stock_min, margen_promedio,
		    fraccion, canje, stock_pro, tasa_canje, precio_mayor,
		    cantidad_mayor, mayorista, tipo
             FROM producto WHERE estado = true AND (lower(descripcion) LIKE lower($S$
	|| quote_literal(expresion) || $S$) OR lower(marca) LIKE lower($S$
	|| quote_literal(expresion) || $S$) OR upper(codigo_corto) LIKE upper($S$
	|| quote_literal(expresion) || $S$)) ORDER BY descripcion, marca $S$;

compuesta := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'COMPUESTA');

FOR list IN EXECUTE query LOOP
    barcode := list.barcode;
    codigo_corto := list.codigo_corto;
    marca := list.marca;
    descripcion := list.descripcion;
    contenido := list.contenido;
    unidad := list.unidad;

    -- Su la mercadería es compuesta, calcula su stock de acuerdo a sus componentes
    IF list.tipo = compuesta THEN 
	stock := (SELECT disponible FROM obtener_stock_desde_barcode (list.barcode));
    ELSE
	stock := list.stock;
    END IF;

    precio := list.precio;

    -- Si la mercadería es compuesta, calcula su costo promedio a partir del costo de sus componentes
    IF list.tipo = compuesta THEN
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
					    OUT precio int4,
					    OUT costo_promedio float8,
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
					    OUT mayorista bool,
					    OUT tipo int4)
returns setof record as $$
declare
	list record;
	query text;
	compuesta int4;
begin
query := $S$ SELECT codigo_corto, barcode, descripcion, marca, contenido,
      	     	    unidad, stock, precio, costo_promedio, vendidos, impuestos,
		    otros, familia, perecibles, stock_min, margen_promedio,
		    fraccion, canje, stock_pro, tasa_canje, precio_mayor,
		    cantidad_mayor, mayorista, tipo
             FROM producto WHERE barcode= $S$
	     || quote_literal(prod_barcode);

compuesta := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'COMPUESTA');

FOR list IN EXECUTE query LOOP
    barcode := list.barcode;
    codigo_corto := list.codigo_corto;
    marca := list.marca;
    descripcion := list.descripcion;
    contenido := list.contenido;
    unidad := list.unidad;

    -- Su la mercadería es compuesta, calcula su stock de acuerdo a sus componentes
    IF list.tipo = compuesta THEN 
	stock := (SELECT disponible FROM obtener_stock_desde_barcode (list.barcode));
    ELSE
	stock := list.stock;
    END IF;

    precio := list.precio;

    -- Si la mercadería es compuesta, calcula su costo promedio a partir del costo de sus componentes
    IF list.tipo = compuesta THEN
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
	discreta int4; -- id de mercadería del tipo discreta (normal)
BEGIN

   discreta := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'DISCRETA');

   -- Productos con impuestos
   SELECT SUM (((precio / (SELECT (SUM(monto)/100)+1 FROM impuesto WHERE id = otros OR id = 1)) - costo_promedio) * stock)
   INTO monto_pci
   FROM producto WHERE impuestos = TRUE
   AND tipo = discreta;

   -- Productos sin impuestos
   SELECT (SELECT SUM((precio - costo_promedio) * stock)) 
   INTO monto_psi
   FROM producto WHERE impuestos = FALSE
   AND tipo = discreta;
   
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
       OUT activo boolean)
returns setof record as $$
declare
	l record;
	query varchar(255);
begin
query := 'select rut, dv, nombre, apell_p, apell_m, giro, direccion, telefono,
      	 	  telefono_movil, mail, abonado, credito, credito_enable, activo
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

resultado := (SELECT SUM (monto) FROM search_deudas_cliente (rut_cliente));

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
       out telefono integer,
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
					      OUT telefono int4,
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
					    OUT telefono int4,
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

-- retorna la Ãºltima linea de asistencia
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
       in in_fifo double precision,
       in in_iva double precision,
       in in_otros double precision,
       in in_tipo int4)
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
				  otros,
				  tipo)
	       	    VALUES(num_linea,
		    	   in_id_venta,
			   in_barcode,
			   in_cantidad,
			   in_precio,
			   in_fifo,
			   in_iva,
			   in_otros, 
			   in_tipo);

end;$$ language plpgsql;


-- busca productos en base un patron con el formate de LIKE
-- Si variable de entrada con_stock > 0, Productos para la venta
-- Si no, son para visualizar en mercaderia 
create or replace function buscar_producto(IN expresion varchar(255),
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
	OUT precio int4,
	OUT costo_promedio float8,
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
	OUT mayorista bool, 
	OUT tipo int4)
returns setof record as $$
declare
	list record;
	query text;
	i integer;
begin
	query := $S$ SELECT barcode, codigo_corto, marca, descripcion, contenido, unidad, 
	      	     	    (SELECT disponible FROM obtener_stock_desde_barcode (barcode)) AS stock,
			    (SELECT costo FROM obtener_costo_promedio_desde_barcode (barcode)) AS costo_promedio,
	      	     	    precio, vendidos, impuestos, otros, familia, perecibles,
			    stock_min, margen_promedio, fraccion, canje, stock_pro,
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
	    tipo := list.tipo;
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

--retorna el detalle de una compra 
--
create or replace function get_detalle_compra(
		IN id_compra integer,
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


-- retorna el iva de un producto
--
create or replace function get_iva(
		IN barcode bigint,
		OUT valor double precision)
returns double precision as $$
begin

		SELECT impuesto.monto INTO valor FROM producto, impuesto WHERE producto.barcode=barcode and producto.impuestos='true' AND impuesto.id=1;
                       
                if valor is null then
                   valor=-1;
                end if;

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
returns double precision as $$
declare
	resultado double precision;
begin

resultado := (select costo_promedio from producto where barcode=barcode_in);
return resultado;

end; $$ language plpgsql;

create or replace function ranking_ventas(
       in starts date,
       in ends date,
       out barcode varchar,
       out descripcion varchar,
       out marca varchar,
       out contenido varchar,
       out unidad varchar,
       out familia integer,
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

q := $S$ SELECT producto.barcode as barcode,
     	      producto.descripcion as descripcion,
       	      producto.marca as marca,
	      producto.contenido as contenido,
	      producto.unidad as unidad,
	      producto.familia as familia,
	      SUM (venta_detalle.cantidad) as amount,
	      SUM (((venta_detalle.cantidad*venta_detalle.precio)-(venta.descuento*((venta_detalle.cantidad*venta_detalle.precio)/(venta.monto+venta.descuento))))::integer/*-(venta_detalle.iva+venta_detalle.otros)::integer*/) as sold_amount,
	      SUM (venta_detalle.cantidad*venta_detalle.fifo) as costo,
       	      SUM ((venta_detalle.precio*cantidad)-((iva+venta_detalle.otros)+(fifo*cantidad))) as contrib
      FROM venta, venta_detalle inner join producto on venta_detalle.barcode = producto.barcode
      where venta_detalle.id_venta=venta.id and fecha>=$S$ || quote_literal(starts) || $S$ AND fecha< $S$ || quote_literal(ends) || $S$
      AND venta.id NOT IN (SELECT id_sale FROM venta_anulada) GROUP BY venta_detalle.barcode,1,2,3,4,5,6
      ORDER BY producto.descripcion ASC $S$;

for l in execute q loop
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
    return next;
end loop;

return;
end; $$ language plpgsql;

--
-- Ranking de ventas de ventas de las materias primas
-- (ventas indirectas).
--
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
BEGIN

	q := $S$ SELECT p.barcode, p.descripcion, p.marca, p.contenido, p.unidad, p.familia,
		        SUM (vmcd.cantidad) AS cantidad,
			SUM ((vmcd.cantidad*vmcd.precio)-(v.descuento*((vmcd.cantidad*vmcd.precio)/(v.monto+v.descuento)))) AS monto_vendido,
			SUM (vmcd.cantidad*vmcd.fifo) AS costo,
       			SUM ((vmcd.precio*vmcd.cantidad)-((vmcd.iva+vmcd.otros)+(vmcd.fifo*vmcd.cantidad))) AS contribucion
		 FROM venta_mc_detalle vmcd
		 INNER JOIN venta_detalle vd
		 ON vd.id_venta = vmcd.id_venta_vd
		 AND vd.id = vmcd.id_venta_detalle
		 INNER JOIN venta v
		 ON v.id = vd.id_venta
		 INNER JOIN producto p
		 ON p.barcode = vmcd.barcode
		 WHERE fecha>=$S$ || quote_literal(starts) || $S$ AND fecha< $S$ || quote_literal(ends) || $S$
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
BEGIN

	q := $S$ SELECT p.barcode, p.descripcion, p.marca, p.contenido, p.unidad,
		        SUM (vd.cantidad) AS cantidad,
			SUM ((vd.cantidad*vd.precio)-(v.descuento*((vd.cantidad*vd.precio)/(v.monto+v.descuento)))) AS monto_vendido,
			SUM (vd.cantidad*vd.fifo) AS costo,
       			SUM ((vd.precio*vd.cantidad)-((vd.iva+vd.otros)+(vd.fifo*vd.cantidad))) AS contribucion
		 FROM venta_detalle vd
		 INNER JOIN venta v
		 ON v.id = vd.id_venta
		 INNER JOIN producto p
		 ON p.barcode = vd.barcode
		 WHERE vd.barcode IN (SELECT barcode_derivado FROM componente_mc WHERE barcode_madre = $S$ || barcode_mp || $S$)
		 AND fecha>=$S$ || quote_literal(starts) || $S$ AND fecha< $S$ || quote_literal(ends) || $S$
		 GROUP BY 1,2,3,4,5 ORDER BY p.descripcion ASC $S$;

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
		OUT codigo_corto varchar(16),
		OUT descripcion varchar(50),
		OUT marca varchar(35),
		OUT contenido varchar(10),
		OUT unidad varchar(10),
		OUT precio integer,
		OUT cantidad double precision,
		OUT precio_compra double precision,
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
select last_value into current_caja from caja_id_seq;

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
		IN in_codigo_corto varchar(16),
		OUT codigo_corto varchar(16),
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
		OUT stock_day double precision,
		OUT estado boolean,
		OUT tipo integer)
RETURNS SETOF record AS $$
declare
	days double precision;
	datos record;
	query varchar;
	codbar int8;
	compuesta int4;
BEGIN

compuesta := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'COMPUESTA');

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
        -- Su la mercadería es compuesta, calcula su stock de acuerdo a sus componentes
    IF datos.tipo = compuesta THEN 
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
	
	-- -- Si es la anulacion de una venta a credito no debe ser ingresado en la tabla venta_anulada
	-- -- Debe  considerar esa venta como "pagada", de esa forma de "restituye" el credito del cliente (correspondiente a esa venta)
	-- if (select id from venta where tipo_venta=1 AND id=sale_id) IS NOT NULL then   
       	--    	 UPDATE deuda SET pagada='t' WHERE id_venta=sale_id;
	-- else
	
	perform insert_egreso (sale_amount, id_tipo_egreso, salesman_id);
	
	-- end if;

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
	out nullify_sell integer,
	out current_expenses integer,
        out cash_income integer,
        out cash_payed_money integer,
	out cash_loss_money integer,
	out bottle_return integer,
	out bottle_deposit integer
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
        from egreso e 
	inner join tipo_egreso te
	on e.tipo = te.id
        where e.id_caja = cash_box_id
	and te.descrip = 'Retiro';

        if cash_outcome is null then
                cash_outcome := 0;
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
        select SUM (descuento) into total_cash_discount from venta where fecha >= starts and fecha < ends+'1 days' and descuento!=0
                and (select forma_pago FROM documentos_emitidos where id=id_documento)=0 and venta.id not in (select id_sale from venta_anulada);

        select count(*) into total_discount from venta where fecha >= starts and fecha < ends+'1 days' and descuento!=0
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

create or replace function registrar_traspaso(
  IN monto double precision,
  IN origen integer,
  IN destino integer,
  IN vendedor integer,
  OUT inserted_id integer)
  returns integer as $$

BEGIN
	EXECUTE $S$ INSERT INTO traspaso( id, monto, fecha, origen, destino, vendedor)
	VALUES ( DEFAULT, $S$|| monto ||$S$, NOW(),$S$|| origen ||$S$,$S$|| destino ||$S$,$S$|| vendedor ||$S$) $S$;

	SELECT currval( 'traspaso_id_seq' ) INTO inserted_id;

	return;
	END; $$ language plpgsql;

create or replace function registrar_traspaso_detalle(
       in in_id_traspaso int,
       in in_barcode bigint,
       in in_cantidad double precision,
       in in_precio double precision)
returns void as $$
declare
aux int;
num_linea int;
begin
	aux := (select count(*) from traspaso_detalle where id_traspaso = in_id_traspaso);

	if aux = 0 then
	   num_linea := 0;
	else
	   num_linea := (select max(id) from traspaso_detalle where id_traspaso = in_id_traspaso);
	   num_linea := num_linea + 1;
	end if;
	INSERT INTO traspaso_detalle(id,
	       	    		  id_traspaso,
				  barcode,
				  cantidad,
				  precio)
	       	    VALUES(num_linea,
		    	   in_id_traspaso,
			   in_barcode,
			   in_cantidad,
			   in_precio);

end;$$ language plpgsql;


-- Obtiene la información de un producto en un día determinado
create or replace function producto_en_fecha(
       in fecha_inicio date,
       out barcode varchar,
       out codigo_corto varchar,
       out descripcion varchar,
       out marca varchar,
       out familia integer,
       out cantidad_ingresada double precision,
       out cantidad_c_anuladas double precision,
       out cantidad_vendida double precision,
       out cantidad_anulada double precision,
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
begin

q := $S$ SELECT DISTINCT producto.barcode, producto.codigo_corto, producto.marca, producto.descripcion, producto.contenido, producto.unidad, producto.familia, p.cantidad_ingresada, p.cantidad_c_anuladas, p.cantidad_vendida, p.unidades_merma, p.cantidad_anulada, p.cantidad_devolucion, cantidad_envio, cantidad_recibida
     	 	FROM producto
		LEFT JOIN ( 
       		     SELECT p.barcode, p.codigo_corto, p.marca, p.descripcion, p.contenido, p.unidad, p.familia, SUM(cd.cantidad_ingresada) AS cantidad_ingresada, cantidad_c_anuladas, cantidad_vendida, unidades_merma, cantidad_anulada, cantidad_devolucion, cantidad_envio, cantidad_recibida
       		            FROM compra c

		     	    INNER JOIN compra_detalle cd
       		     	    ON c.id = cd.id_compra

       		     	    INNER JOIN producto p
       		     	    ON cd.barcode_product = p.barcode 
			    
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
       
			    -- Las Mermas sufridas hasta la fecha determinada
       			    LEFT JOIN (SELECT barcode, SUM(unidades) AS unidades_merma
       	     	  	                      FROM merma m

			         	      WHERE m.fecha < $S$ || quote_literal(fecha_inicio) || $S$
			         	      GROUP BY barcode) AS merma
       			    ON p.barcode = merma.barcode

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

			    -- Los traspasos enviados hasta la fecha determinada
       			    LEFT JOIN (SELECT td.barcode AS barcode, SUM(td.cantidad) AS cantidad_recibida
       	     	  	                      FROM traspaso t
			 	 	      INNER JOIN traspaso_detalle td
			 	 	      ON t.id = td.id_traspaso

			         	      WHERE t.fecha < $S$ || quote_literal(fecha_inicio) || $S$
					      AND t.origen != 1
			         	      GROUP BY barcode) AS traspaso_recibido
                            ON p.barcode = traspaso_recibido.barcode

                     	    WHERE c.fecha < $S$ || quote_literal(fecha_inicio) || $S$
                     	    AND c.ingresada = 'TRUE'
                     	    AND p.estado = 'TRUE'
                     	    GROUP BY p.barcode, p.codigo_corto, p.marca, p.descripcion, p.contenido, p.unidad, p.familia, cantidad_c_anuladas, cantidad_vendida, unidades_merma, cantidad_anulada, cantidad_devolucion, cantidad_envio, cantidad_recibida
                     	    ORDER BY barcode) AS p
         ON producto.barcode = p.barcode

         INNER JOIN compra_detalle cd
	 ON cd.barcode_product = producto.barcode

	 INNER JOIN compra c
	 ON c.id = cd.id_compra

	 WHERE c.ingresada = 'TRUE'
	 AND producto.estado = 'TRUE'
	 ORDER BY producto.barcode ASC $S$;

for l in execute q loop
    barcode := l.barcode;
    codigo_corto := l.codigo_corto;
    marca := l.marca;
    descripcion := l.descripcion ||' '|| l.contenido ||' '|| l.unidad;
    familia := l.familia;
    cantidad_ingresada := l.cantidad_ingresada;
    cantidad_c_anuladas := COALESCE(l.cantidad_c_anuladas,0);
    cantidad_vendida := COALESCE(l.cantidad_vendida,0);
    cantidad_merma := COALESCE(l.unidades_merma,0);
    cantidad_anulada := COALESCE(l.cantidad_anulada,0);
    cantidad_devoluciones := COALESCE(l.cantidad_devolucion,0);
    cantidad_envio := COALESCE(l.cantidad_envio,0);
    cantidad_recibida := COALESCE(l.cantidad_recibida,0);
    cantidad_fecha := COALESCE(l.cantidad_ingresada,0) - COALESCE(l.cantidad_c_anuladas,0) - COALESCE(l.cantidad_vendida,0) - COALESCE(l.unidades_merma,0) + COALESCE(l.cantidad_anulada,0) - COALESCE(l.cantidad_devolucion,0) - COALESCE(l.cantidad_envio,0) + COALESCE(l.cantidad_recibida,0);
    return next;
end loop;

return;
end; $$ language plpgsql;


-- Entrega la informacion del producto desde la fecha otorgada hasta ahora
create or replace function producto_en_periodo(
       in fecha_inicio date,
       out barcode varchar,
       out codigo_corto varchar,
       out descripcion varchar,
       out marca varchar,
       out familia integer,
       out stock_inicial double precision,
       out compras_periodo double precision,
       out anulaciones_c_periodo double precision,
       out ventas_periodo double precision,
       out anulaciones_periodo double precision,       
       out devoluciones_periodo double precision,
       out mermas_periodo double precision,
       out enviados_periodo double precision,
       out recibidos_periodo double precision,
       out stock_teorico double precision
       )
RETURNS setof record AS $$
DECLARE
q text;
q2 text;
q3 text;
l record;
z record;
BEGIN

q := $S$ SELECT stock1.barcode AS barcode,
	        stock1.codigo_corto AS codigo_corto,
       	 	stock1.marca AS marca,
		stock1.familia AS familia,
       	 	stock1.descripcion AS descripcion,
		stock1.cantidad_ingresada AS cantidad_ingresada,       	 	

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
       
	 FROM producto_en_fecha( $S$ || quote_literal(fecha_inicio) || $S$ ) stock1 INNER JOIN producto_en_fecha( $S$ || quote_literal(current_date + 1) || $S$ ) stock2
       	 ON stock1.barcode = stock2.barcode $S$;

q2 := $S$SELECT SUM(cd.cantidad_ingresada) AS cantidad_ingresada_n, COALESCE(cantidad_c_anuladas,0) AS cantidad_c_anuladas_n, COALESCE(cantidad_vendida,0) AS ventas_n, COALESCE(unidades_merma,0) AS mermas_n, COALESCE(cantidad_anulada,0) AS anuladas_n, COALESCE(cantidad_devolucion,0) AS devolucion_n, COALESCE(cantidad_envio,0) AS envios_n, COALESCE(cantidad_recibida,0) AS recibida_n
         FROM compra c

         INNER JOIN compra_detalle cd
         ON c.id = cd.id_compra

         INNER JOIN producto p
         ON cd.barcode_product = p.barcode 
	 
	 -- Las anulaciones de compras hechas hasta la fecha determinada
    	 LEFT JOIN (SELECT SUM(cad.cantidad_anulada) AS cantidad_c_anuladas, cad.barcode AS barcode_ca
	      	   	   FROM compra_anulada ca
			   INNER JOIN compra_anulada_detalle cad
			   ON ca.id = cad.id_compra_anulada
			   GROUP BY barcode) AS compras_anuladas_n
	 ON p.barcode = compras_anuladas_n.barcode_ca

         -- Las Ventas hechas hasta la fecha determinada
         LEFT JOIN (SELECT SUM(vd.cantidad) AS cantidad_vendida, vd.barcode AS barcode_v -- LEFT JOIN MUESTRA TODOS LOS PRODUCTOS
       	     	  	  FROM venta v
			  INNER JOIN venta_detalle vd 
			  ON v.id = vd.id_venta
			  GROUP BY barcode_v) AS ventas_n
         ON p.barcode = ventas_n.barcode_v

         -- Las anulaciones de venta hechas hasta la fecha determinada
         LEFT JOIN (SELECT SUM(vd.cantidad) AS cantidad_anulada, vd.barcode AS barcode_a 
       	     	  	  FROM venta v
			  INNER JOIN venta_detalle vd 
			  ON v.id = vd.id_venta
			  INNER JOIN venta_anulada va
			  ON va.id_sale = v.id
			  GROUP BY barcode_a) AS ventas_anuladas_n
         ON p.barcode = ventas_anuladas_n.barcode_a
       
         -- Las Mermas sufridas hasta la fecha determinada
         LEFT JOIN (SELECT merma.barcode AS barcode_m, SUM(merma.unidades) AS unidades_merma
       	     	  	 FROM merma
			 GROUP BY barcode_m) AS merma_n
         ON p.barcode = merma_n.barcode_m

         -- Las devoluciones hechas hasta la fecha determinada
         LEFT JOIN (SELECT dd.barcode AS barcode_d, SUM(dd.cantidad) AS cantidad_devolucion
       	     	  	 FROM devolucion d
			 INNER JOIN devolucion_detalle dd
			 ON d.id = dd.id_devolucion
			 GROUP BY barcode_d) AS devolucion_n
         ON p.barcode = devolucion_n.barcode_d

	 -- Los traspasos enviados hasta la fecha determinada
     	 LEFT JOIN (SELECT td.barcode AS barcode_td, SUM(td.cantidad) AS cantidad_envio
      	                 FROM traspaso t
			 INNER JOIN traspaso_detalle td
			 ON t.id = td.id_traspaso
		         AND t.origen = 1
			 GROUP BY barcode) AS traspaso_envio_n
         ON p.barcode = traspaso_envio_n.barcode_td

	 -- Los traspasos enviados hasta la fecha determinada
       	 LEFT JOIN (SELECT td.barcode AS barcode_td2, SUM(td.cantidad) AS cantidad_recibida
       	                 FROM traspaso t
	 	         INNER JOIN traspaso_detalle td
			 ON t.id = td.id_traspaso
		         AND t.origen != 1
			 GROUP BY barcode) AS traspaso_recibido_n
         ON p.barcode = traspaso_recibido_n.barcode_td2

         WHERE c.ingresada = 'TRUE'
         AND p.estado = 'TRUE'
         AND p.barcode =$S$;

FOR l IN EXECUTE q loop
    barcode := l.barcode;
    codigo_corto := l.codigo_corto;
    marca := l.marca;
    familia := l.familia;
    descripcion := l.descripcion;
    IF l.cantidad_ingresada IS NULL THEN   -- Significa que no ha sido comprado aún, por lo que se mostrará toda su información sin limite de fecha    
       q3 := q2|| l.barcode || $S$ GROUP BY cantidad_c_anuladas_n, ventas_n, mermas_n, anuladas_n, devolucion_n, envios_n, recibida_n$S$;
       FOR z IN EXECUTE q3 loop
          stock_inicial := 0;
          compras_periodo := z.cantidad_ingresada_n;
	  anulaciones_c_periodo := z.cantidad_c_anuladas_n;
          ventas_periodo := z.ventas_n;
	  anulaciones_periodo := z.anuladas_n;
          devoluciones_periodo := z.devolucion_n;
          mermas_periodo := z.mermas_n;
	  enviados_periodo := z.envios_n;
	  recibidos_periodo := z.recibida_n;
          stock_teorico := z.cantidad_ingresada_n - z.cantidad_c_anuladas_n - z.mermas_n - z.ventas_n - z.devolucion_n + z.anuladas_n - z.envios_n + z.recibida_n;
       END loop;
    ELSE
       stock_inicial := l.stock1_cantidad_fecha;  -- cantidad_fecha = stock con el que se inicio el día seleccionado (ESTE SE MANTIENE)
       compras_periodo := l.stock2_cantidad_ingresada - l.stock1_cantidad_ingresada;
       anulaciones_c_periodo := l.stock2_cantidad_c_anuladas - l.stock1_cantidad_c_anuladas;
       ventas_periodo := l.stock2_cantidad_vendida - l.stock1_cantidad_vendida;
       anulaciones_periodo := l.stock2_cantidad_anulada - l.stock1_cantidad_anulada;
       devoluciones_periodo := l.stock2_cantidad_devoluciones - l.stock1_cantidad_devoluciones;
       mermas_periodo := l.stock2_cantidad_merma - l.stock1_cantidad_merma;
       enviados_periodo := l.stock2_cantidad_envio - l.stock1_cantidad_envio;
       recibidos_periodo := l.stock2_cantidad_recibida - l.stock1_cantidad_recibida;
       stock_teorico := l.stock2_cantidad_fecha;
    END IF;
    RETURN NEXT;
END loop;

return;
end; $$ language plpgsql;


-- Anula una compra y devuelve el resultado
create or replace function nullify_buy(
       IN id_compra_in integer,
       IN maquina integer,
       OUT barcode varchar,
       OUT cantidad double precision,
       OUT cantidad_anulada double precision,
       OUT nuevo_stock double precision,
       OUT costo double precision,
       OUT precio integer
       )
RETURNS setof record AS $$
DECLARE
id_compra_anulada integer;
rut_proveedor_compra integer;
query text;
l record;
BEGIN

-- Se obtiene el rut del proveedor correspondiente a la compra
SELECT rut_proveedor INTO rut_proveedor_compra FROM compra WHERE id = id_compra_in;

-- Se inserta los datos de la compra en la tabla compra_anulada
EXECUTE $S$ INSERT INTO compra_anulada (id_compra, fecha_anulacion, rut_proveedor, maquina)
	    VALUES ($S$|| id_compra_in ||$S$, NOW(), $S$|| rut_proveedor_compra ||$S$,$S$|| maquina ||$S$ ) $S$;

-- Se obtiene el id de la compra_anulada
SELECT id INTO id_compra_anulada FROM compra_anulada WHERE id_compra = id_compra_in;

-- Se ingresa nota de credito de ser necesario
EXECUTE 'INSERT INTO nota_credito (fecha, num_factura, fecha_factura, rut_proveedor)
	 SELECT NOW(), num_factura, fecha, rut_proveedor 
	 FROM factura_compra
	 WHERE pagada = true
	 AND id_compra = '|| id_compra_in;

-- Se ingresa el detalle de nota de credito de ser necesario
EXECUTE 'INSERT INTO nota_credito_detalle (id_nota_credito, barcode, costo, precio, cantidad)
	 SELECT 
	 	(SELECT id FROM nota_credito nc WHERE nc.num_factura = fc.num_factura) AS id_nota_credito,
	 	fcd.barcode, fcd.precio AS costo, 
	 	(SELECT precio FROM producto WHERE barcode = fcd.barcode) AS precio, 
	 fcd.cantidad 
	 FROM factura_compra_detalle fcd
	 INNER JOIN factura_compra fc
	 ON fc.id = fcd.id_factura_compra
	 WHERE fc.pagada = true
	 AND fc.id_compra = '|| id_compra_in;

-- Obtengo todos los productos correspondiente a la compra
query := 'SELECT fc.id, fcd.barcode, precio AS costo, 
          (SELECT precio FROM producto WHERE barcode = fcd.barcode) AS precio,
	  (SELECT stock FROM producto WHERE barcode = fcd.barcode) AS stock_actual,
	  (SELECT pagada FROM factura_compra WHERE id = fc.id) AS pagada,
          SUM (cantidad) AS cantidad
      	  FROM factura_compra_detalle fcd
	  INNER JOIN factura_compra fc
	  ON fcd.id_factura_compra = fc.id
	  WHERE fc.id_compra = ' || id_compra_in ||
	 'GROUP BY barcode, costo, precio, fc.id';

FOR l IN EXECUTE query loop
    --Registrando datos a retornar
    barcode := l.barcode;
    costo := l.costo;
    precio := l.precio;

    --La cantidad a anular no puede ser mayor al stock actual
    IF l.stock_actual < l.cantidad THEN
        cantidad_anulada := l.stock_actual;
	nuevo_stock := 0;
    ELSE
	cantidad_anulada := l.cantidad;
	nuevo_stock := l.stock_actual - l.cantidad;
    END IF;

    --Reajustar stock producto
    UPDATE producto SET stock = nuevo_stock WHERE barcode = barcode;

    --Colocar los productos en la tabla compra_anulada_detalle
    EXECUTE 'INSERT INTO compra_anulada_detalle (id_compra_anulada, barcode, costo, precio, cantidad, cantidad_anulada)
             VALUES ('||id_compra_anulada||','||barcode||','||costo||','||precio||','||l.cantidad||','||cantidad_anulada||')';

    --Reajustar la cantidad del producto en nota_credito_detalle
    -- en caso de que no se haya podido anular la cantidad completa (stock < cantidad)
    
    --Recalcular costo_promedio

    RETURN NEXT;
END loop;

--Actualizar anulada_pi en compra
UPDATE compra SET anulada_pi = 't' WHERE id = id_compra_in;

RETURN;
END; $$ language plpgsql;

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

	query := $S$ SELECT v.id, v.monto, v.maquina, v.vendedor, v.fecha, v.tipo_venta, 
	      	     	    pm.rut1, pm.rut2, pm.tipo_pago1, pm.tipo_pago2, pm.monto1, pm.monto2
	      	     FROM venta v LEFT JOIN pago_mixto pm ON pm.id_sale = v.id
		     WHERE v.id IN (SELECT id_venta FROM deuda WHERE rut_cliente=$S$||rut||$S$ AND pagada='f')
		     AND v.id NOT IN (SELECT id_sale FROM venta_anulada)
		     ORDER BY v.id DESC $S$;

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


--registra el detalle de una venta
create or replace function registrar_venta_mc_detalle(IN in_id_venta int4,
                                                      IN in_barcode_product bigint,
						      IN in_cantidad double precision,
						      IN in_precio int,
						      IN in_iva double precision,
						      IN in_otros double precision)
returns void as $$
declare
	list record;
	query varchar;
	id_vd_1 int4;
	id_vd_2 int4;
	compuesta int4;
	costo_product_mc double precision;
begin
	compuesta := (SELECT id FROM tipo_mercaderia WHERE upper(nombre) LIKE 'COMPUESTA');
	id_vd_1 := (SELECT id FROM venta_detalle WHERE id_venta = in_id_venta AND barcode = in_barcode_product);
	id_vd_2 := in_id_venta;
	
	query := $S$ SELECT * FROM componente_mc WHERE barcode_derivado = $S$ || in_barcode_product;

	FOR list IN EXECUTE query LOOP

	    costo_product_mc := (SELECT costo_promedio FROM producto WHERE barcode = list.barcode_madre);

	    INSERT INTO venta_mc_detalle (id,
	    	   			  id_venta_detalle,
	       	    		       	  id_venta_vd,
				       	  barcode,
				       	  cantidad,
				       	  precio,
				       	  fifo,
				       	  iva,
				       	  otros,
				       	  tipo)
	       	   VALUES (DEFAULT,
			   id_vd_1,
			   id_vd_2,
			   list.barcode_madre,
			   list.cant_mud * in_cantidad,
			   in_precio, -- TODO: si tenga varios componentes de debe repartir el precio entre todos
			   costo_product_mc, -- Costo promedio del producto
			   in_iva, -- TODO: cuando tenga varios componentes debe ser calcuado de forma especial
			   in_otros, -- TODO: cuando tenga varios componentes debe ser calcuado de forma especial
			   list.tipo_madre);
	END LOOP;

END;
$$ LANGUAGE plpgsql;


--registra el detalle de una venta
create or replace function update_stock_producto_compuesto (IN in_barcode_product bigint,
						            IN in_cantidad double precision)
returns void as $$
declare
	list record;
	query varchar;
begin	
	query := $S$ SELECT * FROM componente_mc WHERE barcode_derivado = $S$ || in_barcode_product;

	FOR list IN EXECUTE query LOOP

	    UPDATE producto 
	    SET vendidos=vendidos+(list.cant_mud * in_cantidad), stock=stock-(list.cant_mud * in_cantidad)
	    WHERE barcode=list.barcode_madre;

	END LOOP;
END;
$$ LANGUAGE plpgsql;
