import mido
from mido import Message, MidiFile, MidiTrack
import sys
import random

# Verificar si se pasó un nombre de archivo como argumento
if len(sys.argv) < 2:
    print("Por favor, proporciona un nombre de archivo.")
    sys.exit(1)

# Nombre del archivo MIDI
filename = sys.argv[1]

# Crear un nuevo archivo MIDI
mid = MidiFile()
ticks_per_beat = 480  # Establecer ticks por negra (opcional)
mid.ticks_per_beat = ticks_per_beat

# Definir la intensidad de las notas y la duración en ticks
velocity = 64
note_duration = 480  # Duración de 1 beat
silence_duration = 240  # Silencio entre notas (medio beat)

# --- Primera pista: Todas las notas pares ---
track1 = MidiTrack()
mid.tracks.append(track1)
track1.name = 'Notas Pares'

# Añadir un mensaje de cambio de programa a la pista 1
track1.append(Message('program_change', program=0, time=0))

# Generar notas pares (del rango MIDI estándar 21 a 108)
for note in range(22, 108, 2):  # Notas pares
    track1.append(Message('note_on', note=note, velocity=velocity, time=0))
    track1.append(Message('note_off', note=note, velocity=velocity, time=note_duration))
    # Añadir silencio entre notas
    track1.append(Message('note_off', note=0, velocity=0, time=silence_duration))

# --- Segunda pista: Todas las notas impares ---
track2 = MidiTrack()
mid.tracks.append(track2)
track2.name = 'Notas Impares'

# Añadir un mensaje de cambio de programa a la pista 2
track2.append(Message('program_change', program=0, time=0))

# Generar notas impares (del rango MIDI estándar 21 a 108)
for note in range(21, 108, 2):  # Notas impares
    track2.append(Message('note_on', note=note, velocity=velocity, time=0))
    track2.append(Message('note_off', note=note, velocity=velocity, time=note_duration))
    # Añadir silencio entre notas
    track2.append(Message('note_off', note=0, velocity=0, time=silence_duration))

# --- Tercera pista: Notas aleatorias con duraciones variables ---
track3 = MidiTrack()
mid.tracks.append(track3)
track3.name = 'Notas Aleatorias'

# Añadir un mensaje de cambio de programa a la pista 3
track3.append(Message('program_change', program=0, time=0))

# Generar notas aleatorias
num_random_notes = 50  # Número de notas aleatorias a generar
for _ in range(num_random_notes):
    note = random.randint(21, 108)  # Nota aleatoria en rango MIDI estándar
    duration = random.randint(240, 1920)  # Duración aleatoria entre medio beat y 4 beats
    track3.append(Message('note_on', note=note, velocity=velocity, time=0))
    track3.append(Message('note_off', note=note, velocity=velocity, time=duration))
    # Añadir silencio aleatorio entre notas
    silence = random.randint(0, 480)
    track3.append(Message('note_off', note=0, velocity=0, time=silence))

# --- Cuarta pista: Progresión de 4 acordes que se repite en 3 escalas ---
track4 = MidiTrack()
mid.tracks.append(track4)
track4.name = 'Progresión de Acordes'

# Añadir un mensaje de cambio de programa a la pista 4
track4.append(Message('program_change', program=0, time=0))

# Definir la progresión de acordes (usaremos grados de la escala mayor)
chord_progression = [
    [0, 4, 7],    # I (acorde de tónica)
    [5, 9, 12],   # IV (subdominante)
    [7, 11, 14],  # V (dominante)
    [0, 4, 7]     # I (tónica)
]

# Escalas a utilizar (Do, Re, Mi mayor)
scales = [
    {'root': 60},  # Do Mayor (C4)
    {'root': 62},  # Re Mayor (D4)
    {'root': 64}   # Mi Mayor (E4)
]

for scale in scales:
    root_note = scale['root']
    for chord in chord_progression:
        # Calcular las notas del acorde en la escala actual
        chord_notes = [root_note + interval for interval in chord]
        # Activar todas las notas del acorde
        for idx, note in enumerate(chord_notes):
            if idx == 0:
                delta_time = 0  # Comienza inmediatamente
                track4.append(Message('note_on', note=note, velocity=velocity, time=delta_time))
            else:
                track4.append(Message('note_on', note=note, velocity=velocity, time=0))
        # Duración del acorde
        chord_duration = 2 * ticks_per_beat  # Cada acorde dura 2 beats
        # Desactivar todas las notas del acorde
        for idx, note in enumerate(chord_notes):
            if idx == 0:
                track4.append(Message('note_off', note=note, velocity=velocity, time=chord_duration))
            else:
                track4.append(Message('note_off', note=note, velocity=velocity, time=0))
        # Añadir silencio entre acordes
        silence_between_chords = ticks_per_beat  # Silencio de 1 beat
        track4.append(Message('note_off', note=0, velocity=0, time=silence_between_chords))

# Guardar el archivo MIDI con el nombre proporcionado
mid.save(filename)
print(f"Archivo MIDI '{filename}' creado con éxito.")
