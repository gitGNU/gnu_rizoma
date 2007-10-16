#!/bin/sh

if [ -z "$1" ]; then
    echo Uso: $0 [Archivo SQL]
    exit
fi

# Obtenemos el nombre de usuario para la db
echo "Ingrese el nombre de usuario: "
read USERNAME

# Obtenemos el host a utilizar
#echo "Ingrese el host: "
#read DBHOST

# Obtenemos el puerto
#echo "Ingrese el puerto: "
#read DBPORT

# Obtenemos el nombre de la db
echo "Ingrese el nombre de la DB: "
read DBNAME

# Actualizamos la base de datos
echo "Actualizando base de datos..."
psql -U $USERNAME -d $DBNAME < $1 &> /dev/null

if [ $? == 1 ]; then
    echo "Error al actualizar base de datos."
    exit
else
    echo "Actualizacion completada con exito."
fi