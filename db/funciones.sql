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
create or replace function puedo_devolver(float8, int8)
returns setof record as $$
declare
	num_prods ALIAS FOR $1;
	prod ALIAS FOR $2;
	list record;
	query varchar(255);

begin
query := 'SELECT cantidad<' || quote_literal(num_prods)
	|| ' as respuesta FROM devoluciones '
	|| ' WHERE id=(SELECT id FROM devoluciones WHERE barcode_product='
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
-- administracion_productos.c:1351
create or replace function insert_producto(prod_barcode int8, prod_codigo varchar(10),prod_precio int4)
returns void as $$
begin
INSERT INTO producto (barcode,codigo_corto,precio) VALUES (prod_barcode,
       quote_literal(prod_codigo), prod_precio);

RETURN ;
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
create or replace function select_producto()
returns setof record as $$
declare

	list record;
	query text;

begin
query := 'SELECT codigo_corto, barcode, descripcion, marca, contenido, unidad, '
      || 'stock, precio, costo_promedio, vendidos, impuestos, otros, familia, '
      || 'perecibles, stock_min, margen_promedio, fraccion, canje, stock_pro, '
      || 'tasa_canje, precio_mayor, cantidad_mayor, mayorista FROM producto';

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; $$ language plpgsql;

-- Informacion de los productos para la ventana de Mercaderia
-- administracion_productos.c:1508
CREATE FUNCTION estadisticas_producto( IN codigo_barras bigint, 
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
				       OUT mayorista boolean)
RETURNS SETOF record AS $$
declare
days double precision;
datos record;
query varchar(500);
BEGIN

SELECT date_part ('day', (SELECT NOW() - fecha FROM compra WHERE id=t1.id_compra)) INTO days
FROM compra_detalle AS t1, producto AS t2, compra AS t3 WHERE t2.barcode= $1 AND t1.barcode_product=t2.barcode
AND t3.id=t1.id_compra ORDER BY t3.fecha ASC;

IF NOT FOUND THEN
days := 1;
END IF;

query := 'SELECT *, (SELECT SUM(unidades) FROM merma WHERE barcode=producto.barcode) as merma_unid,(SELECT SUM ((cantidad * precio) - (iva + otros + (fifo * cantidad))) FROM venta_detalle WHERE barcode=producto.barcode) as contrib_agregada, (producto.vendidos / '
|| days
|| ') AS merma, (SELECT SUM ((cantidad * precio) - (iva + otros)) FROM venta_detalle WHERE barcode=producto.barcode) FROM producto WHERE barcode='
|| $1;

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

RETURN NEXT;
END LOOP;
RETURN;
END;

$$ LANGUAGE plpgsql;

-- -- ??
-- -- administracion_productos.c:1637
-- create or replace function ()
-- returns setof record as '
-- declare

-- 	list record;
-- 	query varchar(255);

-- begin
-- "SELECT nombre FROM proveedores WHERE rut IN (SELECT rut_proveedor FROM compras WHERE id IN (SELECT id_compra FROM products_buy_history WHERE barcode_product='%s')) GROUP BY nombre", barcode)
-- query := '''';


-- FOR list IN EXECUTE query LOOP
-- 	RETURN NEXT list;
-- END LOOP;

-- RETURN;

-- END; ' language plpgsql;

-- busca productos en base un patron con el formate de LIKE
-- administracion_productos.c:1738
-- compras.c:4057
create or replace function buscar_productos(varchar(255))
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := ''SELECT * FROM productos WHERE lower(descripcion) LIKE lower(''
	|| quote_literal($1) || '')''
	|| '' OR lower(marca) LIKE lower(''
	|| quote_literal($1) || '')'';

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- esta funcion es util para obtener los datos de un barcode dado
-- administracion_productos.c:1803
-- postgres-functions.c:941, 964, 978, 990, 1197, 1432, 1480, 1530, 1652, 1853
-- ventas.c:95, 1504, 3026, 3032,
create or replace function select_producto(varchar(14))
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := ''SELECT codigo FROM productos WHERE barcode=''
	|| quote_literal($1);

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;
RETURN;

END; ' language plpgsql;

-- retorna todos los impuestos menos el IVA
-- administracion_productos.c:2099
-- compras.c:3170, 3679
create or replace function obtener_impuestos()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := ''SELECT * FROM impuestos WHERE id != 0'';

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

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

-- ??
-- caja.c:63
create or replace function arqueo_caja_ultimo_dia()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT (SELECT inicio FROM caja WHERE date_trunc ('day', fecha_inicio)=date_trunc('day', t1.fecha_inicio)) AS inicio, (SELECT SUM (monto) FROM ventas WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio) AND tipo_venta=%d) AS ventas_efect, (SELECT SUM(t1.monto) FROM cheques AS t1 WHERE id_venta IN (SELECT id FROM ventas WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio))) AS ventas_doc, (SELECT SUM(monto_abonado) FROM abonos WHERE date_trunc('day', fecha_abono)=date_trunc('day', fecha_inicio)) AS pago_credit, (SELECT SUM(monto) FROM ingresos WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio)) AS otros, (SELECT SUM (monto) FROM egresos WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio) AND tipo=1) AS retiros, (SELECT SUM (monto) FROM egresos WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio) AND tipo=3) AS gastos, (SELECT SUM (monto) FROM egresos WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio) AND tipo=2) AS otros_egresos, (SELECT SUM(monto) FROM facturas_compras WHERE id IN (SELECT id_fact FROM pagos  WHERE caja='t' AND date_trunc('day', fecha)=date_trunc('day', fecha_inicio))) AS facturas, fecha_inicio FROM caja AS t1 WHERE t1.fecha_termino=to_timestamp('DD-MM-YY', '00-00-00')", CASH;

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- caja.c:86
create or replace function saldo_caja()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT (SELECT inicio FROM caja WHERE date_trunc ('day', fecha_inicio)=date_trunc('day', localtimestamp)) AS inicio, (SELECT SUM (monto) FROM ventas WHERE date_trunc('day', fecha)=date_trunc('day', localtimestamp) AND tipo_venta=%d) AS ventas_efect, (SELECT SUM(t1.monto) FROM cheques AS t1 WHERE id_venta IN (SELECT id FROM ventas WHERE date_trunc('day', fecha)=date_trunc('day', localtimestamp))) AS ventas_doc, (SELECT SUM (monto) FROM egresos WHERE date_trunc('day', fecha)=date_trunc('day', localtimestamp) AND tipo=1) AS retiros, (SELECT SUM(monto_abonado) FROM abonos WHERE date_trunc('day', fecha_abono)=date_trunc('day', localtimestamp)) AS pago_credit, (SELECT SUM(monto) FROM ingresos WHERE date_trunc('day', fecha)=date_trunc('day', localtimestamp)) AS otros, (SELECT SUM (monto) FROM egresos WHERE date_trunc('day', fecha)=date_trunc('day', localtimestamp) AND tipo=3) AS gastos, (SELECT SUM (monto) FROM egresos WHERE date_trunc('day', fecha)=date_trunc('day', localtimestamp) AND tipo=2) AS otros_egresos, (SELECT SUM(monto) FROM facturas_compras WHERE id IN (SELECT id_fact FROM pagos  WHERE caja='t' AND date_trunc('day', fecha)=date_trunc('day', localtimestamp))) AS facturas", CASH)

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- retorna los tipos de ingreso
-- caja.c:191
create or replace function select_tipo_ingreso()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := ''SELECT * FROM tipo_ingreso'';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- retorna los tips de egreso
-- caja.c:317
create or replace function select_tipo_egreso()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := ''SELECT * FROM tipo_egreso'';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- caja.c:379
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT (SELECT inicio FROM caja WHERE date_part('year', fecha_inicio)=%d AND date_part('month', fecha_inicio)=%d AND date_part('day', fecha_inicio)=%d) AS inicio, (SELECT SUM (monto) FROM ventas WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d AND tipo_venta=%d) AS ventas_efect, (SELECT SUM(t1.monto) FROM cheques AS t1 WHERE id_venta IN (SELECT id FROM ventas WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d AND date_part('year', fecha)=date_part('year', t1.fecha) AND date_part('month', fecha)=date_part('month', t1.fecha) AND date_part('day', fecha)=date_part('day', t1.fecha))) AS ventas_doc, (SELECT SUM (monto) FROM egresos WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d AND tipo=1) AS retiros, (SELECT SUM(monto_abonado) FROM abonos WHERE date_part('year', fecha_abono)=%d AND date_part('month', fecha_abono)=%d AND date_part('day', fecha_abono)=%d) AS pago_credit, (SELECT SUM(monto) FROM ingresos WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d) AS otros, (SELECT SUM (monto) FROM egresos WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d AND tipo=3) AS gastos, (SELECT SUM (monto) FROM egresos WHERE date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d AND tipo=2) AS otros_egresos, (SELECT SUM(monto) FROM facturas_compras WHERE id IN (SELECT id_fact FROM pagos  WHERE caja='t' AND date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND date_part('day', fecha)=%d)) AS facturas",
		      year, month+1, day, year, month+1, day, CASH, year, month+1, day, year, month+1,
		      day, year, month+1, day, year, month+1, day, year, month+1, day, year,
		      month+1, day, year, month+1, day

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

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

-- ??
-- caja.c:793
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT date_part('day', fecha_inicio) AS dia, date_part('month', fecha_inicio) AS mes, date_part('year', fecha_inicio) AS ano FROM caja WHERE id=(SELECT last_value FROM caja_id_seq)

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ?????
-- caja.c:798
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT ((SELECT inicio FROM caja WHERE date_part('year', fecha_inicio)=%s AND date_part('month', fecha_inicio)=%s AND date_part('day', fecha_inicio)=%s) + (SELECT SUM (monto) FROM ventas WHERE date_part('year', fecha)=%s AND date_part('month', fecha)=%s AND date_part('day', fecha)=%s AND tipo_venta=%d) + (SELECT SUM(t1.monto) FROM cheques AS t1 WHERE id_venta IN (SELECT id FROM ventas WHERE date_part('year', fecha)=%s AND date_part('month', fecha)=%s AND date_part('day', fecha)=%s AND date_part('year', fecha)=date_part('year', t1.fecha) AND date_part('month', fecha)=date_part('month', t1.fecha) AND date_part('day', fecha)=date_part('day', t1.fecha))) + (SELECT SUM(monto_abonado) FROM abonos WHERE date_part('year', fecha_abono)=%s AND date_part('month', fecha_abono)=%s AND date_part('day', fecha_abono)=%s) + (SELECT SUM(monto) FROM ingresos WHERE date_part('year', fecha)=%s AND date_part('month', fecha)=%s AND date_part('day', fecha)=%s)) - ((SELECT SUM (monto) FROM egresos WHERE date_part('year', fecha)=%s AND date_part('month', fecha)=%s AND date_part('day', fecha)=%s AND tipo=1) + (SELECT SUM (monto) FROM egresos WHERE date_part('year', fecha)=%s AND date_part('month', fecha)=%s AND date_part('day', fecha)=%s AND tipo=3) + (SELECT SUM (monto) FROM egresos WHERE date_part('year', fecha)=%s AND date_part('month', fecha)=%s AND date_part('day', fecha)=%s AND tipo=2) + (SELECT monto FROM facturas_compras WHERE id IN (SELECT id_fact FROM pagos  WHERE caja='t')))", PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0), PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0), CASH, PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0), PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0), PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0), PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0), PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0), PQgetvalue (res, 0, 2), PQgetvalue (res, 0, 1), PQgetvalue (res, 0, 0))

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

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

-- ??
-- caja.c:831
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT SUM (monto) as total_sell FROM ventas WHERE "
				      "date_part('day', fecha)=date_part('day', CURRENT_DATE) "
				      "AND date_part('month', fecha)=date_part('month', CURRENT_DATE) AND"
				      " date_part('year', fecha)=date_part('year', CURRENT_DATE) AND tipo_venta=%d",
				      CASH)

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- caja.c:837
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := ''SELECT termino FROM caja WHERE id=(SELECT last_value FROM caja_id_seq)'';

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??inserta una nueva perdida
-- caja.c:841
create or replace function insert_perdida (int4)
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
INSERT INTO caja (perdida) VALUES($1) WHERE id=(SELECT last_value FROM caja_id_seq);
RETURN;

END; ' language plpgsql;

-- ??
-- caja.c:859
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT fecha_termino FROM caja WHERE id=(SELECT last_value FROM caja_id_seq) AND "
     "date_part('day', fecha_inicio)=date_part('day', NOW()) AND "
     "date_part('year', fecha_inicio)=date_part('year', NOW()) AND date_part('year', "
     "fecha_inicio)=date_part('year', NOW())"

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- retorna el monto con que termino el ultimo cierre de caja
-- caja.c:937
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := ''SELECT termino FROM caja WHERE id=(SELECT last_value FROM caja_id_seq)'';

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- retorna los datos un proveedor dado
-- compras.c:644, 4650, 4945, 6200
create or replace function obtener_proveedor(varchar(20))
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := ''SELECT * FROM proveedores WHERE rut='' || quote_literal($1);

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:686
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT t1.codigo, t1.descripcion, t1.marca, t1.contenido, t1.unidad, t2.cantidad, t2.precio, (t2.cantidad * t2.precio)::double precision AS total, t2.barcode, t2.id_compra, date_part('year', t2.fecha), date_part('month', t2.fecha), date_part('day', t2.fecha), (SELECT num_factura FROM facturas_compras WHERE id=(SELECT id_factura FROM guias_compra WHERE numero=%s)) FROM productos AS t1, documentos_detalle AS t2 WHERE t2.id_compra=(SELECT id_compra FROM guias_compra WHERE numero=%s AND rut_proveedor='%s') AND t1.barcode=t2.barcode AND t2.numero=%s", doc, doc, rut_proveedor, doc

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:725
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT monto, date_part('day',fecha), date_part('month',fecha), "
		  "date_part('year',fecha) FROM facturas_compras WHERE num_factura=%s",
		  doc
FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:750
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT t1.codigo, t1.descripcion, t1.marca, t1.contenido, t1.unidad, t2.cantidad, t2.precio, (t2.cantidad * t2.precio)::double precision AS total, t2.barcode, t2.id_compra, date_part('year', t2.fecha), date_part('month', t2.fecha), date_part('day', t2.fecha) FROM productos AS t1, documentos_detalle AS t2 WHERE t2.id_compra=(SELECT id_compra FROM facturas_compras WHERE num_factura=%s AND rut_proveedor='%s' OR id=%s) AND t1.barcode=t2.barcode AND t2.numero=%s", doc, rut_proveedor, id, doc

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??revisar si se satisface con alguna funcion anterior
-- compras.c:2749
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
-- esta sentencia creo que se puede satisfacer con una anterior
"select barcode from productos where codigo = '%s'"
FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

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

-- ??esta se puede satisfacer facilmente con la funcion obtener_producto(varchar(14))
-- compras.c:2790
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := ''SELECT descripcion FROM productos WHERE barcode='%s'", barcode'';

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??se puede satisfacer con obtener_producto(varchar(14))
-- compras.c:2794
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := ''"SELECT marca, fifo, canje, stock_pro FROM productos WHERE barcode='%s'", barcode)'';

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??se puedensatisfacer con obtener_producto(varchar(14))
-- compras.c:2853, 2855, 2857, 2859, 2861, 2863
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';

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

-- ??
-- compras.c:4330
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
"SELECT (SELECT nombre FROM proveedores WHERE rut=t1.rut_proveedor), t2.precio, t2.cantidad,"
      " date_part('day', t1.fecha), date_part('month', t1.fecha), date_part('year', t1.fecha), "
      "t1.id, t2.iva, t2.otros FROM compras AS t1, products_buy_history AS t2, productos WHERE "
      "productos.barcode='%s' AND t2.barcode_product=productos.barcode AND t1.id=t2.id_compra "
      "AND t2.anulado='f' ORDER BY t1.fecha DESC", barcode)

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:4403
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
"SELECT t2.nombre, (SELECT SUM ((t2.cantidad - t2.cantidad_ingresada) * t2.precio) FROM products_buy_history AS t2 WHERE t2.id_compra=t1.id)::double precision AS precio, date_part('day', t1.fecha), date_part ('month', t1.fecha), date_part('year', t1.fecha), t1.id FROM compras AS t1, proveedores AS t2, products_buy_history AS t3 WHERE t2.rut=t1.rut_proveedor AND t3.id_compra=t1.id AND t3.cantidad_ingresada<t3.cantidad AND t1.anulada='f' AND t3.anulado='f' GROUP BY t1.id, t2.nombre, t1.fecha ORDER BY fecha DESC"

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:4460
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
"SELECT t2.codigo, t2.descripcion, t2.marca, t2.contenido, t2.unidad, t1.precio, "
	 "t1.cantidad, t1.cantidad_ingresada, (t1.precio * (t1.cantidad - t1.cantidad_ingresada))::bigint,"
	 "t2.barcode, t1.precio_venta, t1.margen FROM products_buy_history AS t1, productos AS t2 "
	 "WHERE t1.id_compra=%d AND t2.barcode=t1.barcode_product AND t1.cantidad_ingresada<t1.cantidad "
	 "AND t1.anulado='f'", id)

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??retorna del rut del proveedor para una compra dada
-- revisar si se puede satisfacer con alguna funcion anteriormente definida
-- compras.c:4549, 7023
create or replace function (int)
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := ''SELECT rut_proveedor FROM compras WHERE id='' || quote_literal($1);

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

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

-- ??
-- compras.c:5636
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT date_part('day', fecha), date_part('month', fecha), "
				      "date_part('year', fecha) FROM compras WHERE id=%d", id

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:5704
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT num_factura FROM facturas_compras WHERE rut_proveedor='%s' AND num_factura=%s", rut_proveedor, n_documento

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:5740
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT numero FROM guias_compra WHERE rut_proveedor='%s' AND numero=%s", rut_proveedor, n_documento)

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:5766
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT t1.id, t1.num_factura, t1.monto, date_part ('day', t1.fecha), date_part('month', t1.fecha), date_part('year', t1.fecha), t1.id_compra, date_part ('day', fecha_pago) AS pay_day, date_part ('month', fecha_pago) AS pay_month, date_part ('year', fecha_pago) AS pay_year, t1.forma_pago, t1.id, t1.rut_proveedor FROM facturas_compras AS t1 WHERE t1.rut_proveedor='%s' AND t1.pagada='f' ORDER BY pay_year, pay_month, pay_day ASC", rut_proveedor

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:5769
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT t1.id, t1.num_factura, t1.monto, date_part ('day', t1.fecha), date_part('month', t1.fecha), date_part('year', t1.fecha), t1.id_compra, date_part ('day', fecha_pago) AS pay_day, date_part ('month', fecha_pago) AS pay_month, date_part ('year', fecha_pago) AS pay_year, t1.forma_pago, t1.id, t1.rut_proveedor FROM facturas_compras AS t1 WHERE pagada='f' ORDER BY pay_year, pay_month, pay_day ASC"

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:5823
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT numero, id_compra, id, date_part ('day', fecha_emicion), "
	  "date_part ('month', fecha_emicion), date_part ('year', fecha_emicion), rut_proveedor FROM "
	  "guias_compra WHERE id_factura=%s", PQgetvalue (res, i, 0)

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:5867
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT t1.numero, t1.id_compra, date_part ('day', t1.fecha_emicion), date_part ('month', t1.fecha_emicion), date_part ('year', t1.fecha_emicion), (SELECT SUM (t2.cantidad * t2.precio) FROM documentos_detalle AS t2 WHERE t2.numero=t1.numero AND t2.id_compra=t1.id_compra), (SELECT nombre FROM formas_pago WHERE id=(SELECT forma_pago FROM compras WHERE id=t1.id_compra)) FROM guias_compra AS t1, products_buy_history AS t3 WHERE t1.rut_proveedor='%s' AND t3.id_compra=t1.id_compra AND t1.id_factura=0 GROUP BY t1.numero, t1.id_compra, t1.fecha_emicion", proveedor

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:5909
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT t1.codigo, t1.descripcion,  t2.cantidad, t2.precio, t2.cantidad * t2.precio AS "
	  "total, t2.barcode, t2.id_compra, t1.marca, t1.contenido, t1.unidad FROM productos AS "
	  "t1, documentos_detalle AS t2 WHERE t2.id_compra=(SELECT id_compra FROM guias_compra "
	  "WHERE numero=%s AND rut_proveedor='%s') AND t1.barcode=t2.barcode AND t2.numero=%s",
	  guia, rut_proveedor, guia

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:5965
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT id_compra=(SELECT id_compra FROM guias_compra WHERE numero=%s AND rut_proveedor='%s') FROM guias_compra WHERE numero=%s AND rut_proveedor='%s'", guia_first, rut, guia_add, rut

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:6007
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT t1.numero, t1.id_compra, date_part ('day', t1.fecha_emicion), date_part ('month', t1.fecha_emicion), date_part ('year', t1.fecha_emicion), (SELECT SUM (t2.cantidad * t2.precio) FROM documentos_detalle AS t2 WHERE t2.numero=t1.numero AND t2.id_compra=t1.id_compra), (SELECT nombre FROM formas_pago WHERE id=(SELECT forma_pago FROM compras WHERE id=t1.id_compra)) FROM guias_compra AS t1 WHERE numero=%s AND rut_proveedor='%s'", string, rut_proveedor

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??buscar proveedores
-- compras.c:6168
create or replace function buscar_proveedor(varchar(100))
returns setof record as '
declare

	list record;
	query text;

begin
query := ''SELECT * FROM proveedores WHERE lower(nombre) LIKE lower('' || quote_literal($1) || '') ''
	''OR rut LIKE '' || quote_literal($1);

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:6306
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT date_part('day', fecha), date_part('month', fecha), "
		      "date_part('year', fecha) FROM compras WHERE id=(SELECT id_compra FROM "
		      "guias_compra WHERE numero=%s AND rut_proveedor='%s')", guia, rut

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:6558
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT SUM (t1.precio * t2.cantidad) AS neto, SUM (t2.iva) AS iva, SUM (t2.otros) AS otros, SUM ((t1.precio * t2.cantidad) + t2.iva + t2.otros) AS total  FROM products_buy_history AS t1, documentos_detalle AS t2 WHERE t1.id_compra=(SELECT id_compra FROM guias_compra WHERE numero=%s AND rut_proveedor='%s') AND t2.numero=%s AND t1.barcode_product=t2.barcode",
					      guia, rut_proveedor, guia

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:7185
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
"SELECT id, (SELECT nombre FROM proveedores WHERE rut=compras.rut_proveedor), rut_proveedor, "
	"(SELECT SUM (cantidad*precio) FROM products_buy_history WHERE id_compra=compras.id) FROM compras "
	"WHERE date_part ('day', fecha)=%d AND date_part ('month', fecha)=%d AND "
	"date_part ('year', fecha)=%d ORDER BY fecha DESC", ventastats->selected_from_day,
	ventastats->selected_from_month, ventastats->selected_from_year

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:7192
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
SELECT id, (SELECT nombre FROM proveedores WHERE rut=compras.rut_proveedor), rut_proveedor, "
	"(SELECT SUM (cantidad*precio) FROM products_buy_history WHERE id_compra=compras.id) FROM compras "
	"WHERE fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND "
	"fecha<=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') ORDER BY fecha DESC",
	ventastats->selected_from_day, ventastats->selected_from_month, ventastats->selected_from_year,
	ventastats->selected_to_day, ventastats->selected_to_month, ventastats->selected_to_year

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- compras.c:7214
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);

begin
query := '''';
"SELECT (SELECT descripcion FROM productos WHERE barcode=barcode_product), cantidad, precio "
			   "FROM products_buy_history WHERE id_compra=%s", PQgetvalue (res, i, 0)

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

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

-- ???
-- credito.c:952
create or replace function ()
returns setof record as '
declare
	list record;
	query varchar(255);
begin
query := '''';
SELECT t2.codigo, t2.descripcion, t2.marca, t1.cantidad, t1.precio "
			  "FROM products_sell_history AS t1, productos AS t2 WHERE "
			  "t1.id_venta=%d AND t2.barcode=t1.barcode", id_venta

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??aqui se hacen varias consultas para obtener datos de los clientes
-- se deberia unificar en una sola consulta
-- en la linea compras.c:1566 se pide un dato de un cliente dado, pues se puede usar la
-- funcion que retorna la ficha de un cliente
-- credito.c:1105, 1566
create or replace function ()
returns setof record as '
declare
	list record;
	query varchar(255);
begin
query := '''';

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

-- ??
-- datos_negocio.c:150
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);
begin
INSERT INTO negocio (razon_social, rut, nombre, fono, fax, direccion, comuna, ciudad, giro, at) "
	  "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')", razon_social_value, rut_value, nombre_fantasia_value,
	  fono_value, fax_value, direccion_value, comuna_value, ciudad_value, giro_value, at_value

RETURN;

END; ' language plpgsql;

-- ??
-- datos_negocio.c:158
create or replace function ()
returns setof record as '
declare

	list record;
	query varchar(255);
begin
UPDATE negocio SET razon_social='%s', rut='%s', nombre='%s', fono='%s', fax='%s', "
	  "direccion='%s', comuna='%s', ciudad='%s', giro='%s', at='%s'", razon_social_value, rut_value, nombre_fantasia_value,
	  fono_value, fax_value, direccion_value, comuna_value, ciudad_value, giro_value, at_value

RETURN;

END; ' language plpgsql;

--
-- datos_negocio.c:176
create or replace function ()
returns setof record as '
declare
	list record;
	query varchar(255);
begin
SELECT razon_social, rut, nombre, fono, fax, direccion, comuna, ciudad, giro, at FROM negocio

return;
end; ' language plpgsql;

--
-- encriptar.c:77
create or replace function (varchar(30),varchar(400))
returns setof record as '
declare
	list record;
	query varchar(255);
begin
query := ''SELECT * FROM users WHERE passwd=md5('' || quote_literal($2) || '')''
	|| '' AND usuario='' || quote_literal($1);

return;
end; ' language plpgsql;

-- ???se pueden hacer en una sola funcion y segun parece anteriormente se puede haber
-- ???hecho alguna funcion anteriormente que puede satisfacerlo
-- factura_more.c:46, 47, 51
create or replace function ()
returns setof record as '
declare
	list record;
	query varchar(255);
begin
query := SELECT nombre || ' ' || apellido_paterno || ' ' || apellido_materno AS name FROM clientes
return;
end; ' language plpgsql

-- ??retorna TODOS los impuestos
-- impuestos.c:48
create or replace function ()
returns setof record as '
declare
	list record;
	query varchar(255);
begin
SELECT * FROM impuestos ORDER BY id

end; ' language plpgsql;

-- inserta un nuevo impuesto
-- impuestos.c:76
create or replace function insert_impuesto(varchar(250), float8)
returns setof record as '
declare
	list record;
	query varchar(255);
begin
INSERT INTO impuestos VALUES (DEFAULT, quote_literal($1), $2);
returns;
end: ' language plpgsql;

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

-- ??
-- manejo_productos.c:37
create or replace function (varchar(14))
returns setof record as '
begin
SELECT codigo, barcode, descripcion, marca, contenido, unidad, precio, fifo, margen_promedio, (SELECT monto FROM impuestos WHERE id=0 AND "
      "productos.impuestos='t'), (SELECT monto FROM impuestos WHERE id=productos.otros), canje , stock_pro, precio_mayor, cantidad_mayor, mayorista FROM productos WHERE barcode='%s'", barcode

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

-- inserta un nuevo documento emitido
-- postgres-functions.c:194
create or replace function insert_documentos_emitidos()
returns setof record as '
begin
INSERT INTO documentos_emitidos (tipo_documento, forma_pago, num_documento, fecha_emision)
					  VALUES (%d, %d, %d, NOW()), document_type, sell_type, get_ticket_number (document_type) + 1)

return;
end; ' language plpgsql;

-- inserta una nueva venta
-- postgres-functions.c:224
create or replace function insert_venta()
returns void as '
begin
INSERT INTO ventas (id, monto, fecha, maquina, vendedor, tipo_documento, tipo_venta, descuento, id_documento, canceled) VALUES "
		"(DEFAULT, %d, NOW(), %d, %d, %d, %d, %s, '%d', '%d')",
		total, machine, seller, tipo_documento, tipo_venta, CUT(discount), id_documento, (gint)canceled

return;
end; ' language plpgsql;

-- retorna el id de la ultima venta
-- postgres-functions:229
create or replace function last_value_venta()
returns int4 as '
declare
	list record;
	query varchar(255);
begin
query:= ''SELECT last_value FROM ventas_id_seq'';
FOR list IN EXECUTE query LOOP
	return list.last_value;
END LOOP;

return;
end; ' language plpgsql;

-- debe retornar los campos pasados por parámetro en la fecha tambien pasada por parametro
-- NPI como hacerlo
-- postgres-functions.c:283
create or replace function ()
returns as '
declare
	list record;
	query varchar(255);
begin
query:= '''';
SELECT %s FROM ventas WHERE "
						"date_part('year', fecha)=%d AND date_part('month', fecha)=%d AND "
						"date_part('day', fecha)=%d ORDER BY fecha DESC",
						fields, from_year, from_month, from_day));
FOR list IN EXECUTE query LOOP
	return list.last_value;
END LOOP;

return;
end; ' language plpgsql;

-- debe retornar los campos pasados por parámetro en el rango de fecha tambien pasado por parametro
-- NPI como hacerlo
-- postgres-functions.c:289
create or replace function ()
returns as '
declare
	list record;
	query varchar(255);
begin
query:= '''';
"SELECT %s FROM ventas WHERE "
						"%s>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND "
						"%s<=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') ORDER BY fecha DESC",
						fields, date_column, from_day, from_month, from_year,
						date_column, to_day+1, to_month, to_year));

FOR list IN EXECUTE query LOOP
	return list.last_value;
END LOOP;

return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:308
create or replace function ()
returns as '
declare
	list record;
	query varchar(255);
begin
query:= '''';
SELECT SUM ((SELECT SUM (cantidad * precio) FROM products_sell_history WHERE id_venta=ventas.id)),
	  "count (*) FROM ventas WHERE fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND "
	  "fecha<to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND (SELECT forma_pago FROM documentos_emitidos "
	  "WHERE id=id_documento)=%d", from_day, from_month, from_year, to_day+1, to_month, to_year, CASH

FOR list IN EXECUTE query LOOP
	return list.last_value;
END LOOP;

return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:329
create or replace function ()
returns as '
declare
	list record;
	query varchar(255);
begin
query:= '''';
SELECT SUM((SELECT SUM(cantidad * precio) FROM products_sell_history WHERE id_venta=ventas.id)), "
	  "count (*) FROM ventas WHERE fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND "
	  "fecha<to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND ((SELECT forma_pago FROM documentos_emitidos "
	  "WHERE id=id_documento)=%d OR (SELECT forma_pago FROM documentos_emitidos WHERE id=id_documento)=%d)",
	  from_day, from_month, from_year, to_day+1, to_month, to_year, CREDITO, TARJETA)

FOR list IN EXECUTE query LOOP
	return list.last_value;
END LOOP;

return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:351
create or replace function ()
returns as '
declare
	list record;
	query varchar(255);
begin
query:= '''';
SELECT SUM((SELECT SUM(cantidad * precio) FROM products_sell_history WHERE "
					  "id_venta=ventas.id)), count (*) FROM ventas WHERE "
					  "fecha>=to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY') AND "
					  "fecha<to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')",
					  from_day, from_month, from_year, to_day+1, to_month, to_year

FOR list IN EXECUTE query LOOP
	return list.last_value;
END LOOP;

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

-- inserta una nueva deuda
-- postgres-functions:403
create or replace function insert_deuda(int4, int4, int4)
returns boolean as '
begin
INSERT INTO deudas VALUES (DEFAULT, $1, $2, $3, DEFAULT, DEFAULT);

return;
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

-- retorna las deudas de un cliente
-- postgres-functions.c:429
create or replace function deudas_cliente(int4)
returns setof record as '
declare
	list record;
	query text;
begin
query:= ''SELECT id, monto, maquina, vendedor, date_part('day', fecha), date_part('month', fecha), "
	  "date_part('year', fecha), date_part('hour', fecha), date_part('minute', fecha), "
	  "date_part ('second', fecha) FROM ventas WHERE id IN (SELECT id_venta FROM deudas WHERE "
	  "rut_cliente=%d AND pagada='f')",rut'';

FOR list IN EXECUTE query LOOP
	return next list;
END LOOP;

return;
end; ' language plpgsql;

-- marca como pagada una deuda dada
-- postgres-functions.c:442
create or replace function pagar_deuda(int4)
returns void as '
begin
UPDATE deudas SET pagada='t' WHERE id_venta=$1;
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

-- ??
-- postgres-functions:457
create or replace function (int4,int4)
returns void as '
begin
SELECT * FROM ventas WHERE id IN "
									  "(SELECT id_venta FROM deudas WHERE rut_cliente=%d AND pagada='f') ORDER BY fecha asc", rut
return;
end; ' language plpgsql;

-- retorna la ficha de un cliente dado
-- postgres-functions:493, 520, 531, 541, 551, 562, 578
create or replace function obtener_cliente(int4)
returns setof clientes as '
declare
	list record;
	query varchar(255);
begin
query:= ''select * from clientes where rut = '' || quote_literal($1);

for list in execute query loop
	return next list;
end loop
return;
end; ' language plpgsql;

-- actualiza los abonado por un cliente
-- postgres-functions:507
create or replace function set_deuda_cliente(int4,int4)
returns void as '
declare
	rut_cliente alias for $1;
	nueva_deuda alias for $2
begin
update clientes set abonado=nueva_deuda where rut=rut_cliente;
return;
end; ' language plpgsql;

-- retorna la password de un usuario dado
-- postgres-functions.c:590, 603, 616, 626, 635,
create or replace function select_usuario(varchar(30))
returns setof users as '
declare
	list record;
	query varchar(255);
begin
query:= ''SELECT * FROM users WHERE usuario='' || quote_literal($1);

for list in execute query loop
	return next list;
end loop
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

-- retorna true cuando ya existe un vendedor/usuario con el id dado
-- postgres-functions.c:664, 698
create or replace function existe_usuario(int4)
returns boolean as '
declare
	list record;
	query varchar(255);
begin
query := ''SELECT count(*) as suma FROM users WHERE id='' || $1;

for list in execute query loop
	if list.suma > 0 then
		return TRUE;
	else
		return FALSE;
	end if
end loop

return TRUE;
end; ' language plpgsql;

-- inserta un nuevo usuario
-- FIXME: revisar como hacerlo para dar la opcion de usar un id DEFAULT o un id pasado por parámetro
-- postgres-functions.c:677
create or replace function insert_usuario()
returns void as '
begin
INSERT INTO users VALUES (%s, '%s', md5('%s'), %s, '%s', '%s', '%s', NOW(), 1)",
	  strcmp (id, "") != 0 ? id : "DEFAULT", username, passwd, rut, nombre, apell_p, apell_m)

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

-- actualiza la información de un producto
-- postgres-functions.c:733
create or replace function update_producto(varchar(14),varchar(10), varchar(50), int4)
begin
UPDATE productos SET codigo=quote_literal($2),
			descripcion=quote_literal($3),
			precio=quote_literal($4)
			WHERE barcode=quote_literal($1);
return;
end; ' language plpgsql;

-- ??debe actualizar un producto
-- postgres-functions.c:750
create or replace function update_producto_completo()
begin
"UPDATE productos SET codigo='%s', descripcion='%s', marca='%s', unidad='%s', contenido='%s', precio=%d, "
					  "impuestos='%d', otros=(SELECT id FROM impuestos WHERE descripcion='%s'), perecibles='%d', fraccion='%d' WHERE barcode='%s'",
					  codigo, SPE(description), SPE(marca), unidad, contenido, atoi (precio),
					  iva, otros, (gint)perecible, (gint)fraccion, barcode
return;
end; ' language plpgsql;

-- inserta un nuevo producto con casi todos los campos
-- FIXME: hay que definir los parámetros correctamente y armar la sentencia en base a ellos
-- postgres-functions.c:764
create or replace function insert_producto()
returns void as '
begin
INSERT INTO productos (codigo, barcode, descripcion, marca, contenido, unidad, stock, precio, fifo, vendidos, "
					  "impuestos, otros, perecibles, stock_min, margen_promedio, merma_unid, fraccion) VALUES ('%s', '%s', upper('%s'), upper('%s'), "
					  "'%s', upper('%s'), '0', 0, 0, 0, '%d', (SELECT id FROM impuestos WHERE descripcion='%s'), '%d', 0, 0, 0, '%d')",
					  codigo, barcode, SPE(description), SPE(marca), contenido, unidad, iva, otros, perecible, fraccion
return;
end; ' language plpgsql;

-- inserta una nueva compra, y devuelve el id de la compra
-- postgres-functions.c:778
create or replace function insert_compra(varchar(100), varchar(100), int2)
returns int4 as '
declare
	l record;
	q varchar(255);
begin
INSERT INTO compras VALUES (DEFAULT, NOW(), quote_literal($1),quote_literal($2), $3, FALSE, FALSE);

q := ''SELECT last_value FROM compras_id_seq'';

for l in execute q loop
	return l.last_value;
end loop;

return -1;
end; ' language plpgsql;

-- inserta el detalle de una compra
-- postgres-functions.c:808, 814
create or replace function insert_detalle_compra(int4, float8, float8, int4, varchar(14), int4, int4, int4)
returns void as '
begin
INSERT INTO products_buy_history VALUES (DEFAULT, $1, $2, $3, $4, 0, 0, quote_literal($5),$6, $7, $8);
UPDATE productos SET precio=$4 WHERE barcode=quote_literal($5);
return;
end;' language plpgsql;

-- marca una compra como ingresa
-- postgres-functions.c:827
create or replace function ingresa_compra()
returns void as '
begin
UPDATE compras SET ingresada=TRUE WHERE id IN (SELECT id_compra FROM products_buy_history WHERE cantidad=cantidad_ingresada) AND ingresada=FALSE;
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:854
create or replace function ()
returns void as '
begin
INSERT INTO documentos_detalle (id, numero, id_compra, barcode, cantidad, precio, fecha, factura, elaboracion, vencimiento, iva, otros) "
		"VALUES (DEFAULT, %d, %d, '%s', %s, %s, NOW(), '%d', NULL, to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY'), %d, %d)",
		doc, compra, product->barcode, cantidad, CUT (g_strdup_printf ("%.2f", product->precio_neto)),
		(gint)factura, product->venc_day, product->venc_month, product->venc_year, lround (iva), lround (otros)

return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:861
create or replace function ()
returns void as '
begin
INSERT INTO documentos_detalle (id, numero, id_compra, barcode, cantidad, precio, fecha, factura, elaboracion, vencimiento, iva, otros) "
		"VALUES (DEFAULT, %d, %d, '%s', %s, %s, NOW(), '%d', NULL, NULL, %d, %d)",
		doc, compra, product->barcode, cantidad, CUT (g_strdup_printf ("%.2f", product->precio_neto)),(gint)factura, lround (iva), lround (otros)

return;
end; ' language plpgsql;

-- ingresa mercaderia
-- postgres-functions.c:892
create or replace function ()
returns void as '
begin
UPDATE products_buy_history SET cantidad_ingresada=cantidad_ingresada+%s, canjeado=%s "
					  "WHERE barcode_product IN (SELECT barcode  FROM productos WHERE barcode='%s'"
					  " AND id_compra=%d)", cantidad, CUT (g_strdup_printf ("%.2f", canjeado)),product->barcode, compra

return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:914
create or replace function ()
returns void as '
begin
UPDATE productos SET margen_promedio=%d, fifo=%d, stock=stock+%s, stock_pro=%s WHERE barcode='%s'

return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:919
create or replace function ()
returns void as '
begin
UPDATE productos SET margen_promedio=%d, fifo=%d, stock=stock+%s WHERE barcode='%s'",
		lround (margen_promedio), fifo, cantidad, product->barcode

return;
end; ' language plpgsql;

-- setea el stock de un producto
-- postgres-functions.c:944
create or replace function set_stock_producto(varchar(14), float8)
returns void as '
begin
UPDATE productos SET stock=$2 WHERE barcode=quote_literal($1);
return;
end; ' language plpgsql;

-- ??la funcion FiFo podria hacer dentro de la base de datos y retornar el valor
-- postgres-functions.c:1009
create or replace function ()
returns void as '
begin
SELECT cantidad, precio, cantidad_ingresada FROM products_buy_history, compras WHERE "
	  "barcode_product='%s' AND compras.id=%d AND products_buy_history.id_compra=%d ORDER BY compras.fecha DESC
return;
end; ' language plpgsql;

-- ??cuando se venden productos se ejecuta esta operacion (revisar codigo para reducir llamadas)
-- postgres-functions.c:1075
create or replace function (varchar(14), float8, float8)
returns void as '
begin
UPDATE productos SET vendidos=vendidos+$2, stock=$2 WHERE barcode=quote_literal($1);
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1091
create or replace function ()
returns void as '
begin
INSERT INTO products_sell_history VALUES(DEFAULT, %d, '%s', '%s', '%s', '%d', '%s',"
		  "'%s', %d, %d, %d, %d)", id_venta,
		  products->product->barcode, SPE(products->product->producto), SPE(products->product->marca),
		  products->product->contenido, products->product->unidad, cantidad, precio,
		  products->product->fifo, lround (iva), lround (otros)
return;
end; ' language plpgsql;

-- ??retorna el ranking de productos
-- postgres-functions.c:1111
create or replace function obtener_rankin_productos()
returns void as '
begin
SELECT t1.descripcion, t1.marca, (SELECT contenido FROM productos WHERE "
	  "barcode=t1.barcode),(SELECT unidad FROM productos WHERE barcode=t1.barcode), "
	  "SUM(t1.cantidad), SUM((t1.cantidad*t1.precio)::integer), SUM ((t1.cantidad*t1.fifo)::integer), "
	  "SUM(((precio*cantidad)-((iva+otros)+(fifo*cantidad)))::integer) FROM products_sell_history AS"
	  " t1 WHERE id_venta IN (SELECT id FROM ventas WHERE fecha>=to_timestamp ('%.2d %.2d %.4d', "
	  "'DD MM YYYY') AND fecha<to_timestamp ('%.2d %.2d %.4d', 'DD MM YYYY')) GROUP BY "
	  "t1.descripcion, t1.marca, t1.barcode",
	  from_day, from_month, from_year, to_day+1, to_month, to_year
return;
end; ' language plpgsql;

-- ??inserta un nuevo proveedor a la base de datos
-- postgres-functions.c:1133
create or replace function insert_proveedor()
returns void as '
begin
INSERT INTO proveedores VALUES (DEFAULT, '%s', '%s', '%s', "
					  "'%s', '%s', %d, '%s', '%s', '%s', '%s')", nombre, rut, direccion, ciudad,
					  comuna, atoi (telefono), email, web, contacto, giro
return;
end; ' language plpgsql;

-- setea los productos ingresados
-- postgres-functions.c:1149
create or replace function set_productos_ingresados()
returns void as '
begin
UPDATE products_buy_history SET ingresado='t' WHERE cantidad_ingresada=cantidad;
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1164
create or replace function ()
returns void as '
begin
"SELECT date_part ('day', (SELECT now() - fecha FROM compras WHERE id=t1.id_compra)) "
		  "FROM products_buy_history AS t1, productos AS t2, compras AS t3 WHERE t2.barcode='%s' AND "
		  "t1.barcode_product='%s' AND t3.id=t1.id_compra ORDER BY t3.fecha ASC", barcode, barcode
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1178
create or replace function ()
returns void as '
begin
SELECT t1.stock / (vendidos / %d) FROM productos AS t1 WHERE t1.barcode='%s'
return;
end; ' language plpgsql;

-- ??inserta un nuevo cheque
-- postgres-functions.c:1213
create or replace function ()
returns void as '
begin
INSERT INTO cheques VALUES (DEFAULT, %d, '%s', %d, '%s', '%s', %d, "
					  "to_timestamp('%.2d %.2d %.4d', 'DD MM YYYY'))", id_venta, serie, number,
					  banco, plaza, monto, day, month, year
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:
create or replace function (int4)
returns int4 as '
begin
SELECT count(*) FROM products_buy_history WHERE id_compra=quote_literal($1) AND cantidad_ingresada>0 AND cantidad_ingresada<cantidad
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1243
create or replace function ()
returns void as '
begin
SELECT * FROM proveedores WHERE rut=(SELECT rut_proveedor FROM compras"
									  " WHERE id=%d", id_compra
return;
end; ' language plpgsql;

-- ??inserta una nueva factura de compra de productos
-- postgres-functions.c:1271
create or replace function insert_factura_compra()
returns void as '
begin
INSERT INTO facturas_compras (id, id_compra, rut_proveedor, num_factura, fecha, valor_neto,"
	  " valor_iva, descuento, pagada, monto) VALUES (DEFAULT, %d, '%s', '%s', "
	  "to_timestamp('%.2d %.2d %.2d', 'DD MM YY'), 0, 0, 0,'f', %d)",
	  id_compra, rut_proveedor, n_doc, atoi (d_emision), atoi (m_emision), atoi (y_emision),
	  total)
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1281
create or replace function ()
returns void as '
begin
UPDATE facturas_compras SET forma_pago=(SELECT compras.forma_pago FROM compras WHERE id=%d)"
		  " WHERE id_compra=%d", id_compra, id_compra)
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1285
create or replace function ()
returns void as '
begin
UPDATE facturas_compras SET fecha_pago=DATE(fecha)+(forma_pago) WHERE id_compra=%d", id_compra
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1292
create or replace function ()
returns void as '
begin
UPDATE facturas_compras SET forma_pago=(SELECT compras.forma_pago FROM compras WHERE id=(SELECT "
		  "id_compra FROM guias_compra WHERE numero=%d AND rut_proveedor='%s')) WHERE num_factura='%s' AND "
		  "rut_proveedor='%s'", guia, rut_proveedor, n_doc, rut_proveedor)
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1298
create or replace function ()
returns void as '
begin
UPDATE facturas_compras SET fecha_pago=DATE(fecha)+(forma_pago) WHERE num_factura='%s'", n_doc
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:
create or replace function insert_guia_compra()
returns void as '
begin
"INSERT INTO guias_compra VALUES (DEFAULT, %s, %d, 0, (SELECT rut_proveedor FROM compras WHERE id=%d), "
									  "to_timestamp('%.2d %.2d %.2d', 'DD MM YY'))",
									  n_doc, id_compra, id_compra, atoi (d_emision), atoi (m_emision), atoi (y_emision)
return;
end; ' language plpgsql;

-- ??asigna una factura a una guia
-- postgres-functions.c:1334
create or replace function asignar_factura_a_guia()
returns void as '
begin
UPDATE guias_compra SET id_factura=%d WHERE numero=%d
return;
end; ' language plpgsql;

-- ??retorna los 'otros' impuestos de unproducto
-- postgres-functions.c:1365, 1398, 1496
create or replace function obtener_otros_impuestos(varchar(14))
returns setof record as '
begin
SELECT impuestos.monto, impuestos.descripcion FROM productos, impuestos WHERE productos.barcode='%s' AND impuestos.id=productos.otros
return;
end; ' language plpgsql;

-- ??obtener id de otros impuestos de un productos dado
-- postgres-functions.c:1382
create or replace function (varchar(14))
returns void as '
begin
SELECT otros FROM productos WHERE barcode='%s'", barcode
return;
end; ' language plpgsql;

-- ??retorna el prrecio neto de un producto
-- postgres-functions.c:1414
create or replace function (varchar(14))
returns void as '
begin
SELECT  precio FROM products_buy_history WHERE "
									  "barcode_product='%s' AND id_compra IN (SELECT id FROM "
									  "compras ORDER BY fecha DESC)", barcode
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1448
create or replace function ()
returns void as '
begin
SELECT * from products_buy_history WHERE id_compra=%s AND cantidad_ingresada<cantidad", compra)
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1464
create or replace function ()
returns void as '
begin
"SELECT * from products_buy_history WHERE id_compra=%s AND barcode_product='%s' AND cantidad_ingresada<cantidad", compra, barcode))
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1513
create or replace function ()
returns void as '
begin
SELECT nombre FROM familias WHERE id=(SELECT familia FROM productos WHERE barcode='%s')", barcode
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1555
create or replace function ()
returns void as '
begin
SELECT SUM(precio * cantidad) FROM products_buy_history "
									  "WHERE barcode_product='%s'", barcode)
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1572
create or replace function ()
returns void as '
begin
SELECT cantidad, date_part ('day', elaboracion), "
					  "date_part ('month', elaboracion), date_part('year', elaboracion) FROM "
					  "documentos_detalle WHERE barcode='%s' ORDER BY fecha ASC", barcode)
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1614
create or replace function ()
returns void as '
begin
SELECT cantidad, date_part ('day', vencimiento), "
					  "date_part ('month', vencimiento), date_part('year', vencimiento) FROM "
					  "documentos_detalle WHERE barcode='%s' ORDER BY fecha DESC", barcode
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1656
create or replace function ()
returns void as '
begin
"SELECT precio, cantidad_ingresada, (precio * cantidad_ingresada)::integer FROM products_buy_history "
									  "WHERE barcode_product='%s'", barcode
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1688
create or replace function ()
returns void as '
begin
"SELECT SUM (fifo * stock)::integer FROM productos
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1701
create or replace function ()
returns void as '
begin
"SELECT SUM (precio * stock)::integer FROM productos"
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1714
create or replace function ()
returns void as '
begin
SELECT SUM (round (fifo * (margen_promedio / 100))  * stock)::integer FROM productos
return;
end; ' language plpgsql;

-- ??actualiza un producto
-- postgres-functions.c:1729
create or replace function ()
returns void as '
begin
UPDATE productos SET stock_min=%s, margen_promedio=%s, precio=%s, canje='%d', tasa_canje=%d, "
					  "precio_mayor=%d, cantidad_mayor=%d, mayorista='%d' WHERE barcode='%s'",
					  stock_minimo, margen, new_venta, (gint)canjeable, tasa, precio_mayorista, cantidad_mayorista,
					  (gint)mayorista, barcode
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1741
create or replace function ()
returns void as '
begin
INSERT INTO egresos VALUES (DEFAULT, %d, (SELECT id FROM tipo_egreso WHERE descrip='%s'), NOW(), %d)",
									  monto, motivo, usuario)
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1755
create or replace function ()
returns void as '
begin
INSERT INTO tarjetas VALUES (DEFAULT, %d, '%s', '%s', '%s')",
					  id_venta, insti, numero, fecha_venc)
return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1770
create or replace function ()
returns void as '
begin
"INSERT INTO ingresos VALUES (DEFAULT, %d, (SELECT id FROM tipo_ingreso WHERE descrip='%s'), NOW(), %d)",
									  monto, motivo, usuario)
return;
end; ' language plpgsql;

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

-- ??define la asistencia
-- postgres-functions.c:1837
create or replace function set_asistencia(userid, entrando boolean)
returns void as '
begin
if entrando then
	INSERT INTO asistencia VALUES (DEFAULT, userid, NOW(), to_timestamp('0','0'));
else
	UPDATE asistencia SET salida=NOW() WHERE id=(SELECT id FROM asistencia WHERE salida=to_timestamp('0','0') AND id_user=userid ORDER BY entrada DESC LIMIT 1);
end if;
return;
end; ' language plpgsql;

-- inserta una nueva forma de pago
-- postgres-functions.c:1875
create or replace function insert_forma_de_pago(varchar(50), int2)
returns void as '
begin
INSERT INTO formas_pagos VALUES (DEFAULT, quote_literal($1), $2);
return;
end; ' language plpgsql;

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

-- ??
-- postgres-functions.c:1924
create or replace function (prod varchar(14), cant float8)
returns void as '
begin
UPDATE productos SET stock=stock-cant WHERE barcode=quote_literal(prod);
INSERT INTO devoluciones (barcode_product, cantidad) VALUES (quote_literal(prod), cant);

return;
end; ' language plpgsql;

-- ??
-- postgres-functions:1944
create or replace function (prod varchar(14), cant float8)
returns void as '
begin
UPDATE productos SET stock=stock+cant WHERE barcode=quote_literal(prod);
UPDATE devoluciones SET cantidad_recivida=cant, devuelto=TRUE WHERE id=(SELECT id FROM devoluciones WHERE barocde_product=quote_literal(prod) AND devuelto=FALSE);

return;
end; ' language plpgsql;

-- ??
-- postgres-functions.c:1965
create or replace function update_proveedor()
returns void as '
begin
UPDATE proveedores SET nombre='%s', direccion='%s', ciudad='%s', "
					  "comuna='%s', telefono='%s', email='%s', web='%s', contacto='%s', giro='%s'"
					  " WHERE rut='%s'", razon, direccion, ciudad,
					  comuna, fono, email, web, contacto, giro, rut)

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
create or replace function select_proveedor()
returns setof record as '
declare
	l record;
	q varchar(255);
begin
q := ''SELECT * FROM proveedores ORDER BY nombre ASC'';

for l in execute q loop
	return next l;
end loop;
return;
end; ' language plpgsql;

-- retorna el proveedor con el rut dado
-- proveedores.c:130, 186
-- ventas_stats.c:118
create or replace function select_proveedor(varchar(20))
returns setof record as '
declare
	l record;
	q varchar(255);
begin
q := ''SELECT * FROM proveedores WHERE rut='' || quote_literal($1);

for l in execute q loop
	return next l;
end loop;
return;
end; ' language plpgsql;

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

-- busca productos
-- ventas.c:3401
create or replace function buscar_producto(patron varchar(100), es_venta boolean)
returns setof record as '
declare
l record;
q varchar(255);
begin
if es_venta then
	q:= ''SELECT * FROM productos WHERE stock>0 AND lower (descripcion) LIKE lower(''
	|| quote_literal(patron) || '') OR lower(marca) LIKE lower(''
	|| quote_literal(patron= || '') ORDER BY descripcion ASC'';
else
	q:= ''SELECT * FROM productos WHERE stock>0 AND lower (descripcion) LIKE lower(''
	|| quote_literal(patron) || '') OR lower(marca) LIKE lower(''
	|| quote_literal(patron= || '') WHERE canje=TRUE ORDER BY descripcion ASC'';
end if

for l in execute q loop
	return next l;
end loop;

return;
end;' language plpgsql;


-- busca productos
-- ventas.c:3407
create or replace function buscar_producto(es_venta boolean)
returns setof record as '
declare
l record;
q varchar(255);
begin
if es_venta then
	q:= ''SELECT * FROM productos WHERE stock>0'';
else
	q:= ''SELECT * FROM productos WHERE stock>0 AND CANJE = TRUE'';
end if

for l in execute q loop
	return next l;
end loop;

return;
end;' language plpgsql;

-- ??
-- ventas.c3916
create or replace function ()
returns setof record as '
declare
l record;
q varchar(255);
begin
SELECT ventas.id, monto, users.usuario FROM ventas "
						"INNER JOIN users ON users.id = ventas.vendedor "
						"WHERE ventas.id = %s AND canceled = FALSE",
						text
for l in execute q loop
	return next l;
end loop;

return;
end;' language plpgsql;

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
END;' language plpgsql

-- ??
-- ventas.c:4100
create or replace function ()
returns setof record as '
declare
l record;
q varchar(255);
begin

SELECT ventas.id, users.usuario,monto FROM ventas "
						"INNER JOIN users ON users.id = ventas.vendedor "
						"WHERE date_trunc('day',fecha) = '%.4d-%.2d-%.2d' AND canceled = FALSE",
						year,month+1,day

return;

end; ' language plpgsql;

-- ??
-- ventas.c:4158
create or replace function ()
returns setof record as '
declare
l record;
q varchar(255);
begin
SELECT descripcion || marca, cantidad, (precio * cantidad) "
							"FROM products_sell_history WHERE id_venta=%s",
							id_venta
return;

end; ' language plpgsql;

-- ??
-- ventas_stats.c:87
create or replace function ()
returns setof record as '
declare
l record;
q varchar(255);
begin

SELECT descripcion, marca, contenido, unidad, (SELECT SUM(cantidad_ingresada) FROM products_buy_history WHERE barcode_product=barcode),vendidos FROM productos WHERE barcode IN (SELECT barcode_product FROM products_buy_history WHERE id_compra IN (SELECT id FROM compras WHERE rut_proveedor='%s') AND anulado='f' GROUP BY barcode_product)", rut

return;

end; ' language plpgsql;

-- ??
-- ventas_stats.c:1933
create or replace function ()
returns setof record as '
declare
l record;
q varchar(255);
begin

SELECT descripcion, marca, contenido, unidad, cantidad, precio, (cantidad * precio) AS "
	  "monto FROM products_sell_history WHERE id_venta=%s", idventa
return;

end; ' language plpgsql;

-- ??
-- ventas_stats.c:1963
create or replace function ()
returns setof record as '
declare
l record;
q varchar(255);
begin
SELECT inicio, termino, date_part('day',fecha_inicio), date_part('month',fecha_inicio), "
		     "date_part('year',fecha_inicio), date_part('day',fecha_termino), date_part('month',fecha_termino), "
		     "date_part('year',fecha_termino), perdida FROM caja"
return;

end; ' language plpgsql;

-- ??
-- ventas_stats.c:2048
create or replace function ()
returns setof record as '
declare
l record;
q varchar(255);
begin
SELECT nombre, rut, (SELECT SUM (cantidad_ingresada) FROM products_buy_history WHERE id_compra IN (SELECT id FROM compras WHERE rut_proveedor=proveedores.rut)), (SELECT SUM (cantidad_ingresada * precio) FROM products_buy_history WHERE id_compra IN (SELECT id FROM compras WHERE rut_proveedor=proveedores.rut))::integer, (SELECT SUM (margen) / COUNT (*) FROM products_buy_history WHERE id_compra IN (SELECT id FROM compras WHERE rut_proveedor=proveedores.rut))::integer, (SELECT SUM (precio_venta - (precio * (margen / 100) +1))  FROM products_buy_history WHERE id_compra IN (SELECT id FROM compras WHERE rut_proveedor=proveedores.rut)) FROM proveedores

return;

end; ' language plpgsql;


-- ??
-- ventas_stats.c:2088
create or replace function ()
returns setof record as '
declare
l record;
q varchar(255);
begin
SELECT t1.id, t1.num_factura, t1.monto, date_part ('day', t1.fecha), date_part('month', t1.fecha), date_part('year', t1.fecha), t1.id_compra, date_part ('day', fecha_pago) AS pay_day, date_part ('month', fecha_pago) AS pay_month, date_part ('year', fecha_pago) AS pay_year, t1.forma_pago, t1.id, t1.rut_proveedor FROM facturas_compras AS t1 WHERE pagada='f' ORDER BY pay_year, pay_month, pay_day ASC"
return;

end; ' language plpgsql;

-- ??
-- ventas_stats.c:2138
create or replace function ()
returns setof record as '
declare
l record;
q varchar(255);
begin
SELECT numero, id_compra, id, date_part ('day', fecha_emicion), "
	  "date_part ('month', fecha_emicion), date_part ('year', fecha_emicion), rut_proveedor FROM "
	  "guias_compra WHERE id_factura=%s", PQgetvalue (res, i, 0)
return;

end; ' language plpgsql;

-- ??
-- ventas_stats.c:2177
create or replace function ()
returns setof record as '
declare
l record;
q varchar(255);
begin
SELECT t1.id, t1.num_factura, t1.monto, date_part ('day', t1.fecha), date_part('month', t1.fecha), date_part('year', t1.fecha), t1.id_compra, date_part ('day', fecha_pago) AS pay_day, date_part ('month', fecha_pago) AS pay_month, date_part ('year', fecha_pago) AS pay_year, t1.forma_pago, t1.id, t1.rut_proveedor FROM facturas_compras AS t1 WHERE pagada='t' ORDER BY pay_year, pay_month, pay_day ASC
return;

end; ' language plpgsql;

-- ??
-- ventas_stats.c:2227
create or replace function ()
returns setof record as '
declare
l record;
q varchar(255);
begin
SELECT numero, id_compra, id, date_part ('day', fecha_emicion), "
	  "date_part ('month', fecha_emicion), date_part ('year', fecha_emicion), rut_proveedor FROM "
	  "guias_compra WHERE id_factura=%s", PQgetvalue (res, i, 0)
return;

end; ' language plpgsql;

