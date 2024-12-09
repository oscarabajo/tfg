#!/bin/bash

EXCLUDE_DIR="$HOME/Escritorio/tfg/midifiles"
BPM=120


# Verificar si se proporcionó un argumento
if [ $# -ge 1 ]; then
    MIDI_FILE="$1"                            # Usar el archivo MIDI proporcionado
    CRIM_FILE="$(basename "$MIDI_FILE" .mid)" # Nombre base para el archivo CRIM2s
    USE_EXISTING_MIDI=true
else
    MIDI_FILE="pruebamultitrak.mid"           # Nombre del archivo MIDI a crear
    CRIM_FILE="pruebamultitrak"               # Nombre base para el archivo CRIM2s
    USE_EXISTING_MIDI=false
fi

# Iniciar FluidSynth en un terminal separado
gnome-terminal -- bash -c "fluidsynth -o synth.polyphony=512 -o synth.cpu-cores=2 -o audio.periods=64 /usr/share/sounds/sf2/FluidR3_GM.sf2; exec bash"
TERMINAL_PID=$!
sleep 5

# Verificar si FluidSynth está en ejecución
FLUID_PID=$(pgrep fluidsynth)
if [ -z "$FLUID_PID" ]; then
    echo "Error: FluidSynth no está en ejecución. Inícialo antes de continuar."
    exit 1
fi

# Conectar RtMidi con FluidSynth automáticamente
aconnect 14:0 128:0
if [ $? -eq 0 ]; then
    echo "Conexión MIDI establecida correctamente entre RtMidi y FluidSynth."
else
    echo "Error: No se pudo establecer la conexión MIDI."
    exit 1
fi

# Si no se proporcionó un archivo MIDI, crearlo
if [ "$USE_EXISTING_MIDI" = false ]; then
    # KREADOR MIDI
    echo "[*] KREADOR MIDI"
    python3 "$HOME/Escritorio/tfg/midiKreatore/creador_midiMultitrak.py" "$MIDI_FILE"

    if [ -f "$HOME/Escritorio/tfg/$MIDI_FILE" ]; then
        echo "Archivo MIDI '$MIDI_FILE' creado con éxito."
    else
        echo "Error: no se pudo crear el archivo MIDI."
        exit 1
    fi
fi

cd "$HOME/Escritorio/tfg" || exit

# EKSTRACCION MIDI
echo "[*] EKSTRACCION MIDI"
python3 ./decoder/midiXtractor.py "$MIDI_FILE" ./decoder/"$CRIM_FILE"

if [ -f ./decoder/"$CRIM_FILE".crim2s ]; then
    echo "Extracción completada."
else
    echo "Error: la extracción no generó los archivos esperados."
    exit 1
fi

# KOMPILADO C++
echo "[*] KOMPILADO C++"
# Compilación y enlace con RtMidi
g++ -I./recursos/rtmidi -c -o ./recursos/midiviewer.o ./recursos/plotNotasQT.cpp
g++ ./recursos/midiviewer.o -o ./recursos/midiviewer -lsfml-graphics -lsfml-window -lsfml-system -lrtmidi -pthread

# Verificar si la compilación fue exitosa
if [ $? -eq 0 ]; then
    echo "Compilación exitosa."
else
    echo "Error: falló la compilación del programa."
    exit 1
fi

# PLOTTEO
echo "[*] PLOTTEO"
if [ -f "./recursos/midiviewer" ]; then
    ./recursos/midiviewer ./decoder/"$CRIM_FILE".crim2s $BPM # Pasar el archivo .crim2s como argumento
else
    echo "Error: archivo ./recursos/midiviewer no encontrado."
    exit 1
fi

# Eliminar archivos .mid y .crim2s, excepto en $EXCLUDE_DIR
#echo "Eliminando archivos .crim3s, y .crim2s, excepto en $EXCLUDE_DIR..."

# ELIMINAR ARCHIVOS
#echo "[*] KLINIKO"
#find . -type f \( -name "*.crim3s" -o -name "*.crim2s" \) ! -path "$EXCLUDE_DIR/*" -exec rm -f {} +

# Cerrar FluidSynth al finalizar el script
echo "Cerrando FluidSynth..."
kill $FLUID_PID

# Fin del script
echo "Proceso completado."
