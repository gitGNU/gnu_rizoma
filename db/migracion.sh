#!/bin/bash

$BD_NAME=rizomaviejo
$BD_USER=rizomaviejo
$BD_HOST=localhost
$BD_PORT=5432
# productos -> producto
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select barcode, codigo, marca, descripcion, contenido, unidad, stock, precio, fifo, vendidos, impuestos, otros, familia, perecibles, stock_min, margen_promedio, fraccion, canje, stock_pro, tasa_canje, precio_mayor, cantidad_mayor, mayorista from productos" | awk -F'|' '{printf ("insert into producto (barcode, codigo_corto, marca, descripcion, contenido, unidad, stock, precio, costo_promedio, vendidos, impuestos, otros, familia, perecibles, stock_min, margen_promedio, fraccion, canje, stock_pro, tasa_canje, precio_mayor, cantidad_mayor, mayorista) values (%s, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, %s, %s, %s, %s, \047%s\047, %s, %s, \047%s\047, %s, %s, \047%s\047, \047%s\047, %s, %s, %s, %s, \047%s\047);", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19, $20, $21, $22, $23)}'

# tipo_egreso
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, descrip from tipo_egreso" | awk -F'|' '{printf ("insert into tipo_egreso (id, descrip) values ($s, \047$s\047);", $1, $2)}'

#tipo_ingreso
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, descrip from tipo_ingreso" | awk -F'|' '{printf ("insert into tipo_ingreso (id, descrip) values (%s, \047%s\047);", $1, $2)}'

#tipo_merma
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, nombre from tipo_merma" | awk -F'|' '{printf ("insert into tipo_merma (id, nombre) values (%s, \047%s\047);", $1, $2)}'

#users
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, rut, usuario, passwd, nombre, apell_p, apell_m, fecha_ingreso, \"level\" from users" | awk -F'|' '{printf ("insert into users (id, rut, dv, usuario, passwd, nombre, apell_p, apell_m, fecha_ingreso, \042level\042) values (%s, %s, \0470\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, %s);", $1, $2, $3, $4, $5, $6, $7, $8, $9)}'

#clientes -> cliente
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select rut, verificador, nombre, apellido_paterno, apellido_materno, giro, direccion, telefono, abonado, credito, credito_enable from clientes" | awk -F'|' '{printf ("insert into cliente (rut, dv, nombre, apell_p, apell_m, giro, direccion, telefono, abonado, credito, credito_enable) values (%s, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, \047%s\047, %s, %s, \047%s\047);", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)}'

#abonos -> abono
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, rut_cliente, monto_abonado, fecha_abono from abonos" | awk -F'|' '{printf ("insert into abono (id, rut_cliente, monto_abonado, fecha_abono) values (%s, %s, %s, \047%s\047);", $1, $2, $3, $4)}'

#ventas -> venta
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, monto, fecha, maquina, vendedor, tipo_documento, tipo_documento, tipo_venta, descuento, id_documento, canceled from ventas" | awk -F'|' '{printf ("insert into venta (id, monto, fecha, maquina, vendedor, tipo_documento, tipo_venta, descuento, id_documento, canceled) values (%s, %s, \047%s\047, %s, %s, %s, %s, %s, %s, \047%s\047);", $1, $2, $3, $4, $5, $6, $7, $8, $9, $10)}'

#asistencia
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, id_user, entrada, salida from asistencia" | awk -F'|' '{printf ("insert into asistencia (id, id_user, entrada, salida) values (%s, %s, \047%s\047, \047%s\047);", $1, $2, $3, $4)}'

#caja
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, fecha_inicio, inicio, fecha_termino, termino, perdida from caja" | awk -F'|' '{printf ("insert into caja (id, id_vendedor, fecha_inicio, inicio, fecha_termino, termino, perdida) values (%s, 1, \047%s\047, %s, \047%s\047, %s, %s);", $1, $2, $3, $4, $5, $6)}'

#canje
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, fecha, barcode, cantidad from caje" | awk -F'|' '{printf ("insert into canje (id, fecha, barcode, cantidad) values (%s, \047%s\047, %s, %s);", $1, $2, $3, $4)}'

#cheques -> cheques
psql -h $BD_HOST -p $BD_PORT $BD_NAME $BD_USER --tuples-only --no-align --command "select id, id_venta, serie, numero, banco, plaza, monto, fecha from cheques" | awk -F'|' '{printf ("insert into cheques (id, id_venta, serie, numero, banco, plaza, monto, fecha) values (%s, %s, \047%s\047, %s, \047%s\047, \047%s\047, %s, \047%s\047);", $1, $2, $3, $4, $5, $6, $7, $8)}'


