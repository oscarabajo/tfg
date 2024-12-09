import mido
import sys

def midi_to_text(midi_file_path, output_text_file):
    mid = mido.MidiFile(midi_file_path)

    # Obtener ticks por negra
    ticks_per_beat = mid.ticks_per_beat

    # Lista para mantener todos los eventos con su tiempo acumulado
    all_events = []

    # Acumular el tiempo globalmente
    for i, track in enumerate(mid.tracks):
        current_ticks = 0
        for msg in track:
            current_ticks += msg.time
            if not msg.is_meta:
                all_events.append((current_ticks, i, msg))

    # Ordenar todos los eventos por tiempo acumulado
    all_events.sort(key=lambda x: x[0])

    total_ticks = all_events[-1][0] if all_events else 0

    # Escribir los eventos en el archivo .crim2s
    with open(output_text_file + '.crim2s', 'w') as file:
        file.write(f"Archivo MIDI: {midi_file_path}\n")
        file.write(f"Ticks per beat: {ticks_per_beat}\n")
        file.write(f"Tiempo total de la canción: {total_ticks} ticks\n")
        file.write(f"Número de pistas: {len(mid.tracks)}\n")
        file.write("Eventos:\n")

        for time, track_index, msg in all_events:
            if msg.type in ['note_on', 'note_off']:
                file.write(f"Time={time} Track={track_index} {msg}\n")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Uso: python midiXtractor.py <ruta_al_archivo_midi> <archivo_de_salida>")
        sys.exit(1)

    input_path = sys.argv[1]
    output = sys.argv[2]
    midi_to_text(input_path, output)
