#!/bin/bash

BD_NAME=$1
BD_USER=$2
BD_HOST=$3
BD_PORT=$4


echo -- tarjetas
echo tarjetas > /dev/stderr
echo COPY tarjetas\(id, id_venta, insti, numero, fecha_venc\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_venta, insti, numero, fecha_venc from tarjetas" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\n",$1,$2,$3,$4,$5)}'
echo \\.


echo -- tipo_egreso
echo tipo_egreso > /dev/stderr
echo COPY tipo_egreso \(id,descrip\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, descrip from tipo_egreso" | awk -F'|' '{printf ("%s\011%s\n",$1,$2)}'
echo \\.

echo -- tipo_ingreso
echo tipo_ingreso > /dev/stderr
echo COPY tipo_ingreso \(id, descrip\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, descrip from tipo_ingreso" | awk -F'|' '{printf ("%s\011%s\n",$1,$2)}'
echo \\.

echo -- tipo_merma
echo tipo_merma > /dev/stderr
echo COPY tipo_merma \(id, nombre\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, nombre from tipo_merma" | awk -F'|' '{printf ("%s\011%s\n",$1,$2)}'
echo \\.

echo -- formas_pago
echo formas_pago > /dev/stderr
echo COPY formas_pago \(id, nombre, days\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, nombre, days from formas_pago" | awk -F'|' '{printf ("%s\011%s\011%s\n",$1,$2,$3)}'
echo \\.

echo -- impuestos -\> impuesto
echo impuestos > /dev/stderr
echo COPY impuesto \(id, descripcion, monto\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, descripcion, monto from impuestos" | awk -F '|' '{printf ("%s\011%s\011%s\n",$1,$2,$3)}'
echo \\.

echo -- negocio
echo negocio > /dev/stderr
echo COPY  negocio \(razon_social, rut, nombre, fono, fax, direccion, comuna, ciudad, giro, "at"\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select razon_social, rut, nombre, fono, fax, direccion, comuna, ciudad, giro, \"at\" from negocio" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n",$1,$2,$3,$4,$5,$6,$7,$8,$9,$10)}'
echo \\.


echo -- numeros_documentos
echo numeros_documentos > /dev/stderr
echo COPY numeros_documentos \(num_boleta, num_factura, num_guias\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select num_boleta, num_factura, num_guias from numeros_documentos" | awk -F'|' '{printf ("%s\011%s\011%s\n",$1,$2,$3)}'
echo \\.

echo -- users
echo users > /dev/stderr
echo COPY users \(id, rut, dv, usuario, passwd, nombre, apell_p, apell_m, fecha_ingreso, "level"\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, rut, usuario, passwd, nombre, apell_p, apell_m, fecha_ingreso, \"level\" from users" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n",$1,$2,$3,$4,$5,$6,$7,$8,$9)}'
echo \\.


echo -- asistencia
echo asistencia > /dev/stderr
echo COPY asistencia \(id, id_user, entrada, salida\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_user, entrada, salida from asistencia" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\n",$1,$2,$3,$4)}'
echo \\.

echo -- caja
echo caja > /dev/stderr
echo COPY caja \(id, id_vendedor, fecha_inicio, inicio, fecha_termino, termino, perdida\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, fecha_inicio, inicio, fecha_termino, termino, perdida from caja" | awk -F'|' '{printf ("%s\0111\011%s\011%s\011%s\011%s\011%s\n",$1,$2,$3,$4,$5,$6)}'
echo \\.

echo -- proveedores -\> proveedor
echo proveedores > /dev/stderr
echo COPY proveedor \(rut, dv, nombre, direccion, ciudad, comuna, telefono, email, web, contacto, giro\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select rut, nombre, direccion, ciudad, comuna, telefono, email, web, contacto, giro from proveedores" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n", substr($1,0,length($1)-2), substr($1,length($1)), $2, $3, $4, $5, $6, $7, $8, $9, $10)}'
echo \\.

echo -- clientes -\> cliente
echo clientes > /dev/stderr
echo COPY cliente \(rut, dv, nombre, apell_p, apell_m, giro, direccion, telefono, abonado, credito, credito_enable\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select rut, verificador, nombre, apellido_paterno, apellido_materno, giro, direccion, telefono, abonado, credito, credito_enable from clientes" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)}'
echo \\.

echo -- abonos -\> abono
echo abonos > /dev/stderr
echo COPY  abono \(id, rut_cliente, monto_abonado, fecha_abono\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, rut_cliente, monto_abonado, fecha_abono from abonos" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\n", $1, $2, $3, $4)}'
echo \\.

echo --  productos -\> producto
echo productos > /dev/stderr
echo COPY producto \(barcode, codigo_corto, marca, descripcion, contenido, unidad, stock, precio, costo_promedio, vendidos, impuestos, otros, familia, perecibles, stock_min, margen_promedio, fraccion, canje, stock_pro, tasa_canje, precio_mayor, cantidad_mayor, mayorista\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select barcode, codigo, marca, descripcion, contenido, unidad, stock, precio, fifo, vendidos, impuestos, otros, familia, perecibles, stock_min, margen_promedio, fraccion, canje, stock_pro, tasa_canje, precio_mayor, cantidad_mayor, mayorista from productos" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19, $20, $21, $22, $23)}'
echo \\.

echo -- ventas -\> venta
echo ventas > /dev/stderr
echo COPY venta \(id, monto, fecha, maquina, vendedor, tipo_documento, tipo_venta, descuento, id_documento, canceled\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, monto, fecha, maquina, vendedor, tipo_documento, tipo_venta, descuento, id_documento, canceled from ventas" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10)}'
echo \\.

echo -- deudas -\> deuda
echo deudas > /dev/stderr
echo COPY deuda \(id, id_venta, rut_cliente, vendedor, pagada, fecha_pagada\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_venta, rut_cliente, vendedor, pagada, fecha_pagada from deudas" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\011%s\n", $1, $2, $3, $4, $5, $6)}'
echo \\.

echo -- canje
echo canje > /dev/stderr
echo COPY  canje \(id, fecha, barcode, cantidad\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, fecha, barcode, cantidad from canje" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\n", $1, $2, $3, $4)}'
echo \\.

echo -- cheques -\> cheques
echo cheques > /dev/stderr
echo COPY cheques \(id, id_venta, serie, numero, banco, plaza, monto, fecha\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_venta, serie, numero, banco, plaza, monto, fecha from cheques" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n", $1, $2, $3, $4, $5, $6, $7, $8)}'
echo \\.

echo -- compras -\> compra
echo compras > /dev/stderr
echo COPY compra \(id, fecha, rut_proveedor, pedido, forma_pago, ingresada, anulada\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, fecha, rut_proveedor, pedido, forma_pago, ingresada, anulada from compras" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\011%s\011%s\n", $1, $2, substr($1,0,length($3)-2), $4, $5, $6, $7)}'
echo \\.

echo -- devoluciones -\> devolucion
echo devoluciones > /dev/stderr
echo COPY devolucion \(id, barcode_product, cantidad, cantidad_recibida, devuelto\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, barcode_product, cantidad, cantidad_recivida, devuelto from devoluciones" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\n", $1, $2, $3, $4, $5)}'
echo \\.

echo -- documentos_detalle
echo documentos_detalle > /dev/stderr
echo COPY documentos_detalle \(id, numero, id_compra, barcode, cantidad, precio, fecha, factura, elaboracion, vencimiento, iva, otros\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, numero, id_compra, barcode, cantidad, precio, fecha, factura, elaboracion, vencimiento, iva, otros from documentos_detalle" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12)}'
echo \\.

echo -- documentos_emitidos
echo documentos_emitidos > /dev/stderr
echo COPY documentos_emitidos \(id, tipo_documento, forma_pago, num_documento, fecha_emision\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, tipo_documento, forma_pago, num_documento, fecha_emision from documentos_emitidos" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\n", $1, $2, $3, $4, $5)}'
echo \\.

echo -- documentos_emitidos_detalle
echo documentos_emitidos_detalle > /dev/stderr
echo COPY documentos_emitidos_detalle \(id, id_documento, barcode_product, precio, cantidad\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_documento, barcode_product, precio, cantidad from documentos_emitidos_detalle" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\n", $1, $2, $3, $4, $5)}'
echo \\.

echo -- egresos -\> egreso
echo egresos > /dev/stderr
echo COPY egreso \(id, monto, tipo, fecha, usuario\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, monto, tipo, fecha, usuario from egresos" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\n",$1, $2, $3, $4, $5)}'
echo \\.

echo -- facturas_compras
echo facturas_compras > /dev/stderr
echo COPY factura_compra \(id, id_compra, rut_proveedor, num_factura, fecha, valor_neto, valor_iva, descuento, pagada, monto, fecha_pago, forma_pago\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_compra, rut_proveedor, num_factura, fecha, valor_neto, valor_iva, descuento, pagada, monto, fecha_pago, forma_pago from facturas_compras" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n", $1, $2, substr($1,0,length($3)-2), $4, $5, $6, $7, $8, $9, $10, $11, $12)}'
echo \\.

echo -- guias_compra
echo guias_compra > /dev/stderr
echo COPY guias_compra \(id, numero, id_compra, id_factura, rut_proveedor, fecha_emision\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, numero, id_compra, id_factura, rut_proveedor, fecha_emicion from guias_compra" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\011%s\n", $1, $2, $3, $4, substr($1,0,length($5)-2), $6)}'
echo \\.

echo -- familias
echo familias > /dev/stderr
echo COPY familias \(id, nombre\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, nombre from familias" | awk -F'|' '{printf ("%s\011%s\011\n", $1, $2)}'
echo \\.

echo -- ingresos -\> ingreso
echo ingresos > /dev/stderr
echo COPY ingreso \(id, monto, tipo, fecha, usuario\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, monto, tipo, fecha, usuario from ingresos" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\n", $1, $2, $3, $4, $5)}'
echo \\.

echo -- merma
echo merma > /dev/stderr
echo COPY merma \(id, barcode, unidades, motivo\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, barcode, unidades, motivo from merma" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\n", $1, $2, $3, $4)}'
echo \\.

echo -- pagos
echo pagos > /dev/stderr
echo COPY pagos \(id_fact, fecha, caja, descrip\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id_fact, 
fecha, caja, descip from pagos" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\n", $1, $2, $3, $4)}'
echo \\.

echo -- productos_recividos -\> producto_recibido
echo productos_recibidos > /dev/stderr
echo COPY producto_recibido \(id_dcto, factura, id_compra, barcode, cant_ingresada\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id_dcto, factura, id_compra, barcode, cant_ingresada from productos_recividos" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\n", $1, $2, $3, $4, $5)}'
echo \\.

echo -- products_buy_history -\> compra_detalle
echo productos_buy_history > /dev/stderr
echo COPY  compra_detalle \(id, id_compra, cantidad, precio, precio_venta, cantidad_ingresada, descuento, barcode_product, margen, iva, otros_impuestos, anulado, canjeado\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_compra, cantidad, precio, precio_venta, cantidad_ingresada, descuento, barcode_product, margen, iva, otros, anulado, canjeado from products_buy_history" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13)}'
echo \\.

echo -- products_sell_history -\> venta_detalle
echo products_sell_history > /dev/stderr
echo COPY venta_detalle \(id, id_venta, barcode, cantidad, precio, fifo, iva, otros\) FROM stdin NULL as \'NULL\'\;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_venta, barcode, cantidad, precio, fifo, iva, otros from products_sell_history" | awk -F'|' '{printf ("%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n", $1, $2, $3, $4, $5, $6, $7, $8)}'
echo \\.


