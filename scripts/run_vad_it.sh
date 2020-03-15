#!/bin/bash

# Be sure that this file has execution permissions:
# Use the nautilus explorer or chmod +x run_vad.sh

ninja -C bin
DB=${PWD}/db.v4
CMD=bin/vad  #write here the name and path of your program

rm "sortida.txt"


 typeset -i exitStatus=1
 typeset -i out=1
    function ejecutar
                {
                    
                    for filewav in $DB/*/*wav; do
                        #   echo
                        #echo "**************** $filewav ****************"
                        if [[ ! -f $filewav ]]; then 
                            echo "Wav file not found: $filewav" >&2
                            exit 1
                        fi
                        filevad=${filewav/.wav/.vad}

                        # a es alpha1, b es alpha2, c es umbral zcr, d es frames min silenci, e es frames min veu
                        # Aqui es donde puede dar error de lectura de fichero wav
                        # En caso de que haya error de lectura de fichero wav, la funcion ejecutar retorna 1
                        $CMD -i $filewav -o $filevad -a $a -b $b -c $c -d $d -e $e -f $f -g $g || exit 1
                        
                        # Alternatively, uncomment to create output wave files
                        #    filewavOut=${filewav/.wav/.vad.wav}
                        #    $CMD $filewav $filevad $filewavOut || exit 1
                    done

                    # Si ha llegado hasta aqui es que todo se ha ejecutado correctamente
                    # Se procede a la escritura en fichero y a la evaluacion
                    echo -n "run: alfa1=$a  alpha2=$b  zcr=$c  max_ms=$d  max_mv=$e  min_s=$f  min_v=$g"
                    echo -n "run: alfa1=$a  alpha2=$b  zcr=$c  max_ms=$d  max_mv=$e  min_s=$f  min_v=$g" >> sortida.txt
                    scripts/vad_evaluation_mod.pl $DB/*/*lab
                    return $?
                }

for ((a = 670; a <= 720; a+=1)); do #alpha1  
    for ((b = 175; b <= 225; b+=1)); do #alpha2 
        for ((c = 1630; c <= 1630; c+=1)); do #zcr
            for d in 15; do #max_ms 
                for e in 3; do #max_mv
                    for f in 1; do #min_s 
                        for g in 1; do #min_v 

                            # Ejecutamos bin/vad, escritura en fichero, y evaluacion
                            ejecutar
                            exitStatus=$?
                            while [ $exitStatus != 0 ]  # Si bin/vad ha retornado 1, se entrara a este bucle 
                            do
                                ejecutar    # Volvemos a ejecutar, con los mismos parametros abcde de antes
                                exitStatus=$?   # Seguramente ahora ha retornado 0
                                echo "Volviendo a ejecutar..."
                            done
                        done
                    done
                done
            done
        done
    done
done