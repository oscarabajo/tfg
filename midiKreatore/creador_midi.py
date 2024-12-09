#!/usr/bin/env python3

import sys
from mido import Message, MidiFile, MidiTrack

def print_usage():
    print("Uso: python3 midiKaleidoskope.py <nombre_del_archivo.mid>")
    print("Ejemplo: python3 midiKaleidoskope.py salida.mid")

def main():
    # Verificar si se proporcionó un argumento
    if len(sys.argv) != 2:
        print("Error: Número incorrecto de argumentos.")
        print_usage()
        sys.exit(1)

    output_filename = sys.argv[1]

    # Asegurarse de que el nombre del archivo termina con .mid
    if not output_filename.lower().endswith('.mid'):
        print("Advertencia: La extensión del archivo no es '.mid'. Se agregará automáticamente.")
        output_filename += '.mid'

    # Crear un nuevo archivo MIDI
    mid = MidiFile()

    # Definir la intensidad de las notas
    velocity = 64

    # Crear la primera pista
    track1 = MidiTrack()
    mid.tracks.append(track1)

    # Añadir un mensaje de cambio de programa a la primera pista (opcional)
    track1.append(Message('program_change', program=0, time=0))

    # Definir tres tipos de duraciones: corta, media, y larga
    durations = {
        "corta": 480,  # 1 beat (asumiendo 120 BPM y 480 ticks por beat)
       # "media": 960,  # 2 beats
        #"larga": 1920  # 4 beats
    }

    # Silencio entre notas
    silence_duration = 480

    # Generar las notas de 1 a 127
    for note in range(1, 128):
        # Alternar entre duraciones cortas, medias y largas
        duration_type = list(durations.keys())[note % len(durations)]
        duration = durations[duration_type]

        # Añadir la nota a la primera pista
        track1.append(Message('note_on', note=note, velocity=velocity, time=0))
        track1.append(Message('note_off', note=note, velocity=velocity, time=duration))
        track1.append(Message('note_on', note=note, velocity=0, time=silence_duration))  # Silencio

    # Guardar el archivo MIDI
    try:
        mid.save(output_filename)
        print(f"Archivo MIDI '{output_filename}' creado con éxito.")
    except Exception as e:
        print(f"Error al guardar el archivo MIDI: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
