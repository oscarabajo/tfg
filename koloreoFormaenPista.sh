#!/bin/bash

EXCLUDE_DIR="$HOME/Escritorio/tfg/tfg/midifiles"
BPM=180
BPM=120
MIX_STRATEGY="average"  # Valor por defecto
USE_EXISTING_MIDI=false
MIDI_FILE="default.mid"
CRIM_FILE="default"



# Verificar si se proporcionaron argumentos adicionales
if [ $# -ge 1 ]; then
    MIDI_FILE="$1"                            # Usar el archivo MIDI proporcionado
    CRIM_FILE="$(basename "$MIDI_FILE" .mid)" # Nombre base para el archivo CRIM2s
    USE_EXISTING_MIDI=true
fi

if [ $# -ge 2 ]; then
    BPM="$2"  # Sobrescribir BPM si se proporciona
fi

if [ $# -ge 3 ]; then
    MIX_STRATEGY="$3"  # Establecer estrategia de mezcla si se proporciona
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

echo "[*] CONEXION CORRECTA CON FLUYDSYNTH"

# Si no se proporcionó un archivo MIDI, crearlo
if [ "$USE_EXISTING_MIDI" = false ]; then
    # KREADOR MIDI
    echo "[*] KREADOR MIDI"
   # python3 "$HOME/Escritorio/tfg/tfg/midiKreatore/creador_midi.py" "$MIDI_FILE"
    python3 "$HOME/Escritorio/tfg/tfg/midiKreatore/creador_midiMultitrak.py" "$MIDI_FILE"

    if [ -f "$HOME/Escritorio/tfg/tfg/$MIDI_FILE" ]; then
        echo "Archivo MIDI '$MIDI_FILE' creado con éxito."
    else
        echo "Error: no se pudo crear el archivo MIDI."
        exit 1
    fi
fi

cd "$HOME/Escritorio/tfg/tfg" || exit

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
# Usar Makefile para compilar
make -C ./recursos/forma_en_pista clean  # Limpiar compilaciones previas
make -C ./recursos/forma_en_pista CRIM_FILE="$CRIM_FILE" BPM="$BPM" MIX_STRATEGY="$MIX_STRATEGY" run  # Compilar y ejecutar

# PLOTTEO
echo "[*] PLOTTEO"
if [ -f "./recursos/forma_en_pista/midiviewer" ]; then
    ./recursos/forma_en_pista/midiviewer "./decoder/$CRIM_FILE.crim2s" "$BPM" "$MIX_STRATEGY" # Pasar el archivo .crim2s y la estrategia de mezcla como argumentos
else
    echo "Error: archivo ./recursos/forma_en_pista/midiviewer no encontrado."
    exit 1
fi

# PLOTTEO
echo "[*] PLOTTEO"
if [ -f "./recursos/forma_en_pista/midiviewer" ]; then
    ./recursos/forma_en_pista/midiviewer ./decoder/"$CRIM_FILE".crim2s $BPM # Pasar el archivo .crim2s como argumento
else
    echo "Error: archivo ./recursos/forma_en_pista/midiviewer no encontrado."
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
