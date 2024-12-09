#include <SFML/Graphics.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <mutex>
#include <rtmidi/RtMidi.h>

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

std::mutex noteMutex;

sf::Color setColorByOctaveLinealAbss2(int note) {
    float colors[12][3] = {
        {255, 0, 0}, {255, 127, 0}, {255, 255, 0},
        {127, 255, 0}, {0, 255, 0}, {0, 255, 127},
        {0, 255, 255}, {0, 127, 255}, {0, 0, 255},
        {127, 0, 255}, {255, 0, 255}, {255, 0, 127}
    };
    return sf::Color(colors[note % 12][0], colors[note % 12][1], colors[note % 12][2]);
}

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
        if (sscanf(line.c_str(), "Time=%d Track=%d %s", &time, &trackIndex, msgType) >= 3) {
            if (std::string(msgType) == "note_on" || std::string(msgType) == "note_off") {
                if (sscanf(line.c_str(), "Time=%d Track=%d %s channel=%d note=%d velocity=%d", &time, &trackIndex, msgType, &channel, &note, &velocity) == 6) {
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
                    } else {
                        // Encontrar la nota y establecer endTime
                        for (auto& n : tracks[trackIndex].notes) {
                            if (n.note == note && n.endTime == -1) {
                                n.endTime = time;
                                break;
                            }
                        }
                    }
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

void processTrack(Track& track, float visualScrollSpeed, int trackIndex, float trackHeight, float noteHeight, float currentTimeSeconds, float activationLineX, std::vector<sf::RectangleShape>& noteShapes, RtMidiOut &midiout, float pixelsPerSecond, float ticksPerSecond) {
    float yOffset = trackIndex * trackHeight;

    for (auto& note : track.notes) {
        float noteStartTimeSeconds = note.startTime / ticksPerSecond;
        float noteEndTimeSeconds = note.endTime / ticksPerSecond;

        // Calcula la posición horizontal de la nota en la pantalla
        float xPosition = 800 - pixelsPerSecond * (currentTimeSeconds - noteStartTimeSeconds);
        float durationSeconds = noteEndTimeSeconds - noteStartTimeSeconds;
        float noteWidth = durationSeconds * pixelsPerSecond;

        // Genera la visualización de la nota en pantalla
        if (xPosition + noteWidth >= activationLineX && xPosition < 800) {
            sf::RectangleShape rect(sf::Vector2f(noteWidth, noteHeight - 5));
            rect.setFillColor(setColorByOctaveLinealAbss2(note.note));
            int noteIndex = note.note % 12;
            float yPosition = yOffset + noteIndex * noteHeight;
            rect.setPosition(xPosition, yPosition);

            // Bloquea el mutex para evitar conflictos al agregar una nueva forma
            std::lock_guard<std::mutex> lock(noteMutex);
            noteShapes.push_back(rect);
        }

        // Enviar note_on cuando la nota cruce la línea de activación
        if (!note.noteOnSent && xPosition <= activationLineX) {
            std::vector<unsigned char> message = {0x90, static_cast<unsigned char>(note.note), 64};
            midiout.sendMessage(&message);
            note.noteOnSent = true;
        }

        // Enviar note_off cuando termine la duración de la nota
        if (!note.noteOffSent && xPosition + noteWidth <= activationLineX) {
            std::vector<unsigned char> message = {0x80, static_cast<unsigned char>(note.note), 64};
            midiout.sendMessage(&message);
            note.noteOffSent = true;
        }
    }
}

int main(int argc, char* argv[]) {
    // Verifica que el usuario proporcione el archivo de entrada y los BPM como argumentos
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <ruta al archivo .crim2s> <bpm>" << std::endl;
        return -1;
    }
    std::string crim2sFilePath = argv[1];
    float bpm = std::stof(argv[2]);

    // Inicializa salida MIDI
    RtMidiOut midiout;
    unsigned int nPorts = midiout.getPortCount();
    if (nPorts == 0) {
        std::cout << "No ports available!\n";
        return -1;
    }
    midiout.openPort(0);

    int ticksPerBeat = 480; // Valor por defecto, se actualizará al leer el archivo
    std::vector<Track> tracks = readCrim2sFile(crim2sFilePath, ticksPerBeat);
    if (tracks.empty()) {
        std::cerr << "Error: no se encontraron pistas en el archivo." << std::endl;
        return -1;
    }

    // Calcula ticks por segundo
    float beatsPerSecond = bpm / 60.0f;
    float ticksPerSecond = ticksPerBeat * beatsPerSecond;

    // Configura ventana de visualización
    sf::RenderWindow window(sf::VideoMode(800, 600), "MIDI Visualizer with Tracks");

    // Define la altura de cada pista y de cada nota
    int numTracks = tracks.size();
    float trackHeight = 600.0f / numTracks;
    float noteHeight = trackHeight / 12.0f;
    float pixelsPerSecond = 100.0f; // Ajusta este valor para cambiar la escala horizontal

    sf::Clock clock;
    float activationLineX = 200.0f;
    std::vector<sf::RectangleShape> noteShapes;

    // Bucle principal de la ventana
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        sf::Time elapsed = clock.getElapsedTime();
        float currentTimeSeconds = elapsed.asSeconds();

        window.clear();

        // Dibuja la línea de activación
        sf::RectangleShape line(sf::Vector2f(2, 600));
        line.setFillColor(sf::Color(128, 128, 128));
        line.setPosition(activationLineX, 0);
        window.draw(line);

        // Dibuja las líneas que delimitan las pistas
        for (int i = 1; i < numTracks; ++i) {
            float yPosition = i * trackHeight;
            sf::RectangleShape trackLine(sf::Vector2f(800, 2));
            trackLine.setFillColor(sf::Color(192, 192, 192));
            trackLine.setPosition(0, yPosition);
            window.draw(trackLine);
        }

    // Procesa cada pista para visualización y reproducción
        for (int i = 0; i < numTracks; ++i) {
            processTrack(tracks[i], 0.0f, i, trackHeight, noteHeight, currentTimeSeconds, activationLineX, noteShapes, midiout, pixelsPerSecond, ticksPerSecond);
        }

        // Dibuja las notas en pantalla
        for (const auto& shape : noteShapes) {
            if (shape.getPosition().x + shape.getSize().x > 0 && shape.getPosition().x < 800) {
                window.draw(shape);
            }
        }
        noteShapes.clear(); // Limpia las formas de notas para la próxima iteración
        window.display();   // Muestra el contenido en la ventana
    }

    return 0;
}
