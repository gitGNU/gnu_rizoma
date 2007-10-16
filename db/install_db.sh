#!/bin/sh

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

# Creamos la db
#createdb -h $DBHOST -p $DBPORT -U $USERNAME $DBNAME
echo "Creando base de datos..."
createdb -U $USERNAME $DBNAME &> /dev/null

if [ $? == 1 ]; then
    echo "No se pudo crear la base de datos."
    exit
else
    echo "Base de datos creada."
fi

# Ingreamos la estructura de datos
echo "Volcando estructura de datos..."

psql -U $USERNAME -d $DBNAME < rizoma.structure &> /dev/null

if [ $? == 1 ]; then
    echo "Error al volcar estructura."
    exit
else
    echo "Volcado de estructura finalizado."
fi

# Ingresamos los datos iniciales
echo "Volando datos iniciales..."

psql -U $USERNAME -d $DBNAME < rizoma.initvalues &> /dev/null

if [ $? == 1 ]; then
    echo "Error al volcar los datos iniciales"
    exit
else
    echo "Volcado de datos iniciales finalizado."
fi


