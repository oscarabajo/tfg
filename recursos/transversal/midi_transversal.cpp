#include <SFML/Graphics.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <rtmidi/RtMidi.h>
#include <mutex>
#include "../colorFunctions/colorFunctions.h"  // Asegúrate de que este archivo está correctamente incluido

struct NoteEvent {
    int note;
    int startTime;
    int endTime;
    bool noteOnSent = false;
    bool noteOffSent = false;
};

struct Track {
    std::vector<NoteEvent> notes;
};

struct TrackState {
    int activeNotes = 0;
    sf::Color currentColor = sf::Color::Black;
    std::vector<sf::Color> activeNoteColors;
};

// Función para leer el archivo .crim2s y extraer las pistas y eventos
std::vector<Track> readCrim2sFile(const std::string& filename, int& ticksPerBeat) {
    std::ifstream file(filename);
    std::vector<Track> tracks;
    if (!file.is_open()) {
        std::cerr << "Error al abrir el archivo " << filename << std::endl;
        return tracks;
    }
    std::string line;
    int totalTracks = 0;
    // Leer información del encabezado
    while (std::getline(file, line)) {
        if (line.find("Ticks per beat:") != std::string::npos) {
            sscanf(line.c_str(), "Ticks per beat: %d", &ticksPerBeat);
        } else if (line.find("Número de pistas:") != std::string::npos) {
            sscanf(line.c_str(), "Número de pistas: %d", &totalTracks);
            tracks.resize(totalTracks);
        } else if (line.find("Eventos:") != std::string::npos) {
            break;
        }
    }

    // Leer los eventos
    while (std::getline(file, line)) {
        int time = 0, trackIndex = 0;
        char msgType[16];
        int channel = 0, note = 0, velocity = 0;
        // Modificar el sscanf para ignorar el último campo time=<int>
        if (sscanf(line.c_str(), "Time=%d Track=%d %s channel=%d note=%d velocity=%d time=%*d",
                   &time, &trackIndex, msgType, &channel, &note, &velocity) == 6) {
            if (trackIndex >= totalTracks) {
                std::cerr << "Índice de pista inválido " << trackIndex << std::endl;
                continue;
            }
            if (std::string(msgType) == "note_on" && velocity > 0) {
                NoteEvent newNote;
                newNote.note = note;
                newNote.startTime = time;
                newNote.endTime = -1;
                tracks[trackIndex].notes.push_back(newNote);
            } else if (std::string(msgType) == "note_off" || (std::string(msgType) == "note_on" && velocity == 0)) {
                // Encontrar la nota y establecer endTime
                bool found = false;
                for (auto& n : tracks[trackIndex].notes) {
                    if (n.note == note && n.endTime == -1) {
                        n.endTime = time;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    std::cerr << "Nota_off encontrada sin nota_on correspondiente: Nota=" << note << std::endl;
                }
            }
        }
    }

    // Establecer endTime para notas que no lo tienen
    int maxTime = 0;
    for (const auto& track : tracks) {
        for (const auto& note : track.notes) {
            if (note.endTime > maxTime) {
                maxTime = note.endTime;
            }
        }
    }
    for (auto& track : tracks) {
        for (auto& note : track.notes) {
            if (note.endTime == -1) {
                note.endTime = maxTime;
            }
        }
    }

    file.close();
    return tracks;
}

int main(int argc, char* argv[]) {
    // Verificar que el usuario proporcione el archivo de entrada, los BPM y la estrategia de mezcla
    if (argc != 4) {
        std::cerr << "Uso: " << argv[0] << " <ruta al archivo .crim2s> <bpm> <mix_strategy>" << std::endl;
        std::cerr << "mix_strategy: sum | average" << std::endl;
        return -1;
    }
    std::string crim2sFilePath = argv[1];
    float bpm = std::stof(argv[2]);
    std::string mixStrategy = argv[3];

    // Seleccionar la función de mezcla basada en el parámetro
    typedef sf::Color (*MixFunction)(const sf::Color&, const sf::Color&);
    MixFunction mixFunc = nullptr;

    if (mixStrategy == "sum") {
        mixFunc = mixColorsSum;
    } else if (mixStrategy == "average") {
        mixFunc = mixColorsAverage;
    } else {
        std::cerr << "Estrategia de mezcla desconocida: " << mixStrategy << std::endl;
        std::cerr << "Usa 'sum' o 'average'." << std::endl;
        return -1;
    }

    // Inicializa salida MIDI
    RtMidiOut midiout;
    unsigned int nPorts = midiout.getPortCount();
    if (nPorts == 0) {
        std::cout << "No hay puertos MIDI disponibles.\n";
        return -1;
    }
    midiout.openPort(0);

    int ticksPerBeat = 480; // Valor por defecto, se actualizará al leer el archivo
    std::vector<Track> tracks = readCrim2sFile(crim2sFilePath, ticksPerBeat);
    std::cout << "Número de pistas leídas: " << tracks.size() << std::endl; // Depuración
    if (tracks.empty()) {
        std::cerr << "Error: no se encontraron pistas en el archivo." << std::endl;
        return -1;
    }

    // Calcula ticks por segundo
    float beatsPerSecond = bpm / 60.0f;
    float ticksPerSecond = ticksPerBeat * beatsPerSecond;

    // Configura ventana de visualización
    sf::RenderWindow window(sf::VideoMode(800, 600), "Vista Transversal MIDI");

    // Define parámetros de visualización
    int numTracks = tracks.size();
    float rectWidth = 750.0f; // Ancho de los rectángulos
    float rectHeight = 30.0f; // Altura de los rectángulos
    float rectSpacing = 10.0f; // Espacio entre rectángulos
    float startY = 50.0f; // Posición Y inicial

    // Crear rectángulos para cada pista
    std::vector<sf::RectangleShape> trackRectangles;
    std::vector<TrackState> trackStates(numTracks, TrackState());

    for (int i = 0; i < numTracks; ++i) {
        sf::RectangleShape rect(sf::Vector2f(rectWidth, rectHeight));
        rect.setFillColor(sf::Color::Black); // Color inicial
        rect.setPosition(25.0f, startY + i * (rectHeight + rectSpacing));
        trackRectangles.push_back(rect);
    }

    // Clock para controlar el tiempo
    sf::Clock clock;

    // Bucle principal de la ventana
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Tiempo actual en ticks
        float currentTimeSeconds = clock.getElapsedTime().asSeconds();
        int currentTimeTicks = static_cast<int>(currentTimeSeconds * ticksPerSecond);

        // Procesar eventos de notas
        for (int i = 0; i < numTracks; ++i) {
            for (auto& note : tracks[i].notes) {
                if (!note.noteOnSent && currentTimeTicks >= note.startTime) {
                    // Enviar note_on
                    std::vector<unsigned char> message = {0x90, static_cast<unsigned char>(note.note), 64};
                    midiout.sendMessage(&message);
                    note.noteOnSent = true;

                    // Incrementar contador de notas activas
                    trackStates[i].activeNotes += 1;

                    // Obtener el color de la nota
                    sf::Color noteColor = setColorByOctaveLinealAbss2(note.note);

                    // Agregar el color al vector de colores activos
                    trackStates[i].activeNoteColors.push_back(noteColor);

                    // Recalcular el color actual usando la estrategia de mezcla
                    trackStates[i].currentColor = applyMixingStrategy(trackStates[i].activeNoteColors, mixFunc);

                    // Actualizar el color del rectángulo
                    trackRectangles[i].setFillColor(trackStates[i].currentColor);
                }

                if (!note.noteOffSent && currentTimeTicks >= note.endTime) {
                    // Enviar note_off
                    std::vector<unsigned char> message = {0x80, static_cast<unsigned char>(note.note), 64};
                    midiout.sendMessage(&message);
                    note.noteOffSent = true;

                    // Decrementar contador de notas activas
                    if (trackStates[i].activeNotes > 0) {
                        trackStates[i].activeNotes -= 1;
                    }

                    // Obtener el color de la nota
                    sf::Color noteColor = setColorByOctaveLinealAbss2(note.note);

                    // Remover el color del vector de colores activos
                    // Busca la primera ocurrencia del color y lo elimina
                    auto it = std::find(trackStates[i].activeNoteColors.begin(), trackStates[i].activeNoteColors.end(), noteColor);
                    if (it != trackStates[i].activeNoteColors.end()) {
                        trackStates[i].activeNoteColors.erase(it);
                    } else {
                        std::cerr << "Advertencia: color de nota no encontrado en activeNoteColors para la pista " << i << std::endl;
                    }

                    // Recalcular el color actual usando la estrategia de mezcla
                    if (!trackStates[i].activeNoteColors.empty()) {
                        trackStates[i].currentColor = applyMixingStrategy(trackStates[i].activeNoteColors, mixFunc);
                    } else {
                        trackStates[i].currentColor = sf::Color::Black;
                    }

                    // Actualizar el color del rectángulo
                    trackRectangles[i].setFillColor(trackStates[i].currentColor);
                }
            }
        }

        // Renderizar
        window.clear(sf::Color::Black);

        // Dibujar rectángulos de las pistas
        for (int i = 0; i < numTracks; ++i) {
            window.draw(trackRectangles[i]);
        }

        window.display();
    }

    return 0;
}
