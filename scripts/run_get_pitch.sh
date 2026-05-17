#!/bin/bash

# Establecemos que el código de retorno de un pipeline sea el del último programa con código de retorno
# distinto de cero, o cero si todos devuelven cero.
set -o pipefail

# Put here the program (maybe with path)
GETF0="get_pitch"

# Define aquí tus parámetros optimizados para no tener que escribirlos cada vez
PARAMS="--window rect  --clip 0.0005 --median 3 --min-f0 50 --max-f0 500"

for fwav in pitch_db/train/*.wav; do
    ff0=${fwav/.wav/.f0}
    echo "$GETF0 $fwav $ff0 con parámetros: $PARAMS ----"
    
    # Añadimos la variable $PARAMS antes de los nombres de archivo
    $GETF0 $PARAMS "$fwav" "$ff0" > /dev/null || { echo -e "\nError in $GETF0 $fwav $ff0" && exit 1; }
done

# Al acabar el bucle, evaluamos todos los resultados generados
pitch_evaluate pitch_db/train/*.f0ref

exit 0
