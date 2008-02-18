#!/bin/bash
# Copyright (C) 2008 Rizoma Tecnologia Limitada <info@rizoma.cl>

# This file is part of rizoma.

# Rizoma is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


# Este script usa un método sumamente lento para hacer la migracion
# Primero hace un select sobre la tabla original para extraer los datos
# A continuacion se formaten para que cumplan con la nueva estructura de la
# de la base de datos, para generar via stdout una serie de comandos
# COPY tabla (args...
# fila1
# \.
# y nuevamente se repite
# se optó por este acercamiento, ya que la base de datos original no tiene
# control de integridad de datos, por lo que al hacer un solo COPY con muchas
# filas basta que una sola fila falle para que se aborte el COPY en su totalidad
# es por ello que se insert fila a fila
# Ejemplo de uso:
# $ ./migracion.sh nombre_bd nombre_usuario servidor puerto > dump.data
# $ psql -h servidor -p puerto nombre_bd_nueva nombre_usuario_nuevo < dump.data
#
# el script en su segunda etapa puede tomar más de media hora para una base de 
# datos de 50MB, y el archivo de salida del script puede utilizar con facilidad
# el triple de espacio de la base de datos original
#
# TODO: agregar las lineas que permiten actualizar las secuencias
#       con algo como select setval('impuesto_id_seq', max(id)) FROM impuesto;
BD_NAME=$1
BD_USER=$2
BD_HOST=$3
BD_PORT=$4


psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --command "
update devoluciones set barcode_product = CAST( barcode_product as int8 ) where barcode_product in ( select barcode from productos where barcode in (select CAST(barcode as int8 ) from productos where barcode like '0%' ) );
update productos_recividos set barcode = CAST( barcode as int8 ) where barcode in ( select barcode from productos where barcode in (select CAST(barcode as int8 ) from productos where barcode like '0%' ) );
update products_buy_history set barcode_product = CAST( barcode_product as int8 ) where barcode_product in ( select barcode from productos where barcode in (select CAST(barcode as int8 ) from productos where barcode like '0%' ));
update products_sell_history set barcode = CAST( barcode as int8 ) where barcode in ( select barcode from productos where barcode in (select CAST(barcode as int8 ) from productos where barcode like '0%' ));
delete from productos where barcode in ( select barcode from productos where barcode in (select CAST(barcode as int8 ) from productos where barcode like '0%' ) );
" > /dev/stderr

echo -- tarjetas
echo tarjetas > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_venta, insti, numero, fecha_venc from tarjetas" | awk -F'|' '{printf ("COPY tarjetas (id, id_venta, insti, numero, fecha_venc) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\n\\.\n",$1,$2,$3,$4,$5)}'

echo -- tipo_egreso
echo tipo_egreso > /dev/stderr
#echo COPY tipo_egreso (id,descrip) FROM stdin NULL as \047NULL\047;
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, descrip from tipo_egreso" | awk -F'|' '{printf ("COPY tipo_egreso (id,descrip) FROM stdin NULL as \047NULL\047;\n%s\011%s\n\\.\n",$1,$2)}'

echo -- tipo_ingreso
echo tipo_ingreso > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, descrip from tipo_ingreso" | awk -F'|' '{printf ("COPY tipo_ingreso (id, descrip) FROM stdin NULL as \047NULL\047;\n%s\011%s\n\\.\n",$1,$2)}'


echo -- tipo_merma
echo tipo_merma > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, nombre from tipo_merma" | awk -F'|' '{printf ("COPY tipo_merma (id, nombre) FROM stdin NULL as \047NULL\047;\n%s\011%s\n\\.\n",$1,$2)}'


echo -- formas_pago
echo formas_pago > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, nombre, days from formas_pago" | awk -F'|' '{printf ("COPY formas_pago (id, nombre, days) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\n\\.\n",$1,$2,$3)}'


echo -- impuestos -\> impuesto
echo impuestos > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, descripcion, monto from impuestos" | awk -F '|' '{printf ("COPY impuesto (id, descripcion, monto) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\n\\.\n",$1,$2,$3)}'


echo -- negocio
echo negocio > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select razon_social, rut, nombre, fono, fax, direccion, comuna, ciudad, giro, \"at\" from negocio" | awk -F'|' '{printf ("COPY  negocio (razon_social, rut, nombre, fono, fax, direccion, comuna, ciudad, giro, \042at\042) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n\\.\n",$1,$2,$3,$4,$5,$6,$7,$8,$9,$10)}'



echo -- numeros_documentos
echo numeros_documentos > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select num_boleta, num_factura, num_guias from numeros_documentos" | awk -F'|' '{printf ("COPY numeros_documentos (num_boleta, num_factura, num_guias) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\n\\.\n",$1,$2,$3)}'


echo -- users
echo users > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, rut, usuario, passwd, nombre, apell_p, apell_m, fecha_ingreso, \"level\" from users" | awk -F'|' '{printf ("COPY users (id, rut, dv, usuario, passwd, nombre, apell_p, apell_m, fecha_ingreso, \042level\042) FROM stdin NULL as \047NULL\047;\n%s\011%s\0110\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n\\.\n",$1,$2,$3,$4,$5,$6,$7,$8,$9)}'



echo -- asistencia
echo asistencia > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_user, entrada, salida from asistencia" | awk -F'|' '{printf ("COPY asistencia (id, id_user, entrada, salida) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\n\\.\n",$1,$2,$3,$4)}'


echo -- caja
echo caja > /dev/stderr
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, fecha_inicio, inicio, fecha_termino, termino, perdida from caja" | awk -F'|' '{printf ("COPY caja (id, id_vendedor, fecha_inicio, inicio, fecha_termino, termino, perdida) FROM stdin NULL as \047NULL\047;\n%s\0111\011%s\011%s\011%s\011%s\011%s\n\\.\n",$1,$2,$3,$4,$5,$6)}'


echo -- proveedores -\> proveedor
echo proveedores > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select rut, nombre, direccion, ciudad, comuna, telefono, email, web, contacto, giro from proveedores" | awk -F'|' '{printf ("COPY proveedor (rut, dv, nombre, direccion, ciudad, comuna, telefono, email, web, contacto, giro) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n\\.\n", substr($1,0,length($1)-2), substr($1,length($1)), $2, $3, $4, $5, $6, $7, $8, $9, $10)}'


echo -- clientes -\> cliente
echo clientes > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select rut, verificador, nombre, apellido_paterno, apellido_materno, giro, direccion, telefono, abonado, credito, credito_enable from clientes" | awk -F'|' '{printf ("COPY cliente (rut, dv, nombre, apell_p, apell_m, giro, direccion, telefono, abonado, credito, credito_enable) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)}'


echo -- abonos -\> abono
echo abonos > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, rut_cliente, monto_abonado, fecha_abono from abonos" | awk -F'|' '{printf ("COPY  abono (id, rut_cliente, monto_abonado, fecha_abono) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4)}'


echo --  productos -\> producto
echo productos > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select barcode, codigo, marca, descripcion, contenido, unidad, stock, precio, fifo, vendidos, impuestos, otros, familia, perecibles, stock_min, margen_promedio, fraccion, canje, stock_pro, tasa_canje, precio_mayor, cantidad_mayor, mayorista from productos" | awk -F'|' '{printf ("COPY producto (barcode, codigo_corto, marca, descripcion, contenido, unidad, stock, precio, costo_promedio, vendidos, impuestos, otros, familia, perecibles, stock_min, margen_promedio, fraccion, canje, stock_pro, tasa_canje, precio_mayor, cantidad_mayor, mayorista) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19, $20, $21, $22, $23)}'


echo -- ventas -\> venta
echo ventas > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, monto, fecha, maquina, vendedor, tipo_documento, tipo_venta, descuento, id_documento, canceled from ventas" | awk -F'|' '{printf ("COPY venta (id, monto, fecha, maquina, vendedor, tipo_documento, tipo_venta, descuento, id_documento, canceled) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10)}'


echo -- deudas -\> deuda
echo deudas > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_venta, rut_cliente, vendedor, pagada, fecha_pagada from deudas" | awk -F'|' '{printf ("COPY deuda (id, id_venta, rut_cliente, vendedor, pagada, fecha_pagada) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4, $5, $6)}'


echo -- canje
echo canje > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, fecha, barcode, cantidad from canje" | awk -F'|' '{printf ("COPY  canje (id, fecha, barcode, cantidad) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4)}'


echo -- cheques -\> cheques
echo cheques > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_venta, serie, numero, banco, plaza, monto, fecha from cheques" | awk -F'|' '{printf ("COPY cheques (id, id_venta, serie, numero, banco, plaza, monto, fecha) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4, $5, $6, $7, $8)}'


echo -- compras -\> compra
echo compras > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, fecha, rut_proveedor, pedido, forma_pago, ingresada, anulada from compras" | awk -F'|' '{printf ("COPY compra (id, fecha, rut_proveedor, pedido, forma_pago, ingresada, anulada) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, substr($3,0,length($3)-2), $4, $5, $6, $7)}'


echo -- devoluciones -\> devolucion
echo devoluciones > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, barcode_product, cantidad, cantidad_recivida, devuelto from devoluciones" | awk -F'|' '{printf ("COPY devolucion (id, barcode_product, cantidad, cantidad_recibida, devuelto) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4, $5)}'


echo -- documentos_detalle
echo documentos_detalle > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, numero, id_compra, barcode, cantidad, precio, fecha, factura, elaboracion, vencimiento, iva, otros from documentos_detalle" | awk -F'|' '{printf ("COPY documentos_detalle (id, numero, id_compra, barcode, cantidad, precio, fecha, factura, elaboracion, vencimiento, iva, otros) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12)}'


echo -- documentos_emitidos
echo documentos_emitidos > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, tipo_documento, forma_pago, num_documento, fecha_emision from documentos_emitidos" | awk -F'|' '{printf ("COPY documentos_emitidos (id, tipo_documento, forma_pago, num_documento, fecha_emision) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4, $5)}'


echo -- documentos_emitidos_detalle
echo documentos_emitidos_detalle > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_documento, barcode_product, precio, cantidad from documentos_emitidos_detalle" | awk -F'|' '{printf ("COPY documentos_emitidos_detalle (id, id_documento, barcode_product, precio, cantidad) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4, $5)}'


echo -- egresos -\> egreso
echo egresos > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, monto, tipo, fecha, usuario from egresos" | awk -F'|' '{printf ("COPY egreso (id, monto, tipo, fecha, usuario) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\n\\.\n",$1, $2, $3, $4, $5)}'


echo -- facturas_compras
echo facturas_compras > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_compra, rut_proveedor, num_factura, fecha, valor_neto, valor_iva, descuento, pagada, monto, fecha_pago, forma_pago from facturas_compras" | awk -F'|' '{printf ("COPY factura_compra (id, id_compra, rut_proveedor, num_factura, fecha, valor_neto, valor_iva, descuento, pagada, monto, fecha_pago, forma_pago) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, substr($1,0,length($3)-2), $4, $5, $6, $7, $8, $9, $10, $11, $12)}'


echo -- guias_compra
echo guias_compra > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, numero, id_compra, id_factura, rut_proveedor, fecha_emicion from guias_compra" | awk -F'|' '{printf ("COPY guias_compra (id, numero, id_compra, id_factura, rut_proveedor, fecha_emision) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4, substr($1,0,length($5)-2), $6)}'


echo -- familias
echo familias > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, nombre from familias" | awk -F'|' '{printf ("COPY familias (id, nombre) FROM stdin NULL as \047NULL\047;\n%s\011%s\011\n\\.\n", $1, $2)}'


echo -- ingresos -\> ingreso
echo ingresos > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, monto, tipo, fecha, usuario from ingresos" | awk -F'|' '{printf ("COPY ingreso (id, monto, tipo, fecha, usuario) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4, $5)}'


echo -- merma
echo merma > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, barcode, unidades, motivo from merma" | awk -F'|' '{printf ("COPY merma (id, barcode, unidades, motivo) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4)}'


echo -- pagos
echo pagos > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id_fact, 
fecha, caja, descip from pagos" | awk -F'|' '{printf ("COPY pagos (id_fact, fecha, caja, descrip) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4)}'


echo -- productos_recividos -\> producto_recibido
echo productos_recibidos > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id_dcto, factura, id_compra, barcode, cant_ingresada from productos_recividos" | awk -F'|' '{printf ("COPY producto_recibido (id_dcto, factura, id_compra, barcode, cant_ingresada) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4, $5)}'


echo -- products_buy_history -\> compra_detalle
echo productos_buy_history > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_compra, cantidad, precio, precio_venta, cantidad_ingresada, descuento, barcode_product, margen, iva, otros, anulado, canjeado from products_buy_history" | awk -F'|' '{printf ("COPY  compra_detalle (id, id_compra, cantidad, precio, precio_venta, cantidad_ingresada, descuento, barcode_product, margen, iva, otros_impuestos, anulado, canjeado) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13)}'


echo -- products_sell_history -\> venta_detalle
echo products_sell_history > /dev/stderr
echo 
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER -P null=NULL --tuples-only --no-align --command "select id, id_venta, barcode, cantidad, precio, fifo, iva, otros from products_sell_history" | awk -F'|' '{printf ("COPY venta_detalle (id, id_venta, barcode, cantidad, precio, fifo, iva, otros) FROM stdin NULL as \047NULL\047;\n%s\011%s\011%s\011%s\011%s\011%s\011%s\011%s\n\\.\n", $1, $2, $3, $4, $5, $6, $7, $8)}'

# actualiza las secuencias
echo "select setval('compra_id_seq', max(id)) FROM compra;"
echo "select setval('abono_id_seq', max(id)) FROM abono;"
echo "select setval('asistencia_id_seq', max(id)) FROM asistencia;"
echo "select setval('caja_id_seq', max(id)) FROM caja;"
echo "select setval('canje_id_seq', max(id)) FROM canje;"
echo "select setval('cheques_id_seq', max(id)) FROM cheques;"
echo "select setval('compra_id_seq', max(id)) FROM compra;"
echo "select setval('deuda_id_seq', max(id)) FROM deuda;"
echo "select setval('devolucion_id_seq', max(id)) FROM devolucion;"
echo "select setval('documentos_detalle_id_seq', max(id)) FROM documentos_detalle;"
echo " select setval('documentos_emitidos_id_seq', max(id)) FROM documentos_emitidos;"
echo "select setval('egreso_id_seq', max(id)) FROM egreso;"
echo "select setval('factura_compra_id_seq', max(id)) FROM factura_compra;"
echo "select setval('familias_id_seq', max(id)) FROM familias;"
echo "select setval('formas_pago_id_seq', max(id)) FROM formas_pago;"
echo "select setval('guias_compra_id_seq', max(id)) FROM guias_compra;"
echo "select setval('impuesto_id_seq', max(id)) FROM impuesto;"
echo "select setval('ingreso_id_seq', max(id)) FROM ingreso;"
echo "select setval('merma_id_seq', max(id)) FROM merma;"
echo "select setval('tarjetas_id_seq', max(id)) FROM tarjetas;"
echo "select setval('tipo_egreso_id_seq', max(id)) FROM tipo_egreso;"
echo "select setval('tipo_ingreso_id_seq', max(id)) FROM tipo_ingreso;"
echo "select setval('tipo_merma_id_seq', max(id)) FROM tipo_merma;"
echo "select setval('users_id_seq', max(id)) FROM users;"
