#!/bin/bash

BD_NAME=rizomaviejo
BD_USER=rizomaviejo
BD_HOST=localhost
BD_PORT=5432


echo -- tarjetas
echo tarjetas > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, id_venta, insti, numero, fecha_venc from tarjetas" | awk -F'|' '{printf ("insert into tarjetas (id, id_venta, insti, numero, fecha_venc) values (%s, %s, \047%s\047, \047%s\047, \047%s\047);\n", $1, $2, $3, $4, $5)}'

echo -- tipo_egreso
echo tipo_egreso > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, descrip from tipo_egreso" | awk -F'|' '{printf ("insert into tipo_egreso (id, descrip) values (%s, \047%s\047);\n", $1, $2)}'

echo -- tipo_ingreso
echo tipo_ingreso > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, descrip from tipo_ingreso" | awk -F'|' '{printf ("insert into tipo_ingreso (id, descrip) values (%s, \047%s\047);\n", $1, $2)}'

echo -- tipo_merma
echo tipo_merma > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, nombre from tipo_merma" | awk -F'|' '{printf ("insert into tipo_merma (id, nombre) values (%s, \047%s\047);\n", $1, $2)}'

echo -- formas_pago
echo formas_pago > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, nombre, days from formas_pago" | awk -F'|' '{printf ("insert into formas_pago (id, nombre, days) values (%s, \047%s\047, %s);\n", $1, $2, $3)}'

echo -- impuestos -\> impuesto
echo impuestos > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, descripcion, monto from impuestos" | awk -F '|' '{printf ("insert into impuesto (id, descripcion, monto) values (%s, \047%s\047, %s);\n", $1, $2, $3)}'

echo -- negocio
echo negocio > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select razon_social, rut, nombre, fono, fax, direccion, comuna, ciudad, giro, \"at\" from negocio" | awk -F'|' '{printf ("insert into negocio (razon_social, rut, nombre, fono, fax, direccion, comuna, ciudad, giro, \042at\042) values (\047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047)\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10)}'

echo -- numeros_documentos
echo numeros_documentos > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select num_boleta, num_factura, num_guias from numeros_documentos" | awk -F'|' '{printf ("insert into numeros_documentos (num_boleta, num_factura, num_guias) values (%s, %s, %s);\n", $1, $2, $3)}'

echo -- users
echo users > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, rut, usuario, passwd, nombre, apell_p, apell_m, fecha_ingreso, \"level\" from users" | awk -F'|' '{printf ("insert into users (id, rut, dv, usuario, passwd, nombre, apell_p, apell_m, fecha_ingreso, \042level\042) values (%s, %s, \0470\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, %s);\n", $1, $2, $3, $4, $5, $6, $7, $8, $9)}'

echo -- asistencia
echo asistencia > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, id_user, entrada, salida from asistencia" | awk -F'|' '{printf ("insert into asistencia (id, id_user, entrada, salida) values (%s, %s, \047%s\047, \047%s\047);\n", $1, $2, $3, $4)}'

echo -- caja
echo caja > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, fecha_inicio, inicio, fecha_termino, termino, perdida from caja" | awk -F'|' '{printf ("insert into caja (id, id_vendedor, fecha_inicio, inicio, fecha_termino, termino, perdida) values (%s, 1, \047%s\047, %s, \047%s\047, %s, %s);\n", $1, $2, $3, $4, $5, $6)}'

echo -- proveedores -\> proveedor
echo proveedores > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select rut, nombre, direccion, ciudad, comuna, telefono, email, web, contacto, giro from proveedores" | awk -F'|' '{printf ("insert into proveedor (rut, dv, nombre, direccion, ciudad, comuna, telefono, email, web, contacto, giro) values (%s, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, %s, \047%s\047, \047%s\047, \047%s\047, \047%s\047);\n", substr($1,0,length($1)-2), substr($1,length($1)), $2, $3, $4, $5, $6, $7, $8, $9, $10)}'

echo -- clientes -\> cliente
echo clientes > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select rut, verificador, nombre, apellido_paterno, apellido_materno, giro, direccion, telefono, abonado, credito, credito_enable from clientes" | awk -F'|' '{printf ("insert into cliente (rut, dv, nombre, apell_p, apell_m, giro, direccion, telefono, abonado, credito, credito_enable) values (%s, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, %s, %s, \047%s\047);\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)}'

echo -- abonos -\> abono
echo abonos > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, rut_cliente, monto_abonado, fecha_abono from abonos" | awk -F'|' '{printf ("insert into abono (id, rut_cliente, monto_abonado, fecha_abono) values (%s, %s, %s, \047%s\047);\n", $1, $2, $3, $4)}'


echo --  productos -\> producto
echo productos > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select barcode, codigo, marca, descripcion, contenido, unidad, stock, precio, fifo, vendidos, impuestos, otros, familia, perecibles, stock_min, margen_promedio, fraccion, canje, stock_pro, tasa_canje, precio_mayor, cantidad_mayor, mayorista from productos" | awk -F'|' '{printf ("insert into producto (barcode, codigo_corto, marca, descripcion, contenido, unidad, stock, precio, costo_promedio, vendidos, impuestos, otros, familia, perecibles, stock_min, margen_promedio, fraccion, canje, stock_pro, tasa_canje, precio_mayor, cantidad_mayor, mayorista) values (%s, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, %s, %s, %s, %s, \047%s\047, %s, %s, \047%s\047, %s, %s, \047%s\047, \047%s\047, %s, %s, %s, %s, \047%s\047);\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19, $20, $21, $22, $23)}'



echo -- ventas -\> venta
echo ventas > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, monto, fecha, maquina, vendedor, tipo_documento, tipo_documento, tipo_venta, descuento, id_documento, canceled from ventas" | awk -F'|' '{printf ("insert into venta (id, monto, fecha, maquina, vendedor, tipo_documento, tipo_venta, descuento, id_documento, canceled) values (%s, %s, \047%s\047, %s, %s, %s, %s, %s, %s, \047%s\047);\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10)}'

echo -- deudas -\> deuda
echo deudas > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, id_venta, rut_cliente, vendedor, pagada, fecha_pagada from deudas" | awk -F'|' '{printf ("insert into deuda (id, id_venta, rut_cliente, vendedor, pagada, fecha_pagada) values (%s, %s, %s, %s, \047%s\047, \047%s\047);\n", $1, $2, $3, $4, $5, $6)}'

echo -- canje
echo canje > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, fecha, barcode, cantidad from canje" | awk -F'|' '{printf ("insert into canje (id, fecha, barcode, cantidad) values (%s, \047%s\047, %s, %s);\n", $1, $2, $3, $4)}'

echo -- cheques -\> cheques
echo cheques > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, id_venta, serie, numero, banco, plaza, monto, fecha from cheques" | awk -F'|' '{printf ("insert into cheques (id, id_venta, serie, numero, banco, plaza, monto, fecha) values (%s, %s, \047%s\047, %s, \047%s\047, \047%s\047, %s, \047%s\047);\n", $1, $2, $3, $4, $5, $6, $7, $8)}'

echo -- compras -\> compra
echo compras > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, fecha, rut_proveedor, pedido, forma_pago, ingresada, anulada from compras" | awk -F'|' '{printf ("insert into compra (id, fecha, rut_proveedor, pedido, forma_pago, ingresada, anulada) values (%s, \047%s\047, %s, \047%s\047, %s, \047%s\047, \047%s\047);\n", $1, $2, substr($1,0,length($3)-2), $4, $5, $6, $7)}'


echo -- devoluciones -\> devolucion
echo devoluciones > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, barcode_product, cantidad, cantidad_recivida, devuelto from devoluciones" | awk -F'|' '{printf ("insert into devolucion (id, barcode_product, cantidad, cantidad_recibida, devuelto) values (%s, %s, %s, %s, \047%s\047);\n", $1, $2, $3, $4,$5)}'

echo -- documentos_detalle
echo documentos_detalle > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, numero, id_compra, barcode, cantidad, precio, fecha, factura, elaboracion, vencimiento, iva, otros from documentos_detalle" | awk -F'|' '{printf ("insert into documentos_detalle (id, numero, id_compra, barcode, cantidad, precio, fecha, factura, elaboracion, vencimiento, iva, otros) values (%s, %s, %s, \047%s\047, %s, %s, \047%s\047, \047%s\047, \047%s\047, \047%s\047, %s, %s);\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12)}'

echo -- documentos_emitidos
echo documentos_emitidos > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, tipo_documento, forma_pago, num_documento, fecha_emision from documentos_emitidos" | awk -F'|' '{printf ("insert into documentos_emitidos (id, tipo_documento, forma_pago, num_documento, fecha_emision) values (%s, %s, %s, %s, \047%s\047);\n", $1, $2, $3, $4, $5)}'

echo -- documentos_emitidos_detalle
echo documentos_emitidos_detalle > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, id_documento, barcode_product, precio, cantidad from documentos_emitidos_detalle" | awk -F'|' '{printf ("insert into documentos_emitidos_detalle (id, id_documento, barcode_product, precio, cantidad) values (%s, %s, %s, %s, %s);\n", $1, $2, $3, $4, $5)}'

echo -- egresos -\> egreso
echo egresos > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, monto, tipo, fecha, usuario from egresos" | awk -F'|' '{printf ("insert into egreso (id, monto, tipo, fecha, usuario) values (%s, %s, %s, \047%s\047, %s);\n",$1, $2, $3, $4, $5)}'

echo -- facturas_compras
echo facturas_compras > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, id_compra, rut_proveedor, num_factura, fecha, valor_neto, valor_iva, descuento, pagada, monto, fecha_pago, forma_pago from facturas_compras" | awk -F'|' '{printf ("insert into factura_compra (id, id_compra, rut_proveedor, num_factura, fecha, valor_neto, valor_iva, descuento, pagada, monto, fecha_pago, forma_pago) values (%s, %s, %s, %s, \047%s\047, %s, $s, %s, \047%s\047, %s, \047%s\047, %s);\n", $1, $2, substr($1,0,length($3)-2), $4, $5, $6, $7, $8, $9, $10, $11, $12)}'

echo -- guias_compra
echo guias_compra > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, numero, id_compra, id_factura, rut_proveedor, fecha_emicion from guias_compra" | awk -F'|' '{printf ("insert into guias_compra (id, numero, id_compra, id_factura, rut_proveedor, fecha_emision) values (%s, %s, %s, %s, %s, \047%s\047);\n", $1, $2, $3, $4, substr($1,0,length($5)-2), $6)}'

echo -- familias
echo familias > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, nombre from familias" | awk -F'|' '{printf ("insert into familias (id, nombre) values (%s, \047%s\047);\n", $1, $2)}'


echo -- ingresos -\> ingreso
echo ingresos > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, monto, tipo, fecha, usuario from ingresos" | awk -F'|' '{printf ("insert into ingreso (id, monto, tipo, fecha, usuario) values (%s, %s, %s, \047%s\047, %s);\n", $1, $2, $3, $4, $5)}'

echo -- merma
echo merma > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, barcode, unidades, motivo from merma" | awk -F'|' '{printf ("insert into ingreso (id, barcode, unidades, motivo) values (%s, %s, %s, %s);\n", $1, $2, $3, $4)}'


echo -- pagos
echo pagos > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id_fact, 
fecha, caja, descip from pagos" | awk -F'|' '{printf ("insert into pagos (id_fact, fecha, caja, descrip) values (%s, \047%s\047, \047%s\047, \047%s\047);\n", $1, $2, $3, $4)}'

echo -- productos_recividos -\> producto_recibido
echo productos_recibidos > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id_dcto, factura, id_compra, barcode, cant_ingresada from productos_recividos" | awk -F'|' '{printf ("insert into producto_recibido (id_dcto, factura, id_compra, barcode, cant_ingresada) values (%s, \047%s\047, %s, %s, %s);\n", $1, $2, $3, $4, $5)}'

echo -- products_buy_history -\> compra_detalle
echo productos_buy_history > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, id_compra, cantidad, precio, precio_venta, cantidad_ingresada, descuento, barcode_product, margen, iva, otros, anulado, canjeado from products_buy_history" | awk -F'|' '{printf ("insert into compra_detalle (id, id_compra, cantidad, precio, precio_venta, cantidad_ingresada, descuento, barcode_product, margen, iva, otros_impuestos, anulado, canjeado) values (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, \047%s\047, \047%s\047);\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13)}'

echo -- products_sell_history -\> venta_detalle
echo products_sell_history > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, id_venta, barcode, cantidad, precio, fifo, iva, otros from products_sell_history" | awk -F'|' '{printf ("insert into venta_detalle (id, id_venta, barcode, cantidad, precio, fifo, iva, otros) values (%s, %s, %s, %s, %s, %s, %s, %s);\n", $1, $2, $3, $4, $5, $6, $7, $8)}'


