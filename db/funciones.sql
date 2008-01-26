-- se debe ser superusuario para crear el lenguaje
CREATE LANGUAGE plpgsql;

-- estas son las funciones que deben ir en la base de datos
-- algunas no estan con buena sintaxis

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

END; $$ language plpgsql;

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
		    FROM producto$S$;

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
       			   			 OUT codigo_corto varchar(10),
				       		 OUT descripcion varchar(50),
				       		 OUT marca varchar(35),
				       		 OUT contenido varchar(10),
				       		 OUT unidad varchar(10),
				      		 OUT stock double precision,
				       		 OUT precio integer,
				       		 OUT costo_promedio integer,
				       		 OUT stock_min double precision,
				       		 OUT margen_promedio double precision,
				       		 OUT merma_unid double precision,
				       		 OUT contrib_agregada integer,
				       		 OUT ventas_dia integer,
				       		 OUT vendidos double precision,
				       		 OUT venta_neta integer,
				       		 OUT canje boolean,
				       		 OUT stock_pro double precision,
				       		 OUT tasa_canje double precision,
				       		 OUT precio_mayor integer,
				       		 OUT cantidad_mayor integer,
				       		 OUT mayorista boolean,
							 OUT unidades_merma double precision)
RETURNS SETOF record AS $$
declare
	days double precision;
	datos record;
	query varchar(500);
BEGIN

SELECT date_part ('day', (SELECT NOW() - fecha FROM compra WHERE id=t1.id_compra)) INTO days
FROM compra_detalle AS t1, producto AS t2, compra AS t3 WHERE t2.barcode= codigo_barras AND t1.barcode_product=t2.barcode
AND t3.id=t1.id_compra ORDER BY t3.fecha ASC;

IF NOT FOUND THEN
   days := 1;
END IF;

query := $S$ SELECT *, (SELECT SUM(unidades) FROM merma WHERE barcode=producto.barcode) as merma_unid,(SELECT SUM ((cantidad * precio) - (iva + otros + (fifo * cantidad))) FROM venta_detalle WHERE barcode=producto.barcode) as contrib_agregada, (producto.vendidos / $S$
|| days
|| $S$) AS merma, (SELECT SUM ((cantidad * precio) - (iva + otros)) FROM venta_detalle WHERE barcode=producto.barcode), select_merma( producto.barcode ) as unidades_merma FROM producto WHERE barcode=$S$
|| codigo_barras;

FOR datos IN EXECUTE query LOOP

codigo_corto := datos.codigo_corto;
descripcion := datos.descripcion;
marca := datos.marca;
contenido := datos.contenido;
unidad := datos.unidad;
stock := datos.stock;
precio := datos.precio;
costo_promedio := datos.costo_promedio;
stock_min := datos.stock_min;
margen_promedio := datos.margen_promedio;
merma_unid := datos.merma_unid;
contrib_agregada := datos.contrib_agregada;
unidades_merma := datos.unidades_merma;

RETURN NEXT;
END LOOP;
RETURN;
END;

$$ LANGUAGE plpgsql;


create or replace function select_merma( IN barcode bigint,
		OUT unidades_merma double precision )
returns double precision as $$
declare
   query varchar(100);
BEGIN
   unidades_merma := (SELECT unidades FROM merma WHERE barcode = $1);
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
	|| quote_literal(expresion) || $S$) $S$;

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
query := 'SELECT id, descripcion, monto FROM impuesto WHERE id != 0';

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

-- inicializa la caja
-- caja.c:780
create or replace function inicializar_caja(int4)
returns setof record as '
declare

	list record;
	query varchar(255);

begin

INSERT INTO caja VALUES(DEFAULT, NOW(), $1, to_timestamp(''DD-MM-YY'', ''00-00-00''));
RETURN;

END; ' language plpgsql;

-- cierra la caja
-- caja.c:815
create or replace function cerrar_caja(int4)
returns setof record as '
declare

	list record;
	query varchar(255);

begin
UPDATE caja SET fecha_termino=NOW(), termino=$1 WHERE id=(SELECT last_value FROM caja_id_seq);
RETURN;

END; ' language plpgsql;

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
create or replace function select_clientes()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := ''select * from clientes'';

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

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
	INSERT INTO impuesto VALUES (DEFAULT,
	       	    	      	     quote_literal(imp_descripcion),
				     imp_monto);
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
aux = (select max(id) from compra_detalle where id_compra=id_compra_in);

if aux = NULL then
   aux := 0;
else
   aux := aux + 1;
end if;

EXECUTE 'INSERT INTO compra_detalle(id, id_compra, cantidad, precio, precio_venta, cantidad_ingresada, descuento, barcode_product, margen, iva, otros_impuestos ) VALUES ('|| aux ||','|| id_compra_in ||','|| cantidad ||','|| precio ||','|| precio_venta ||','|| cantidad_ingresada ||','|| descuento ||','|| barcode_product ||','|| margen ||','|| iva ||','|| otros_impuestos ||');';

q := $S$ UPDATE producto SET precio=$S$|| precio_venta ||$S$ WHERE barcode=$S$|| barcode_product;
raise notice 'update ... %',q;
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
create or replace function buscar_proveedor(varchar(100))
returns setof record as '
declare
	q text;
	l record;
begin
q := ''SELECT * FROM proveedores WHERE lower(nombre) LIKE lower('' || quote_literal($1) || '') ''
	|| ''OR lower(rut) LIKE lower('' || quote_literal($1) || '')'';
for l in execute q loop
	return next l;
end loop;
return;
end; ' language plpgsql;

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
     email, web, contacto, giro FROM proveedor';

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
create or replace function select_asistencia(int)
returns setof record as $$
declare
	id_usuario ALIAS FOR $1;
	l record;
	q text;
begin
q := $S$SELECT date_part ('year', entrada) as entrada_year, $S$
	|| $S$date_part ('month', entrada) as entrada_month, $S$
	|| $S$date_part ('day', entrada) as entrada_day, $S$
	|| $S$date_part ('hour', entrada) as entrada_hour, $S$
	|| $S$date_part ('minute', entrada) as entrada_min, $S$
	|| $S$date_part ('year', salida) as salida_year, $S$
	|| $S$date_part ('month', salida) as salida_month, $S$
	|| $S$date_part ('day', salida) as salida_day, $S$
	|| $S$date_part ('hour', salida) as salida_hour, $S$
	|| $S$date_part ('minute', salida) as salida_min $S$
	|| $S$FROM asistencia WHERE id_user=$S$
	|| quote_literal(id_usuario) || $S$ ORDER BY entrada DESC LIMIT 1$S$;

for l in execute q loop
	return next l;
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

create or replace function buscar_producto(IN expresion varchar(255),
	IN columnas varchar[],
	IN usar_like boolean,
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
	
	FOR i IN 1..array_upper( columnas, 1) LOOP
	IF usar_like IS TRUE THEN
	IF i > 1 THEN
	query := query || $S$ or  $S$;
	END IF;
	query := query || $S$ upper( $S$ || columnas[i] || $S$ ) $S$ || $S$ like upper( '%$S$ || expresion ||$S$%' ) $S$;
	ELSE
	IF i > 1 THEN
	query := query || $S$ and $S$;
	END IF;
	query := query || columnas[i] || $S$ = upper( '$S$ || expresion || $S$' ) $S$;
	END IF;
	END LOOP;

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
		OUT ano integer,
		OUT completada smallint)
returns setof record as $$
declare
		list record;
		query text;
begin
		query := $S$ select t1.id, t2.nombre, SUM((t3.cantidad - t3.cantidad_ingresada) * t3.precio) as precio, date_part('day', t1.fecha) as dia, date_part ('month', t1.fecha) as mes, date_part('year', t1.fecha) as ano, t1.id, t3.cantidad_ingresada, t3.cantidad from compra AS t1, proveedor AS t2, compra_detalle AS t3 WHERE t2.rut=t1.rut_proveedor AND t3.id_compra=t1.id AND t3.cantidad_ingresada<t3.cantidad and t3.anulado='f' GROUP BY t1.id, t2.nombre, t1.fecha, t3.cantidad_ingresada, t3.cantidad ORDER BY fecha DESC $S$;

	FOR list IN EXECUTE query LOOP
	id_compra := list.id;
	nombre := list.nombre;
	precio := list.precio;
	dia := list.dia;
	mes := list.mes;
	ano := list.ano;
	IF list.cantidad_ingresada > 0 AND list.cantidad > list.cantidad_ingresada THEN
	completada := 1;
	ELSE
	completada := 0;
	END IF;
	RETURN NEXT;
	END LOOP;

end; $$ language plpgsql;

create or replace function get_detalle_compra(
		IN id_compra integer,
		OUT codigo_corto integer,
		OUT descripcion varchar(50),
		OUT marca varchar(35),
		OUT contenido varchar(10),
		OUT unidad varchar(10),
		OUT precio integer,
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

		SELECT impuesto.monto INTO valor FROM producto, impuesto WHERE producto.barcode=barcode and producto.impuestos='true' AND impuesto.id=0;

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
