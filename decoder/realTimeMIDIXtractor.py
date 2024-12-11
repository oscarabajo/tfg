import mido
import sys
import time
import os

def open_crim2s_pipe(pipe_path):
    """ Abre el named pipe para escribir los eventos MIDI """
    if not os.path.exists(pipe_path):
        os.mkfifo(pipe_path)  # Crea el pipe si no existe
    return open(pipe_path, 'w')

def main():
    if len(sys.argv) != 2:
        print("Uso: python midiXtractor.py <nombre_del_pipe>")
        sys.exit(1)
    
    pipe_path = sys.argv[1]
    crim2s_pipe = open_crim2s_pipe(pipe_path)
    
    # Inicializar ticks_per_beat (puedes ajustar según sea necesario)
    ticks_per_beat = 480  # Ajusta el valor dependiendo del MIDI

    # Escribir encabezado en el pipe
    crim2s_pipe.write(f"Archivo MIDI en Tiempo Real\n")
    crim2s_pipe.write(f"Ticks per beat: {ticks_per_beat}\n")
    crim2s_pipe.write(f"Número de pistas: 1\n")
    crim2s_pipe.write("Eventos:\n")
    crim2s_pipe.flush()

    # Abrir el puerto MIDI de entrada (ajusta el nombre según tu controlador)
    input_names = mido.get_input_names()
    if not input_names:
        print("No se encontraron puertos MIDI disponibles.")
        sys.exit(1)
    
    print("Puertos MIDI disponibles:")
    for name in input_names:
        print(f" - {name}")
    
    # Seleccionar el primer puerto disponible
    with mido.open_input(input_names[0]) as inport:
        print(f"Escuchando en el puerto MIDI: {input_names[0]}")
        start_time = time.time()
        current_ticks = 0

        for msg in inport:
            # Depuración: Ver el mensaje MIDI
            print(f"Mensaje MIDI recibido: {msg}")
            
            # Calcula el tiempo transcurrido en segundos
            elapsed_time = time.time() - start_time
            # Convertir el tiempo transcurrido a ticks
            bpm = 120  # Ajusta según sea necesario
            beats_per_second = bpm / 60.0
            ticks_per_second = ticks_per_beat * beats_per_second
            current_ticks = int(elapsed_time * ticks_per_second)

            if msg.type in ['note_on', 'note_off']:
                # Escribir el mensaje en el pipe con el tiempo calculado
                crim2s_pipe.write(f"Time={current_ticks} Track=0 {msg}\n")
                crim2s_pipe.flush()

if __name__ == "__main__":
    main()
