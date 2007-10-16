-- estas son las funciones que deben ir en la base de datos
-- algunas no estan con buena sintaxis

-- revisa si hay devoluciones de un producto dado
-- administracion_productos.c:123
create or replace function hay_devolucion(varchar(14))
returns setof record as '
declare
	prod ALIAS FOR $1;
	query varchar(255);
	list record;

begin
query := ''SELECT id FROM devoluciones WHERE barcode_product=''
	|| quote_literal(prod)
	|| '' AND devuelto=FALSE'';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- revisa si se puede devolver el producto
-- administracion_productos.c:130
create or replace function puedo_devolver(float8, varchar(14))
returns setof record as '
declare
	num_prods ALIAS FOR $1;
	prod ALIAS FOR $2;
	list record;
	query varchar(255);

begin
query := ''SELECT cantidad<'' || num_prods
	|| '' FROM devoluciones WHERE id=(SELECT id FROM devoluciones WHERE barcode_product=''
	|| quote_literal(prod)
	|| '' AND devuelto=FALSE)'';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- retorna el maximo de productos que se puede devolver
-- administracion_productos.c:136
create or replace function max_prods_a_devolver(varchar(14))
returns setof record as '
declare
	prod ALIAS FOR $1;
	list record;
	query varchar(255);

begin
query := ''SELECT cantidad FROM devoluciones WHERE id= ''
	|| ''(SELECT id FROM devoluciones WHERE barcode_product=''
	|| quote_literal(prod) || '' AND devuelto=FALSE)'';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- retorna los tipos de merma existentes
-- administracion_productos.c:396
create or replace function get_tipos_merma()
returns setof record as '
declare
	
	list record;
	query varchar(255);

begin
query := ''SELECT * FROM tipo_merma'';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- inserta un nuevo producto
-- administracion_productos.c:1351
create or replace function insert_producto(varchar(255),varchar(255),varchar(255))
returns void as '
begin
INSERT INTO productos VALUES (DEFAULT, quote_literal($1), quote_literal($2), quote_literal($3));
RETURN;
END; ' language plpgsql

-- revisa si existe un producto con el mismo código
-- administracion_productos.c:1354
create or replace function existe_producto_por_codigo(varchar(10))
returns boolean as '
declare
	
	list record;
	query varchar(255);

begin
query := ''SELECT count(codigo) as suma FROM productos WHERE codigo='' || quote_literal($1) ;


FOR list IN EXECUTE query LOOP
	if list.suma > 0 then 
		return TRUE;
	end if;
END LOOP;

RETURN FALSE;

END; ' language plpgsql;

-- revisa si existe un producto con el mismo nombre
-- administracion_productos.c:1356
create or replace function existe_producto_por_nombre()
returns boolean as '
declare
	
	list record;
	query varchar(255);

begin
query := ''SELECT codigo FROM productos WHERE producto='' || quote_literal($1) ;


FOR list IN EXECUTE query LOOP
	if list.suma > 0 then 
		return TRUE;
	end if;
END LOOP;

RETURN FALSE;

END; ' language plpgsql;

-- retorna TODOS los productos
-- administracion_productos.c:1376
create or replace function select_productos()
returns setof record as '
declare
	
	list record;
	query varchar(255);

begin
query := ''select * from productos'';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- administracion_productos.c:1464
create or replace function ()
returns setof record as '
declare
	
	list record;
	query varchar(255);

begin
SELECT date_part ('day', (SELECT NOW() - fecha FROM compras WHERE id=t1.id_compra)) "
	  "FROM products_buy_history AS t1, productos AS t2, compras AS t3 WHERE t2.barcode='%s' "
	  "AND t1.barcode_product='%s' AND t3.id=t1.id_compra ORDER BY t3.fecha ASC", 
	  barcode, barcode)
query := '''';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- administracion_productos.c:1479
create or replace function ()
returns setof record as '
declare
	
	list record;
	query varchar(255);

begin
SELECT codigo, descripcion, marca, contenido, unidad, stock, precio, fifo, stock_min, margen_promedio, merma_unid, (SELECT SUM ((cantidad * precio) - (iva + otros + (fifo * cantidad))) FROM products_sell_history WHERE barcode=productos.barcode), (productos.vendidos / %d) AS merma, vendidos, (SELECT SUM ((cantidad * precio) - (iva + otros)) FROM products_sell_history WHERE barcode=productos.barcode), canje, stock_pro, tasa_canje, precio_mayor, cantidad_mayor, mayorista FROM productos WHERE barcode='%s'", day, barcode)
query := '''';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- administracion_productos.c:1637
create or replace function ()
returns setof record as '
declare
	
	list record;
	query varchar(255);

begin
"SELECT nombre FROM proveedores WHERE rut IN (SELECT rut_proveedor FROM compras WHERE id IN (SELECT id_compra FROM products_buy_history WHERE barcode_product='%s')) GROUP BY nombre", barcode)
query := '''';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

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
create or replace function obtener_producto(varchar(14))
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
returns setof record as '
declare
	
	list record;
	query varchar(255);

begin
query := ''SELECT num_boleta FROM numeros_documentos'';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- retorna los numeros de factura
-- boleta.c:39
create or replace function obtener_num_factura()
returns setof record as '
declare
	
	list record;
	query varchar(255);

begin
query := ''SELECT num_factura FROM numeros_documentos'';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- retorna los numeros de guia
-- boleta.c:42
create or replace function obtener_num_guias()
returns setof record as '
declare
	
	list record;
	query varchar(255);

begin
query := ''SELECT num_guias FROM numeros_documentos'';


FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- actualiza el numero de boleta
-- boleta.c:62
create or replace function update_num_boleta(int4)
returns void as '
begin
UPDATE numeros_documentos SET num_boleta=$1

RETURN;

END; ' language plpgsql;

-- actualiza el numero de factura
-- boleta.c:65
create or replace function update_num_factura(int4)
returns void as '
begin
UPDATE numeros_documentos SET num_factura=$1

RETURN;

END; ' language plpgsql;

-- actualiza el numero de guias
-- boleta.c:68
create or replace function update_num_guias(int4)
returns void as '
begin
UPDATE numeros_documentos SET num_guias=$1

RETURN;

END; ' language plpgsql;

-- ??
-- caja.c:63
create or replace function ()
returns setof record as '
declare
	
	list record;
	query varchar(255);

begin
query := '''';
SELECT (SELECT inicio FROM caja WHERE date_trunc ('day', fecha_inicio)=date_trunc('day', t1.fecha_inicio)) AS inicio, (SELECT SUM (monto) FROM ventas WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio) AND tipo_venta=%d) AS ventas_efect, (SELECT SUM(t1.monto) FROM cheques AS t1 WHERE id_venta IN (SELECT id FROM ventas WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio))) AS ventas_doc, (SELECT SUM(monto_abonado) FROM abonos WHERE date_trunc('day', fecha_abono)=date_trunc('day', fecha_inicio)) AS pago_credit, (SELECT SUM(monto) FROM ingresos WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio)) AS otros, (SELECT SUM (monto) FROM egresos WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio) AND tipo=1) AS retiros, (SELECT SUM (monto) FROM egresos WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio) AND tipo=3) AS gastos, (SELECT SUM (monto) FROM egresos WHERE date_trunc('day', fecha)=date_trunc('day', fecha_inicio) AND tipo=2) AS otros_egresos, (SELECT SUM(monto) FROM facturas_compras WHERE id IN (SELECT id_fact FROM pagos  WHERE caja='t' AND date_trunc('day', fecha)=date_trunc('day', fecha_inicio))) AS facturas, fecha_inicio FROM caja AS t1 WHERE t1.fecha_termino=to_timestamp('DD-MM-YY', '00-00-00')", CASH

FOR list IN EXECUTE query LOOP
	RETURN NEXT list;
END LOOP;

RETURN;

END; ' language plpgsql;

-- ??
-- caja.c:86
create or replace function ()
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
create or replace function select_cliente()
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
