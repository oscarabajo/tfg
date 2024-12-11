# listar_puertos_midi.py

import mido

def listar_puertos_midi():
    input_names = mido.get_input_names()
    output_names = mido.get_output_names()

    if input_names:
        print("Puertos MIDI de Entrada Disponibles:")
        for i, name in enumerate(input_names, 1):
            print(f"{i}. {name}")
    else:
        print("No se encontraron puertos MIDI de entrada.")

    if output_names:
        print("\nPuertos MIDI de Salida Disponibles:")
        for i, name in enumerate(output_names, 1):
            print(f"{i}. {name}")
    else:
        print("No se encontraron puertos MIDI de salida.")

if __name__ == "__main__":
    listar_puertos_midi()
