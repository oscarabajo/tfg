import mido
from mido import Message, MidiFile, MidiTrack
import sys

# Verificar si se pasó un nombre de archivo como argumento
if len(sys.argv) < 2:
    print("Por favor, proporciona un nombre de archivo.")
    sys.exit(1)

# Nombre del archivo MIDI
filename = sys.argv[1]

# Crear un nuevo archivo MIDI
mid = MidiFile()
track = MidiTrack()
mid.tracks.append(track)

# Definir la intensidad de las notas y la duración en ticks (asumiendo 480 ticks por beat y 120 BPM)
velocity = 64
note_duration = 2 * 480  # Duración de 2 beats en 120 BPM
silence_duration = 480   # Silencio entre notas

# Añadir un mensaje de cambio de programa a la pista
track.append(Message('program_change', program=0, time=0))

# Generar todas las notas en orden ascendente (1 a 127)
for note in range(60, 80):
    track.append(Message('note_on', note=note, velocity=velocity, time=0))
    track.append(Message('note_off', note=note, velocity=velocity, time=note_duration))
    # Añadir silencio entre notas
    track.append(Message('note_off', note=0, velocity=0, time=silence_duration))

# Generar acordes de 2, 3 y 5 notas
chords = [
    [60, 64],              # Acorde de 2 notas (C y E)
    [60, 64, 67],          # Acorde de 3 notas (C, E, G)
    [60, 64, 67, 71, 74]   # Acorde de 5 notas (C, E, G, B, D)
]

first_chord = True
for chord in chords:
    # Activar todas las notas del acorde
    for idx, note in enumerate(chord):
        if idx == 0:
            if first_chord:
                delta_time = 0  # Primer acorde, sin silencio antes
                first_chord = False
            else:
                delta_time = silence_duration  # Silencio entre acordes
            track.append(Message('note_on', note=note, velocity=velocity, time=delta_time))
        else:
            # Las notas adicionales del acorde comienzan al mismo tiempo (delta_time = 0)
            track.append(Message('note_on', note=note, velocity=velocity, time=0))
    # Desactivar todas las notas del acorde después de la duración
    for idx, note in enumerate(chord):
        if idx == 0:
            # El primer 'note_off' ocurre después de 'note_duration' ticks
            track.append(Message('note_off', note=note, velocity=velocity, time=note_duration))
        else:
            # Las notas adicionales terminan al mismo tiempo (delta_time = 0)
            track.append(Message('note_off', note=note, velocity=velocity, time=0))

# Guardar el archivo MIDI con el nombre proporcionado
mid.save(filename)
print(f"Archivo MIDI '{filename}' creado con éxito.")
